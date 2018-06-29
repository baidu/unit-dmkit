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

#ifndef DMKIT_REQUEST_CONTEXT_H
#define DMKIT_REQUEST_CONTEXT_H

#include <string>
#include <unordered_map>
#include "remote_service_manager.h"

namespace dmkit {

// Request context for user functions to access,
// including request parameters and an remote_service_manager instance
class RequestContext {
public:
    RequestContext(RemoteServiceManager* remote_service_manager,
                   const std::string& qid,
                   const std::unordered_map<std::string, std::string>& params);
    ~RequestContext();

    const RemoteServiceManager* remote_service_manager() const;
    const std::string& qid() const;
    const std::unordered_map<std::string, std::string>& params() const;
    bool set_param_value(const std::string& param_name, const std::string& value);
    bool try_get_param(const std::string& param_name, std::string& value) const;

private:
    RemoteServiceManager* _remote_service_manager;
    std::string _qid;
    std::unordered_map<std::string, std::string> _params;
};

} // namespace dmkit

#endif  //DMKIT_THREAD_DATA_BASE_H
