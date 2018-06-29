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

#ifndef DMKIT_USER_FUNCTION_MANAGER_H
#define DMKIT_USER_FUNCTION_MANAGER_H

#include <string>
#include <unordered_map>
#include <vector>
#include "request_context.h"

namespace dmkit {

typedef int (*UserFunctionPtr)(const std::vector<std::string>& args,
                               const RequestContext& context,
                               std::string& result);

class UserFunctionManager {
public:
    UserFunctionManager();
    ~UserFunctionManager();

    int init();
    int call_user_function(const std::string& func_name,
                           const std::vector<std::string>& args,
                           const RequestContext& context,
                           std::string& result);

private:
    std::unordered_map<std::string, UserFunctionPtr>* _user_function_map;
    bool _init;
};

} // namespace dmkit

#endif  //DMKIT_USER_FUNCTION_MANAGER_H
