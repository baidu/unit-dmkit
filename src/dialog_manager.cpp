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

#include "dialog_manager.h"
#include <memory>
#include <string>
#include <butil/containers/flat_map.h>
#include <butil/iobuf.h>
#include "app_log.h"
#include "rapidjson.h"
#include "request_context.h"
#include "utils.h"

namespace dmkit {

DialogManager::DialogManager() {
    this->_remote_service_manager = new RemoteServiceManager();
    this->_policy_manager = new PolicyManager();
}

DialogManager::~DialogManager() {
    delete this->_policy_manager;
    this->_policy_manager = nullptr;
    delete this->_remote_service_manager;
    this->_remote_service_manager = nullptr;
}

int DialogManager::init() {
    if (0 != this->_remote_service_manager->init("conf/app", "remote_services.json")) {
        APP_LOG(ERROR) << "Failed to init _remote_service_manager";
        return -1;
    }
    APP_LOG(TRACE) << "_remote_service_manager init done";

    if (0 != this->_policy_manager->init("conf/app", "products.json")) {
        APP_LOG(ERROR) << "Failed to init _policy_manager";
        return -1;
    }
    APP_LOG(TRACE) << "_policy_manager init done";

    return 0;
}

int DialogManager::run(brpc::Controller* cntl) {
    std::string request_json = cntl->request_attachment().to_string();
    APP_LOG(TRACE) << "received request: " << request_json;
    //this->add_notice_log("req", request_json);

    rapidjson::Document request_doc;
    // In the case we cannot parse the request json, it is not a valid request.
    if (request_doc.Parse(request_json.c_str()).HasParseError() || !request_doc.IsObject()) {
        APP_LOG(WARNING) << "Failed to parse request data to json";
        cntl->http_response().set_status_code(400);
        return 0;
    }

    // Need to set log_id as soon as we can get it to avoid missing log_id in logs.
    if (!request_doc.HasMember("log_id") || !request_doc["log_id"].IsString()) {
        APP_LOG(WARNING) << "Missing log_id";
        this->send_error_response(cntl, -1, "Missing log_id");
        return 0;
    }
    std::string log_id = request_doc["log_id"].GetString();
    std::string dmkit_log_id_prefix = "dmkit_";
    log_id = dmkit_log_id_prefix + log_id;
    this->set_log_id(log_id);
    request_doc["log_id"].SetString(log_id.c_str(), log_id.length(), request_doc.GetAllocator());

    // Get access_token from request uri.
    std::string access_token;
    const std::string* access_token_ptr = cntl->http_request().uri().GetQuery("access_token");
    if (access_token_ptr != nullptr) {
        access_token = *access_token_ptr;
    }

    request_json = utils::json_to_string(request_doc);
    // Call unit bot api with the request json as dmkit use the same data contract.
    std::string unit_bot_result;
    if (this->call_unit_bot(access_token, request_json, unit_bot_result) != 0) {
        APP_LOG(ERROR) << "Failed to call unit bot api";
        this->send_error_response(cntl, -1, "Failed to call unit bot api");
        return 0;
    }
    APP_LOG(TRACE) << "unit bot result: " << unit_bot_result;

    // Parse unit bot response.
    // In the case something wrong with unit bot response, informs users.
    rapidjson::Document unit_response_doc;
    if (unit_response_doc.Parse(unit_bot_result.c_str()).HasParseError() 
            || !unit_response_doc.IsObject()) {
        APP_LOG(ERROR) << "Failed to parse unit bot result: " << unit_bot_result;
        this->send_error_response(cntl, -1, "Failed to parse unit bot result");
        return 0;
    }
    if (!unit_response_doc.HasMember("error_code")
            || !unit_response_doc["error_code"].IsInt() 
            || unit_response_doc["error_code"].GetInt() != 0) {
        this->send_json_response(cntl, unit_bot_result);
        return 0;
    }
    
    // Get dmkit session from request. We saved it in the bot session in latest response.
    std::string dm_session;
    if (request_doc.HasMember("bot_session")) {
        std::string request_bot_session = request_doc["bot_session"].GetString();
        rapidjson::Document request_bot_session_doc;
        if (!request_bot_session_doc.Parse(request_bot_session.c_str()).HasParseError() 
                && request_bot_session_doc.IsObject() 
                && request_bot_session_doc.HasMember("dialog_state")
                && request_bot_session_doc["dialog_state"].HasMember("contexts")
                && request_bot_session_doc["dialog_state"]["contexts"].HasMember("dm_session")) {
            dm_session = 
                request_bot_session_doc["dialog_state"]["contexts"]["dm_session"].GetString();
        }
    }
    APP_LOG(TRACE) << "dm session: " << dm_session;
    PolicyOutputSession session = PolicyOutputSession::from_json_str(dm_session);

    // The bot status is included in bot_session
    std::string bot_session = unit_response_doc["result"]["bot_session"].GetString();
    rapidjson::Document bot_session_doc;
    if (bot_session_doc.Parse(bot_session.c_str()).HasParseError()
            || !bot_session_doc.IsObject()) {
        APP_LOG(ERROR) << "Failed to parse bot session: " << bot_session;
        this->send_error_response(cntl, -1, "Failed to parse bot session");
        return 0;
    }
    if (this->handle_unsatisfied_intent(cntl, unit_response_doc,
            bot_session_doc, dm_session) == 0) {
        return 0;
    }

    // Handle satify/understood intents
    std::string bot_id = unit_response_doc["result"]["bot_id"].GetString();
    QuResult* qu_result = QuResult::parse_from_dialog_state(
        bot_id, bot_session_doc["dialog_state"]);
    if (qu_result == nullptr) {
        this->send_error_response(cntl, -1, "Failed to parse qu_result");
        return 0;
    }
    auto qu_map = new butil::FlatMap<std::string, QuResult*>();
    // 2: bucket_count, initial count of buckets, big enough to avoid resize.
    // 50: load_factor, element_count * 100 / bucket_count.
    qu_map->init(2, 50);
    qu_map->insert(bot_id, qu_result);
    // Parsing request params from client_session, only string value is accepted
    std::unordered_map<std::string, std::string> request_params;
    if (request_doc["request"].HasMember("client_session")) {
        std::string client_session = request_doc["request"]["client_session"].GetString();
        rapidjson::Document client_session_doc;
        if (!client_session_doc.Parse(client_session.c_str()).HasParseError() && client_session_doc.IsObject()) {
            for (auto& m_param: client_session_doc.GetObject()) {
                if (!m_param.value.IsString()) {
                    continue;
                }
                std::string param_name = m_param.name.GetString();
                std::string param_value = m_param.value.GetString();
                request_params[param_name] = param_value;
            }
        }
    }
    RequestContext context(this->_remote_service_manager, log_id, request_params);
    PolicyOutput* policy_output = this->_policy_manager->resolve(
        "default", qu_map, session, context);
    for (auto iter = qu_map->begin(); iter != qu_map->end(); ++iter) {
        QuResult* qu_ptr = iter->second;
        if (qu_ptr != nullptr) {
            delete qu_ptr;
        }
    }
    delete qu_map;

    if (policy_output == nullptr) {
        this->send_error_response(cntl, -1, "DM policy resolve failed");
        return 0;
    }
    this->set_dm_response(unit_response_doc, bot_session_doc, policy_output);
    delete policy_output;
    this->send_json_response(cntl, utils::json_to_string(unit_response_doc));
    return 0;
}

int DialogManager::call_unit_bot(const std::string& access_token, 
                                         const std::string& payload,
                                         std::string& result) {
    std::string url = "https://aip.baidubce.com/rpc/2.0/unit/bot/chat?access_token=";
    url += access_token;
    RemoteServiceParam rsp = {
        url,
        HTTP_METHOD_POST,
        payload
    };
    RemoteServiceResult rsr;
    // unit_bot is a remote service configured in conf/app/remote_services.json
    if (this->_remote_service_manager->call("unit_bot", rsp, rsr) !=0) {
        APP_LOG(ERROR) << "Failed to get unit bot result" ;
        return -1;
    }
    APP_LOG(TRACE) << "Got unit bot result";

    result = rsr.result;

    return 0;
}

int DialogManager::handle_unsatisfied_intent(brpc::Controller* cntl,
                                             rapidjson::Document& unit_response_doc,
                                             rapidjson::Document& bot_session_doc,
                                             const std::string& dm_session) {
    std::string action_type = unit_response_doc["result"]["response"]["action_list"][0]["type"].GetString();
    if (action_type != "satisfy" && action_type != "understood") {
        // DM session should be saved
        rapidjson::Value dm_session_json;
        dm_session_json.SetString(
            dm_session.c_str(), dm_session.length(), bot_session_doc.GetAllocator());
        bot_session_doc["dialog_state"]["contexts"].AddMember(
            "dm_session", dm_session_json, bot_session_doc.GetAllocator());

        std::string bot_session = utils::json_to_string(bot_session_doc);
        unit_response_doc.AddMember("debug", bot_session_doc, unit_response_doc.GetAllocator());
        unit_response_doc["result"]["bot_session"].SetString(
            bot_session.c_str(), bot_session.length(), unit_response_doc.GetAllocator());

        this->send_json_response(cntl, utils::json_to_string(unit_response_doc));
        return 0;
    }
    
    return -1;
}

void DialogManager::set_dm_response(rapidjson::Document& unit_response_doc,
                                            rapidjson::Document& bot_session_doc,
                                            const PolicyOutput* policy_output) {
    std::string session_str = PolicyOutputSession::to_json_str(policy_output->session);
    rapidjson::Value dm_session;
    dm_session.SetString(session_str.c_str(), session_str.length(), bot_session_doc.GetAllocator());
    bot_session_doc["dialog_state"]["contexts"].AddMember(
        "dm_session", dm_session, bot_session_doc.GetAllocator());
    // DMKit result as a custom reply
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    writer.StartObject();
    writer.Key("event_name");
    writer.String("DM_RESULT");
    writer.Key("result");
    std::string policy_output_str = PolicyOutput::to_json_str(*policy_output);
    writer.String(policy_output_str.c_str(), policy_output_str.length());
    writer.EndObject();
    std::string custom_reply = buffer.GetString();
    APP_LOG(TRACE) << "custom_reply: " << custom_reply;

    bot_session_doc["interactions"][0]["response"]["action_list"][0]["type"] = "event";
    bot_session_doc["interactions"][0]["response"]["action_list"][0]["say"] = "";
    bot_session_doc["interactions"][0]["response"]["action_list"][0]["custom_reply"].SetString(
        custom_reply.c_str(), custom_reply.length(), unit_response_doc.GetAllocator());

    unit_response_doc["result"]["response"]["action_list"][0]["type"] = "event";
    unit_response_doc["result"]["response"]["action_list"][0]["say"] = "";
    unit_response_doc["result"]["response"]["action_list"][0]["custom_reply"].SetString(
        custom_reply.c_str(), custom_reply.length(), unit_response_doc.GetAllocator());

    std::string bot_session = utils::json_to_string(bot_session_doc);
    unit_response_doc.AddMember("debug", bot_session_doc, unit_response_doc.GetAllocator());
    unit_response_doc["result"]["bot_session"].SetString(
        bot_session.c_str(), bot_session.length(), unit_response_doc.GetAllocator());
}

void DialogManager::send_error_response(brpc::Controller* cntl,
                                                int error_code,
                                                const std::string& error_msg) {
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    writer.StartObject();
    writer.Key("error_code");
    writer.Int(error_code);
    writer.Key("error_msg");
    writer.String(error_msg.c_str(), error_msg.length());
    writer.EndObject();

    this->send_json_response(cntl, buffer.GetString());
}

void DialogManager::send_json_response(brpc::Controller* cntl,
                                       const std::string& data) {
    cntl->http_response().set_content_type("application/json;charset=UTF-8");
    //this->add_notice_log("ret", data);
    cntl->response_attachment().append(data);
}

} // namespace dmkit

