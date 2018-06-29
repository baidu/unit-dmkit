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

#include "request_context.h"

namespace dmkit {

RequestContext::RequestContext(RemoteServiceManager* remote_service_manager,
                               const std::string& qid,
                               const std::unordered_map<std::string, std::string>& params)
    : _remote_service_manager(remote_service_manager), _qid(qid), _params(params) {

}

RequestContext::~RequestContext() {
}

const RemoteServiceManager* RequestContext::remote_service_manager() const {
    return _remote_service_manager;
}

const std::string& RequestContext::qid() const {
    return _qid;
}

const std::unordered_map<std::string, std::string>& RequestContext::params() const {
    return _params;
}

bool RequestContext::set_param_value(const std::string& param_name, const std::string& value) {
    this->_params[param_name] = value;
    return true;
}

bool RequestContext::try_get_param(const std::string& param_name, std::string& value) const {
    value.clear();
    auto search = this->_params.find(param_name);
    if (search == this->_params.end()) {
        return false;
    }

    value = search->second;
    return true;
}

} // namespace dmkit
