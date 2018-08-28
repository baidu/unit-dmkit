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

#include "application_base.h"
#include "policy.h"
#include "policy_manager.h"
#include "qu_result.h"
#include "rapidjson.h"
#include "remote_service_manager.h"
#include "token_manager.h"

#ifndef DMKIT_DIALOG_MANAGER_H
#define DMKIT_DIALOG_MANAGER_H

namespace dmkit {

// The Dialog Manager application
class DialogManager : public ApplicationBase {
public:
    DialogManager();
    virtual ~DialogManager();
    virtual int init();
    virtual int run(BRPC_NAMESPACE::Controller* cntl);

private:
    int process_request(const rapidjson::Document& request_doc,
                        const std::string& dm_session,
                        const std::string& access_token,
                        std::string& json_response,
                        bool& is_dmkit_response);

    int call_unit_bot(const std::string& access_token,
                      const std::string& payload,
                      std::string& result);

    int handle_unsatisfied_intent(rapidjson::Document& unit_response_doc,
                                  rapidjson::Document& bot_session_doc,
                                  const std::string& dm_session,
                                  std::string& response);

    std::string get_error_response(int error_code, const std::string& error_msg);

    void send_json_response(BRPC_NAMESPACE::Controller* cntl, const std::string& data);

    void set_dm_response(rapidjson::Document& unit_response_doc,
                         rapidjson::Document& bot_session_doc,
                         const PolicyOutput* policy_output);

    RemoteServiceManager* _remote_service_manager;
    PolicyManager* _policy_manager;
    TokenManager* _token_manager;
};

} // namespace dmkit

#endif  //DMKIT_DIALOG_MANAGER_H
