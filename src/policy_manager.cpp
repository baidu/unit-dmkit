// Copyright (c) 2018 Baidu, Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "policy_manager.h"
#include <ctime>
#include <forward_list>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <unordered_set>
#include <unordered_map>
#include <unistd.h>
#include <utility>
#include "app_log.h"
#include "utils.h"

namespace dmkit {

static const int POLICY_DICT_RELOAD_INTERVAL = 1000;

DomainPolicy::DomainPolicy(const std::string& name, int score, IntentPolicyMap* intent_policy_map)
    : _name(name), _score(score), _intent_policy_map(intent_policy_map) {
}

DomainPolicy::~DomainPolicy() {
    if (this->_intent_policy_map == nullptr) {
        return;
    }
    for (IntentPolicyMap::iterator iter = this->_intent_policy_map->begin();
            iter != this->_intent_policy_map->end(); ++iter) {
        auto policy_vector = iter->second;
        if (policy_vector == nullptr) {
            continue;
        }
        for (PolicyVector::iterator iter2 = policy_vector->begin();
                iter2 != policy_vector->end(); ++iter2) {
            auto policy = *iter2;
            if (policy == nullptr) {
                continue;
            }
            delete policy;
            *iter2 = nullptr;
        }
        delete policy_vector;
        iter->second = nullptr;
    }
    delete this->_intent_policy_map;
    this->_intent_policy_map = nullptr;
}

int DomainPolicy::score() {
    return this->_score;
}

const std::string& DomainPolicy::name() {
    return this->_name;
}

IntentPolicyMap* DomainPolicy::intent_policy_map() {
    return this->_intent_policy_map;
}

PolicyManager::PolicyManager() {
    this->_user_function_manager = nullptr;
    this->_dual_policy_dict[0] = nullptr;
    this->_dual_policy_dict[1] = nullptr;
    this->_current_policy_dict = 0;
    this->_destroyed = false;
}

PolicyManager::~PolicyManager() {
    if (this->_user_function_manager != nullptr) {
        delete this->_user_function_manager;
        this->_user_function_manager = nullptr;
    }

    this->_destroyed = true;
    this->_reload_thread.join();
    this->destroy_policy_dict(this->_dual_policy_dict[0]);
    this->destroy_policy_dict(this->_dual_policy_dict[1]);
    this->_dual_policy_dict[0] = nullptr;
    this->_dual_policy_dict[1] = nullptr;
}

static int get_file_last_modified_time(const std::string& file_path, std::string& mtime_str) {
    struct stat f_stat;
    if (stat(file_path.c_str(), &f_stat) != 0) {
        LOG(WARNING) << "Failed to get file modified time" << file_path;
        return -1;
    }
    mtime_str = ctime(&f_stat.st_mtime);
    return 0;
}

// Loads policies from JSON configuration files.
int PolicyManager::init(const char* dir_path, const char* conf_file) {
    std::string file_path;
    if (dir_path != nullptr) {
        file_path += dir_path;
    }
    if (!file_path.empty() && file_path[file_path.length() - 1] != '/') {
        file_path += '/';
    }
    if (conf_file != nullptr) {
        file_path += conf_file;
    }
    this->_conf_file_path = file_path;

    this->_dual_policy_dict[0] = this->load_policy_dict();
    if (this->_dual_policy_dict[0] == nullptr) {
        APP_LOG(ERROR) << "Failed to init policy dict";
        return -1;        
    }
    this->_current_policy_dict = 0;
    this->_policy_dict_ref_count[0] = 0;
    this->_policy_dict_ref_count[1] = 0;
    get_file_last_modified_time(this->_conf_file_path, this->_conf_file_last_modified_time);
    LOG(TRACE) << "Policy conf last modify time: " << this->_conf_file_last_modified_time;
    this->_reload_thread = std::thread(&PolicyManager::reload_thread_func, this);

    this->_user_function_manager = new UserFunctionManager();
    if (this->_user_function_manager->init() != 0) {
        APP_LOG(ERROR) << "Failed to init UserFunctionManager";
        return -1;
    }

    return 0;
}

int PolicyManager::reload() {
    LOG(TRACE) << "Reloading policy dict";
    std::lock_guard<std::mutex> lock(this->_policy_dict_mutex);
    int previous_policy_dict = 1 - this->_current_policy_dict;
    if (this->_policy_dict_ref_count[previous_policy_dict] > 0) {
        LOG(WARNING) << "Cannot reload policy! Previous policy dict still in use.";
        return -1;
    }
    this->destroy_policy_dict(this->_dual_policy_dict[previous_policy_dict]);
    this->_dual_policy_dict[previous_policy_dict] = this->load_policy_dict();
    if (this->_dual_policy_dict[previous_policy_dict] == nullptr) {
        LOG(WARNING) << "Cannot reload policy! Policy dict load failed.";
        return -1;
    }
    this->_current_policy_dict = previous_policy_dict;
    LOG(TRACE) << "Reload finished";
    return 0;
}

PolicyOutput* PolicyManager::resolve(const std::string& product,
                                     BUTIL_NAMESPACE::FlatMap<std::string, QuResult*>* qu_result,
                                     const PolicyOutputSession& session,
                                     const RequestContext& context) {
    ProductPolicyMap* product_policy_map = this->acquire_policy_dict();
    DomainPolicyMap** seek_result = product_policy_map->seek(product);
    if (seek_result == nullptr) {
        APP_LOG(WARNING) << "unkown product " << product;
        this->release_policy_dict(product_policy_map);
        return nullptr;
    }
    DomainPolicyMap* domain_policy_map = *seek_result;
    DomainPolicy* state_domain_policy = nullptr;
    Policy* state_policy = nullptr;
    std::forward_list<std::pair<DomainPolicy*, Policy*>> ranked_policies;
    std::vector<Slot> empty_slots;
    QuResult empty_qu("", "", empty_slots);
    std::string request_domain;
    context.try_get_param("domain", request_domain);
    for (DomainPolicyMap::iterator iter = domain_policy_map->begin();
            iter != domain_policy_map->end(); ++iter) {
        auto domain_policy = iter->second;
        const std::string& domain_name = domain_policy->name();
        if (!request_domain.empty() && domain_name != request_domain) {
            continue;
        }
        QuResult** qu_seek_result = qu_result->seek(domain_name);
        QuResult* qu = qu_seek_result != nullptr ? *qu_seek_result : &empty_qu;

        // Find a best policy give the qu result in current domain
        Policy* find_result = this->find_best_policy(domain_policy, qu, session, context);
        if (find_result == nullptr) {
            continue;
        }

        // Policy with a domain and trigger state matches current DMKit domain&state is ranked top
        if (!session.domain.empty() && (domain_name == session.domain)
            && !session.state.empty() && (find_result->trigger().state == session.state)) {
            state_domain_policy = domain_policy;
            state_policy = find_result;;
            continue;
        }

        // In case there are multiple domain results, the policies a ranked by static domain score
        auto previous_it = ranked_policies.before_begin();
        for (auto it = ranked_policies.begin(); it != ranked_policies.end(); ++it) {
            if (domain_policy->score() > it->first->score()) {
                break;
            }
            previous_it = it;
        }
        std::pair<DomainPolicy*, Policy*> p(domain_policy, find_result);
        ranked_policies.insert_after(previous_it, p);
    }

    if (state_domain_policy != nullptr && state_policy != nullptr) {
        std::pair<DomainPolicy*, Policy*> p(state_domain_policy, state_policy);
        ranked_policies.insert_after(ranked_policies.before_begin(), p);
    }

    // Resolve policy output and returns the first output resolved successfully.
    for (auto& p: ranked_policies) {
        const std::string& domain_name = p.first->name();
        APP_LOG(TRACE) << "Resolving policy output for domain [" << domain_name << "]";
        QuResult** qu_seek_result = qu_result->seek(domain_name);
        QuResult* qu = qu_seek_result != nullptr ? *qu_seek_result : &empty_qu;
        PolicyOutput* result = this->resolve_policy_output(domain_name,
                p.second, qu, session, context);
        if (result != nullptr) {
            APP_LOG(TRACE) << "Final result domain [" << domain_name << "]";
            this->release_policy_dict(product_policy_map);
            return result;
        }
    }

    this->release_policy_dict(product_policy_map);
    return nullptr;
}

ProductPolicyMap* PolicyManager::load_policy_dict() {
    ProductPolicyMap* product_policy_map = new ProductPolicyMap();
    // 10: bucket_count, initial count of buckets, big enough to avoid resize.
    // 80: load_factor, element_count * 100 / bucket_count.
    product_policy_map->init(10, 80);

    FILE* fp = fopen(this->_conf_file_path.c_str(), "r");
    if (fp == nullptr) {
        APP_LOG(ERROR) << "Failed to open file " << this->_conf_file_path;
        this->destroy_policy_dict(product_policy_map);
        return nullptr;
    }
    char read_buffer[1024];
    rapidjson::FileReadStream is(fp, read_buffer, sizeof(read_buffer));
    rapidjson::Document doc;
    doc.ParseStream(is);
    fclose(fp);

    if (doc.HasParseError() || !doc.IsObject()) {
        APP_LOG(ERROR) << "Failed to parse RemoteServiceManager settings";
        this->destroy_policy_dict(product_policy_map);
        return nullptr;
    }

    for (rapidjson::Value::ConstMemberIterator prod_iter = doc.MemberBegin();
            prod_iter != doc.MemberEnd(); ++prod_iter) {
        std::string prod_name = prod_iter->name.GetString();
        if (!prod_iter->value.IsObject()) {
            APP_LOG(ERROR) << "Invalid product conf for " << prod_name;
            this->destroy_policy_dict(product_policy_map);
            return nullptr;
        }
        DomainPolicyMap* domain_policy_map = this->load_domain_policy_map(prod_name, prod_iter->value);
        if (domain_policy_map == nullptr) {
            APP_LOG(ERROR) << "Failed to load policies for product " << prod_name;
            this->destroy_policy_dict(product_policy_map);
            return nullptr;
        }
        product_policy_map->insert(prod_name, domain_policy_map);
    }

    return product_policy_map;
}

void PolicyManager::destroy_policy_dict(ProductPolicyMap* policy_dict) {
    if (policy_dict == nullptr) {
        return;
    }
    for (ProductPolicyMap::iterator iter = policy_dict->begin();
            iter != policy_dict->end(); ++iter) {
        auto domain_policy_map = iter->second;
        if (domain_policy_map == nullptr) {
            continue;
        }
        for (DomainPolicyMap::iterator iter2 = domain_policy_map->begin();
                iter2 != domain_policy_map->end(); ++iter2) {
            auto domain_policy = iter2->second;
            if (domain_policy == nullptr) {
                continue;
            }
            delete domain_policy;
            iter2->second = nullptr;
        }
        delete domain_policy_map;
        iter->second = nullptr;
    }
    delete policy_dict;
    policy_dict = nullptr;
}

ProductPolicyMap* PolicyManager::acquire_policy_dict() {
    std::lock_guard<std::mutex> lock(this->_policy_dict_mutex);
    this->_policy_dict_ref_count[this->_current_policy_dict]++;
    return this->_dual_policy_dict[this->_current_policy_dict];
}

void PolicyManager::release_policy_dict(ProductPolicyMap* policy_dict) {
    std::lock_guard<std::mutex> lock(this->_policy_dict_mutex);
    if (this->_dual_policy_dict[0] == policy_dict) {
        this->_policy_dict_ref_count[0]--;
    } else if (this->_dual_policy_dict[1] == policy_dict) {
        this->_policy_dict_ref_count[1]--;
    }
}

void PolicyManager::reload_thread_func() {
    while (!this->_destroyed) {
        std::string last_modified_time;
        get_file_last_modified_time(this->_conf_file_path, last_modified_time);
        if (last_modified_time != this->_conf_file_last_modified_time && this->reload() == 0) {
            LOG(TRACE) << "Policy conf last modify time: " << this->_conf_file_last_modified_time;
            this->_conf_file_last_modified_time = last_modified_time;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(POLICY_DICT_RELOAD_INTERVAL));
    }
}

DomainPolicyMap* PolicyManager::load_domain_policy_map(const std::string& product_name,
                                                        const rapidjson::Value& product_json) {
    DomainPolicyMap* domain_policy_map = new DomainPolicyMap();
    // 10: bucket_count, initial count of buckets, big enough to avoid resize.
    // 80: load_factor, element_count * 100 / bucket_count.
    domain_policy_map->init(10, 80);

    APP_LOG(TRACE) << "Loading policies for product: " << product_name;
    for (rapidjson::Value::ConstMemberIterator domain_iter = product_json.MemberBegin();
            domain_iter != product_json.MemberEnd(); ++domain_iter) {
        std::string domain_name = domain_iter->name.GetString();
        const rapidjson::Value& domain_json = domain_iter->value;
        rapidjson::Value::ConstMemberIterator setting_iter;
        setting_iter = domain_json.FindMember("score");
        if (setting_iter == domain_json.MemberEnd() || !setting_iter->value.IsInt()) {
            APP_LOG(WARNING) << "Failed to parse score for domain "
                << domain_name << " in product " << product_name << ", skipped";
            continue;
        }
        int score = setting_iter->value.GetInt();

        setting_iter = domain_json.FindMember("conf_path");
        if (setting_iter == domain_json.MemberEnd() || !setting_iter->value.IsString()) {
            APP_LOG(WARNING) << "Failed to parse conf_path for domain "
                << domain_name << " in product " << product_name << ", skipped";
            continue;
        }
        std::string conf_path = setting_iter->value.GetString();

        APP_LOG(TRACE) << "Loading policies for domain " << domain_name << " from " << conf_path;
        DomainPolicy* domain_policy = this->load_domain_policy(domain_name, score, conf_path);
        if (domain_policy == nullptr) {
            APP_LOG(WARNING) << "Failed to load policy for domain "
                << domain_name << " in product " << product_name << ", skipped";
            continue;
        }
        APP_LOG(TRACE) << "Loaded policies for domain " << domain_name;

        domain_policy_map->insert(domain_name, domain_policy);
    }

    return domain_policy_map;
}

DomainPolicy* PolicyManager::load_domain_policy(const std::string& domain_name,
                                                int score,
                                                const std::string& conf_path) {
    FILE* fp = fopen(conf_path.c_str(), "r");
    if (fp == nullptr) {
        APP_LOG(ERROR) << "Failed to open file " << conf_path;
        return nullptr;
    }
    char read_buffer[1024];
    rapidjson::FileReadStream is(fp, read_buffer, sizeof(read_buffer));
    rapidjson::Document doc;
    doc.ParseStream(is);
    fclose(fp);
    if (doc.HasParseError() || !doc.IsArray()) {
        APP_LOG(ERROR) << "Failed to parse domain conf " << conf_path;
        return nullptr;
    }
    IntentPolicyMap* intent_policy_map = new IntentPolicyMap();
    // 10: bucket_count, initial count of buckets, big enough to avoid resize.
    // 80: load_factor, element_count * 100 / bucket_count.
    intent_policy_map->init(10, 80);
    for (rapidjson::Value::ConstValueIterator policy_iter = doc.Begin();
            policy_iter != doc.End(); ++policy_iter) {
        APP_LOG(TRACE) << "loading policy...";
        Policy* policy = Policy::parse_from_json_value(*policy_iter);
        if (policy == nullptr) {
            APP_LOG(WARNING) << "Found invalid policy conf in path " << conf_path << ", skipped";
            continue;
        }
        const std::string& trigger_intent = policy->trigger().intent;
        if (intent_policy_map->seek(trigger_intent) == nullptr) {
            intent_policy_map->insert(trigger_intent, new PolicyVector);
        }
        (*intent_policy_map)[trigger_intent]->push_back(policy);
    }
    APP_LOG(TRACE) << "initializing domain policy...";
    DomainPolicy* domain_policy = new DomainPolicy(domain_name, score, intent_policy_map);
    APP_LOG(TRACE) << "finish initializing domain policy...";
    return domain_policy;
}

// When multiple policies satisfy the current intent,
// the following strategy is applies to find the best one:
// 1. The policies with none empty trigger state, it should match current DM state.
// 2. Policy with maximum number of matched trigger slot is ranked top.
static Policy* find_best_policy_from_candidates(PolicyVector& policy_vector,
                                                std::string& state,
                                                std::unordered_multiset<std::string>& qu_slot_set) {
    Policy* policy_result = nullptr;
    for (auto const& policy: policy_vector) {
        if (!policy->trigger().state.empty() && policy->trigger().state != state) {
            continue;
        }

        bool missing_slot = false;
        std::unordered_multiset<std::string> trigger_slot_set;
        for (auto const& slot: policy->trigger().slots) {
            trigger_slot_set.insert(slot);
        }
        for (auto iter = trigger_slot_set.begin(); iter != trigger_slot_set.end();) {
            int trigger_slot_cnt = trigger_slot_set.count(*iter);
            int qu_slot_cnt = qu_slot_set.count(*iter);
            if (qu_slot_cnt < trigger_slot_cnt) {
                missing_slot = true;
                break;
            }
            std::advance(iter, trigger_slot_cnt);
        }
        if (missing_slot) {
            continue;
        }

        if (policy_result == nullptr) {
            policy_result = policy;
            continue;
        }

        if (!policy_result->trigger().state.empty() && policy->trigger().state.empty()) {
            continue;
        }

        if (policy_result->trigger().state.empty() && !policy->trigger().state.empty()) {
            policy_result = policy;
            continue;
        }

        if (policy_result->trigger().slots.size() < policy->trigger().slots.size()) {
            policy_result = policy;
        }
    }

    return policy_result;
}

Policy* PolicyManager::find_best_policy(DomainPolicy* domain_policy,
                                        QuResult* qu_result,
                                        const PolicyOutputSession& session,
                                        const RequestContext& context) {
    (void) context;

    std::string state;
    if (domain_policy->name() == session.domain) {
        state = session.state;
    }

    std::unordered_multiset<std::string> qu_slot_set;
    IntentPolicyMap* intent_policy_map = domain_policy->intent_policy_map();
    PolicyVector** policy_vector_seek = nullptr;
    Policy* policy_result = nullptr;

    for (auto const& slot: qu_result->slots()) {
        qu_slot_set.insert(slot.key());
    }
    // Policy with matching intent.
    const std::string& intent = qu_result->intent();
    policy_vector_seek = intent_policy_map->seek(intent);
    if (policy_vector_seek != nullptr) {
        APP_LOG(TRACE) << "intent [" << intent << "] candidate count [" << (*policy_vector_seek)->size() << "]";
        policy_result = find_best_policy_from_candidates(**policy_vector_seek, state, qu_slot_set);
    }

    // Fallback policy when none of the policies match intent.
    if (policy_result == nullptr) {
        const std::string fallback_intent = "system@reserved@domain_fallback_intent";
        policy_vector_seek = intent_policy_map->seek(fallback_intent);
        if (policy_vector_seek != nullptr) {
            APP_LOG(TRACE) << "system@reserved@domain_fallback_intent candidate count [" << (*policy_vector_seek)->size() << "]";
            policy_result = find_best_policy_from_candidates(**policy_vector_seek, state, qu_slot_set);
        }
    }

    return policy_result;
}

// Resolve a string with params in it.
static bool try_resolve_params(std::string& unresolved,
                              const std::unordered_map<std::string, std::string>& param_map) {

    if (unresolved.empty()) {
        return true;
    }

    std::string resolved;
    bool is_param = false;
    unsigned int last_index = 0;
    for (unsigned int i = 0; i < unresolved.length(); ++i) {
        if (unresolved[i] == '{' && i + 1 < unresolved.length() && unresolved[i + 1] == '%' ) {
            resolved += unresolved.substr(last_index, i - last_index);
            last_index = i + 2;
            is_param = true;
            ++i;
        }
        if (unresolved[i] == '%' && i + 1 < unresolved.length() && unresolved[i + 1] == '}') {
            if (!is_param) {
                APP_LOG(WARNING) << "Cannot resolve params in string, invalid format. " << unresolved;
                return false;
            }
            std::string param_name = unresolved.substr(last_index, i - last_index);
            std::unordered_map<std::string, std::string>::const_iterator find_res = param_map.find(param_name);
            if (find_res == param_map.end()) {
                APP_LOG(WARNING) << "Cannot resolve params in string, unknow param. "
                    << unresolved << " " << param_name;
                return false;
            }
            resolved += find_res->second;
            last_index = i + 2;
            is_param = false;
            ++i;
        }
    }
    if (is_param) {
        APP_LOG(WARNING) << "Cannot resolve params in string, invalid format. " << unresolved;
        return false;
    }
    if (last_index < unresolved.length()) {
        resolved += unresolved.substr(last_index);
    }

    unresolved = resolved;
    return true;
}

// Resolve a string with delimiter and params in it
static bool try_resolve_param_list(const std::string& unresolved,
                                   const char delimiter,
                                   const std::unordered_map<std::string, std::string>& param_map,
                                   std::vector<std::string> &result) {
    std::size_t pos = 0;
    std::size_t last_pos = 0;
    result.clear();
    while (last_pos < unresolved.length() && (pos = unresolved.find(delimiter, last_pos)) != std::string::npos) {
        std::string part = unresolved.substr(last_pos, pos - last_pos);
        utils::trim(part);
        if (!try_resolve_params(part, param_map)) {
            result.clear();
            return false;
        }
        result.push_back(part);
        last_pos = pos + 1;
    }
    if (last_pos < unresolved.length()) {
        std::string part = unresolved.substr(last_pos);
        utils::trim(part);
        if (!try_resolve_params(part, param_map)) {
            result.clear();
            return false;
        }
        result.push_back(part);
    }

    return true;
}

PolicyOutput* PolicyManager::resolve_policy_output(const std::string& domain,
                                                   Policy* policy,
                                                   QuResult* qu_result,
                                                   const PolicyOutputSession& session,
                                                   const RequestContext& context) {
    // Process parameters
    std::unordered_map<std::string, std::string> param_map;
    for (auto const& param: policy->params()) {
        APP_LOG(TRACE) << "resolving parameter [" << param.name << "]";
        std::string value;
        if (param.type == "slot_val" || param.type == "slot_val_ori") {
            bool success = false;
            std::vector<std::string> args;
            utils::split(param.value, ',', args);
            int index = 0;
            if (args.size() >= 2 && !utils::try_atoi(args[1], index)) {
                APP_LOG(WARNING) << "Invalid index for slot_val parameter: " << param.value;
                index = -1;
            }
            for (auto const& slot: qu_result->slots()) {
                if (index < 0) {
                    break;
                }
                if (slot.key() == args[0]) {
                    if (index > 0) {
                        index--;
                        continue;
                    }
                    if (param.type == "slot_val" && !slot.normalized_value().empty()) {
                        value = slot.normalized_value();
                        success = true;
                        break;
                    }
                    value = slot.value();
                    success = true;
                    break;
                }
            }
            if (!success) {
                if (param.required) {
                    return nullptr;
                }
                value = param.default_value;
            }
        } else if (param.type == "qu_intent") {
            value = qu_result->intent();
        } else if (param.type == "session_state") {
            value = session.state;
        } else if (param.type == "session_context") {
            bool success = false;
            for (auto const& obj: session.context) {
                if (obj.key == param.value) {
                    value = obj.value;
                    success = true;
                    break;
                }
            }
            if (!success) {
                if (param.required) {
                    return nullptr;
                }
                value = param.default_value;
            }
        } else if (param.type == "const_val") {
            value = param.value;
        } else if (param.type == "request_param") {
            const std::unordered_map<std::string, std::string> request_params = context.params();
            std::unordered_map<std::string, std::string>::const_iterator find_res
                = request_params.find(param.value);
            bool success = false;
            if (find_res != request_params.end()) {
                value = find_res->second;
                success = true;
            }
            if (!success) {
                if (param.required) {
                    return nullptr;
                }
                value = param.default_value;
            }
        } else if (param.type == "func_val") {
            std::string func_val = param.value;
            std::string func_name;
            std::vector<std::string> args;
            std::size_t pos = func_val.find(':');
            bool has_error = false;
            if (pos == std::string::npos || pos == func_val.length() - 1) {
                func_name = func_val;
                if (!try_resolve_params(func_name, param_map)) {
                    has_error = true;
                }
            } else {
                func_name = func_val.substr(0, pos);
                if (!try_resolve_params(func_name, param_map)) {
                    has_error = true;;
                }
                std::string arg_list = func_val.substr(pos + 1);
                if (!try_resolve_param_list(arg_list, ',', param_map, args)) {
                    has_error = true;
                }
            }
            utils::trim(func_name);
            if (has_error || this->_user_function_manager->call_user_function(func_name, args, context, value) != 0) {
                has_error = true;
            }
            if (has_error) {
                if (param.required) {
                    return nullptr;
                }
                value = param.default_value;
            }
        } else {
            APP_LOG(WARNING) << "Unknown param type " << param.type;
            if (param.required) {
                return nullptr;
            }
            value = param.default_value;
        }
        param_map[param.name] =  value;
        APP_LOG(TRACE) << "Parameter value [" << value << "]";
    }

    int selected_output_index = -1;
    for (unsigned int i = 0; i < policy->outputs().size(); ++i) {
        bool failed = false;
        // Process assertions
        APP_LOG(TRACE) << "Candidate output size [" << policy->outputs().size() << "]";
        for (unsigned int j = 0; j < policy->outputs()[i].assertions.size(); ++j) {
            std::string assertion_type = policy->outputs()[i].assertions[j].type;
            std::string assertion_value = policy->outputs()[i].assertions[j].value;
            if (!try_resolve_params(assertion_value, param_map)) {
                failed = true;
                break;
            }
            APP_LOG(TRACE) << "evaluating assertion, type[" << assertion_type << "] value[" << assertion_value << "]";
            if (assertion_type == "not_empty") {
                if (assertion_value.empty()) {
                    failed = true;
                    break;
                }
            } else if (assertion_type == "empty") {
                if (!assertion_value.empty()) {
                    failed = true;
                    break;
                }
            } else if (assertion_type == "in") {
                std::vector<std::string> value_list;
                bool has_match = false;
                if (try_resolve_param_list(assertion_value, ',', param_map, value_list)) {
                    for (unsigned int k = 1; k < value_list.size(); k++) {
                        if (value_list[0] == value_list[k]) {
                            has_match = true;
                            break;
                        }
                    }
                }
                if (!has_match) {
                    failed = true;
                }
            } else if (assertion_type == "not_in") {
                std::vector<std::string> value_list;
                if (try_resolve_param_list(assertion_value, ',', param_map, value_list)) {
                    for (unsigned int k = 1; k < value_list.size(); k++) {
                        if (value_list[0] == value_list[k]) {
                            failed = true;
                            break;
                        }
                    }
                }
            } else if (assertion_type == "eq") {
                std::vector<std::string> value_list;
                if (!try_resolve_param_list(assertion_value, ',', param_map, value_list)
                    || value_list.size() < 2
                    || value_list[0] != value_list[1]) {
                    failed = true;
                }
            } else if (assertion_type == "gt") {
                std::vector<std::string> value_list;
                double left_val = 0;
                double right_val = 0;
                if (!try_resolve_param_list(assertion_value, ',', param_map, value_list)
                    || value_list.size() < 2
                    || !utils::try_atof(value_list[0], left_val)
                    || !utils::try_atof(value_list[1], right_val)
                    || left_val <= right_val) {
                    failed  = true;
                }
            } else if (assertion_type == "ge") {
                std::vector<std::string> value_list;
                double left_val = 0;
                double right_val = 0;
                if (!try_resolve_param_list(assertion_value, ',', param_map, value_list)
                    || value_list.size() < 2
                    || !utils::try_atof(value_list[0], left_val)
                    || !utils::try_atof(value_list[1], right_val)
                    || left_val < right_val) {
                    failed  = true;
                }
            } else {
                APP_LOG(WARNING) << "Unknown assertion type " << assertion_type;
                failed = true;
                break;
            }
        }
        if (!failed) {
            LOG(TRACE) << "selected output at index [" << i << "]";
            selected_output_index = i;
            break;
        }
    }
    if (selected_output_index == -1) {
        return nullptr;
    }

    PolicyOutput output;
    output.meta = policy->outputs()[selected_output_index].meta;
    output.session = policy->outputs()[selected_output_index].session;
    output.qu = policy->outputs()[selected_output_index].qu;
    output.results = policy->outputs()[selected_output_index].results;

    for (unsigned int i = 0; i < output.meta.size(); ++i) {
        if (!try_resolve_params(output.meta[i].key, param_map)) {
            return nullptr;
        }
        if (!try_resolve_params(output.meta[i].value, param_map)) {
            return nullptr;
        }
    }
    if (!try_resolve_params(output.session.domain, param_map)) {
        return nullptr;
    }
    if (!try_resolve_params(output.session.state, param_map)) {
        return nullptr;
    }
    for (unsigned int i = 0; i < output.session.context.size(); ++i) {
        if (!try_resolve_params(output.session.context[i].key, param_map)) {
            return nullptr;
        }
        if (!try_resolve_params(output.session.context[i].value, param_map)) {
            return nullptr;
        }
    }
    if (!try_resolve_params(output.qu.domain, param_map)) {
        return nullptr;
    }
    if (!try_resolve_params(output.qu.intent, param_map)) {
        return nullptr;
    }
    for (unsigned int i = 0; i < output.qu.slots.size(); ++i) {
        if (!try_resolve_params(output.qu.slots[i].key, param_map)) {
            return nullptr;
        }
        if (!try_resolve_params(output.qu.slots[i].value, param_map)) {
            return nullptr;
        }
        if (!try_resolve_params(output.qu.slots[i].normalized_value, param_map)) {
            return nullptr;
        }
    }
    for (unsigned int i = 0; i < output.results.size(); ++i) {
        if (output.results[i].values.empty()) {
            APP_LOG(WARNING) << "empty result value!";
            return nullptr;
        }
        if (output.results[i].values.size() > 1) {
            int size = output.results[i].values.size();
            int index = std::time(nullptr) % size;
            if (index < 0 || index >= size) {
                index = 0;
            }
            std::string value = output.results[i].values[index];
            output.results[i].values.clear();
            output.results[i].values.push_back(value);
        }
        if (!try_resolve_params(output.results[i].values[0], param_map)) {
            return nullptr;
        }
    }

    PolicyOutput* output_ptr = new PolicyOutput();
    *output_ptr = output;
    output_ptr->session.domain = domain;

    return output_ptr;
}

} // namespace dmkit
