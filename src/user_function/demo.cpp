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

#include "demo.h"
#include "../utils.h"

namespace dmkit {
namespace user_function {
namespace demo {

// These functions are for demo purpose which returns mock data.
// In real application, these might need to invoke a remote service call.
// Http GET/POST calls are already included as shared function.

int get_cellular_data_usage(const std::vector<std::string>& args,
        const RequestContext& context, std::string& result) {
    (void)context;
    if (args.size() < 1) {
        return -1;
    }

    std::vector<std::string> elements;
    utils::split(args[0], '-', elements);
    if (elements.size() > 1 && elements[1] == "01") {
        result = "0";
    } else {
        result = "2";
    }
    
    return 0;
}

int get_cellular_data_left(const std::vector<std::string>& args,
        const RequestContext& context, std::string& result) {
    (void)context;
    if (args.size() < 1) {
        return -1;
    }

    std::vector<std::string> elements;
    utils::split(args[0], '-', elements);
    if (elements.size() > 1 && elements[1] == "01") {
        result = "2";
    } else {
        result = "0";
    }
    
    return 0;
}

int get_package_options(const std::vector<std::string>& args,
        const RequestContext& context, std::string& result) {
    (void)context;
    if (args.size() < 1) {
        return -1;
    }

    if (args[0] == "省内流量包") {
        result = "10元100M，20元300M";
    } else if (args[0] == "全国流量包") {
        result = "10元100M，50元1G";
    } else {
        result = "20元300M，50元1G";
    }

    return 0;
}

} // namespace demo
} // namespace user_function
} // namespace dmkit
