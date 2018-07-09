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

#include <curl/curl.h>
#include "app_log.h"
#include "user_function_manager.h"
#include "user_function/shared.h"
#include "user_function/demo.h"

namespace dmkit {

UserFunctionManager::UserFunctionManager() {
    this->_user_function_map = new std::unordered_map<std::string, UserFunctionPtr>();
    this->_init = false;
}

UserFunctionManager::~UserFunctionManager() {
    delete this->_user_function_map;
    this->_user_function_map = nullptr;
    
    if (this->_init) {
        curl_global_cleanup();
        this->_init = false;
    }
}

int UserFunctionManager::init() {
    // All user functions should be registered here.
    // Shared functions.
    (*_user_function_map)["json_get_value"] = user_function::json_get_value;
    (*_user_function_map)["replace"] = user_function::replace;
    (*_user_function_map)["number_add"] = user_function::number_add;
    (*_user_function_map)["float_mul"] = user_function::float_mul;
    (*_user_function_map)["choose_if_equal"] = user_function::choose_if_equal;
    (*_user_function_map)["url_encode"] = user_function::url_encode;
    (*_user_function_map)["service_http_get"] = user_function::service_http_get;
    (*_user_function_map)["service_http_post"] = user_function::service_http_post;
    
    // Scenario specific functions.
    (*_user_function_map)["demo_get_cellular_data_usage"] = 
            user_function::demo::get_cellular_data_usage;
    (*_user_function_map)["demo_get_cellular_data_left"] = 
            user_function::demo::get_cellular_data_left;
    (*_user_function_map)["demo_get_package_options"] = user_function::demo::get_package_options;

    // Curl init is required for url_encode function.
    if (curl_global_init(CURL_GLOBAL_DEFAULT) != 0) {
        APP_LOG(WARNING) << "curl_global_init returns non-zeros";
        return -1;
    }

    this->_init = true;
    return 0;
}

int UserFunctionManager::call_user_function(const std::string& func_name,
                                            const std::vector<std::string>& args,
                                            const RequestContext& context,
                                            std::string& result) {
    auto find_res = this->_user_function_map->find(func_name);
    if (find_res == this->_user_function_map->end()) {
        APP_LOG(WARNING) << "call to undefined user function: " << func_name;
        return -1;
    }
    UserFunctionPtr func_ptr = find_res->second;
    int res = -1;
    try {
        res = (*func_ptr)(args, context, result);
    } catch (const char* msg) {
        APP_LOG(WARNING) << "Exception calling user function [" 
            << func_name << "], exception: " << msg;
        res = -1;
    }

    return res;
}

} // namespace dmkit
