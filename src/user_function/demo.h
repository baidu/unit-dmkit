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

#ifndef DMKIT_USER_FUNCTION_DEMO_H
#define DMKIT_USER_FUNCTION_DEMO_H

#include <string>
#include <vector>
#include "../request_context.h"

namespace dmkit {
namespace user_function {
namespace demo {

// Functions used in our demo scenario
int get_cellular_data_usage(const std::vector<std::string>& args,
                            const RequestContext& context,
                            std::string& result);

int get_cellular_data_left(const std::vector<std::string>& args,
                            const RequestContext& context,
                            std::string& result);

int get_package_options(const std::vector<std::string>& args,
                            const RequestContext& context,
                            std::string& result);

} // namespace demo
} // namespace user_function
} // namespace dmkit

#endif  //DMKIT_USER_FUNCTION_DEMO_H

