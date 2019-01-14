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

#ifndef DMKIT_POLICY_MANAGER_H
#define DMKIT_POLICY_MANAGER_H

#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include "butil.h"
#include "policy.h"
#include "qu_result.h"
#include "request_context.h"
#include "user_function_manager.h"

namespace dmkit {

typedef std::vector<Policy*> PolicyVector;
typedef BUTIL_NAMESPACE::FlatMap<std::string, PolicyVector*> IntentPolicyMap;

// Holds all policies given a domain.
class DomainPolicy {
public:
    DomainPolicy(const std::string& name, int score, IntentPolicyMap* intent_policy_map);
    ~DomainPolicy();
    const std::string& name();
    int score();
    // Maps a intent to a vector of policies
    IntentPolicyMap* intent_policy_map();

private:
    std::string _name;
    int _score;
    IntentPolicyMap* _intent_policy_map;
};

typedef BUTIL_NAMESPACE::FlatMap<std::string, DomainPolicy*> DomainPolicyMap;
typedef BUTIL_NAMESPACE::FlatMap<std::string, DomainPolicyMap*> ProductPolicyMap;

class PolicyManager {
public:
    PolicyManager();
    ~PolicyManager();
    int init(const char* dir_path, const char* conf_file);
    int reload();
    // Resolve a policy output given a qu result and current dm session
    PolicyOutput* resolve(const std::string& product,
                          BUTIL_NAMESPACE::FlatMap<std::string, QuResult*>* qu_result,
                          const PolicyOutputSession& session,
                          const RequestContext& context);

    static int policy_conf_change_callback(void* param);

private:
    DomainPolicyMap* load_domain_policy_map(const std::string& product_name,
                                             const rapidjson::Value& product_json);

    DomainPolicy* load_domain_policy(const std::string& domain_name,
                                     int score,
                                     const std::string& conf_path);

    Policy* find_best_policy(DomainPolicy* domain_policy,
                             QuResult* qu_result,
                             const PolicyOutputSession& session,
                             const RequestContext& context);

    PolicyOutput* resolve_policy_output(const std::string& domain,
                                        Policy* policy,
                                        QuResult* qu_result,
                                        const PolicyOutputSession& session,
                                        const RequestContext& context);

    ProductPolicyMap* load_policy_dict();

    std::string _conf_file_path;
    std::shared_ptr<ProductPolicyMap> _p_policy_dict;

    UserFunctionManager* _user_function_manager;
};

} // namespace dmkit

#endif  //DMKIT_POLICY_MANAGER_H
