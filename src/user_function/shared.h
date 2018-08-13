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

#ifndef DMKIT_USER_FUNCTION_SHARED_H
#define DMKIT_USER_FUNCTION_SHARED_H

#include <string>
#include <vector>
#include "../request_context.h"

namespace dmkit {
namespace user_function {

// JSON Operations
int json_get_value(const std::vector<std::string>& args,
                   const RequestContext& context,
                   std::string& result);

// String Operations
int replace(const std::vector<std::string>& args,
            const RequestContext& context,
            std::string& result);

// Arithmetic Operations
int number_add(const std::vector<std::string>& args,
               const RequestContext& context,
               std::string& result);

int float_mul(const std::vector<std::string>& args,
              const RequestContext& context,
              std::string& result);

// Logical Operations
int choose_if_equal(const std::vector<std::string>& args,
                    const RequestContext& context,
                    std::string& result);

// Network Operations
int url_encode(const std::vector<std::string>& args,
               const RequestContext& context,
               std::string& result);

int service_http_get(const std::vector<std::string>& args,
                     const RequestContext& context,
                     std::string& result);

int service_http_post(const std::vector<std::string>& args,
                      const RequestContext& context,
                      std::string& result);

// Time Related
int now_strftime(const std::vector<std::string>& args,
                 const RequestContext& context,
                 std::string& result);

} // namespace user_function
} // namespace dmkit

#endif  //DMKIT_USER_FUNCTION_SHARED_H

