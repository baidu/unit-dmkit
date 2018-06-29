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

#include "shared.h"
#include <ctime>
#include <curl/curl.h>
#include <iostream>
#include <math.h>
#include <sstream>
#include <stdlib.h>
#include <unordered_map>
#include "../app_log.h"
#include "../rapidjson.h"
#include "../utils.h"

namespace dmkit {
namespace user_function {

// All user functions share the same signature,
// arg:
//    args: arguements supplied in configuration
//    context: current request context
//    result: value assigned to parameter value in configuration
// return: 
//    0 if function process success
//    -1 if function process fail

// Get value from a JSON string with supplied path.
// args[0]: JSON string
// args[1]: path to search for, split with a dot sign
int json_get_value(const std::vector<std::string>& args,
                   const RequestContext& context,
                   std::string& result) {
    (void)context;
    result = "";
    if (args.size() < 2) {
        return -1;
    }

    std::string data = args[0];
    std::string search = args[1];
    if (search.empty()) {
        return -1;
    }
    rapidjson::Document doc;
    if (doc.Parse(data.c_str()).HasParseError() || (!doc.IsObject() && !doc.IsArray())) {
        APP_LOG(WARNING) << "Failed to load json data: " << data;
        return -1;
    }
    std::size_t last_pos = 0;
    const rapidjson::Value* value = &doc;
    while (last_pos < search.length()) {
        std::size_t pos = search.find('.', last_pos);
        if (pos == std::string::npos) {
            pos = search.length();
        }
        std::string key = search.substr(last_pos, pos - last_pos);
        last_pos = pos + 1;

        utils::trim(key);
        if (key.empty()) {
            return -1;
        }

        if (value->IsObject()) {
            const rapidjson::Value::ConstMemberIterator miter = value->FindMember(key.c_str());
            if (miter == value->MemberEnd()) {
                return -1;
            }
            value = &(miter->value);
            continue;
        } else if (value->IsArray()) {
            if (key.empty()) {
                return -1;
            }
            unsigned index = atoi(key.c_str());
            if (index == 0 && key[0] != '0') {
                return -1;
            }
            if (value->Size() <= index) {
                return -1;
            }
            value = &((*value)[index]);
        } else {
            return -1;
        }
    }

    if (value->IsNull()) {
        return -1;
    }

    if (value->IsUint()) {
        unsigned num = value->GetUint();
        result = std::to_string(num);
    } else if (value->IsInt()) {
        int num = value->GetInt();
        result = std::to_string(num);
    } else if (value->IsUint64()) {
        unsigned long num = value->GetUint64();
        result = std::to_string(num);
    } else if (value->IsInt64()) {
        long num = value->GetInt64();
        result = std::to_string(num);
    } else if (value->IsDouble()) {
        double num = value->GetDouble();
        result = std::to_string(num);
    } else if (value->IsString()) {
        result = value->GetString();
    } else {
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        value->Accept(writer);
        result = buffer.GetString();
    }

    return 0;
}

// String replacement.
// args[0]: string
// args[1]: sub string to be replaced
// args[2]: a new string to replace with
int replace(const std::vector<std::string>& args,
        const RequestContext& context, std::string& result) {
    (void)context;
    if (args.size() != 3) {
        return -1;
    }
    std::string str = args[0];
    std::string old_substr = args[1];
    std::string new_substr = args[2];
    size_t pos = str.find(old_substr);
    if (pos != std::string::npos) {
        str.replace(pos, old_substr.length(), new_substr);
    }

    result = str;
    return 0;
}

// Add all integer numbers supplied in args.
int number_add(const std::vector<std::string>& args,
               const RequestContext& context,
               std::string& result) {
    (void)context;
    if (args.size() < 1) {
        return -1;
    }
    long res = 0;
    for (auto & arg : args) {
        if (arg.empty()) {
            return -1;
        }
        int num = atoi(arg.c_str());
        if (num == 0 && arg[0] != '0') {
            return -1;
        }
        res += num;
    }

    result = std::to_string(res);
    return 0;
}

// Multiply all float numbers supplied in args.
int float_mul(const std::vector<std::string>& args,
               const RequestContext& context,
               std::string& result) {
    (void)context;
    if (args.size() < 1) {
        return -1;
    }
    double res = 1;
    for (auto & arg : args) {
        if (arg.empty()) {
            return -1;
        }
        double num = atof(arg.c_str());
        const double EPSILON = 1e-9;
        if (fabs(num) < EPSILON && arg[0] != '0') {
            return -1;
        }
        res *= num;
    }

    result = std::to_string(res);
    return 0;
}

// Return args[2] if args[0] equals to args[1],
// otherwise return args[3]
int choose_if_equal(const std::vector<std::string>& args,
                    const RequestContext& context,
                    std::string& result) {
    (void)context;
    if (args.size() < 4) {
        return -1;
    }
    if (args[0] == args[1]) {
        result = args[2];
        return 0;
    }

    result = args[3];
    return 0;
}

// Url encode args[0]
int url_encode(const std::vector<std::string>& args,
               const RequestContext& context,
               std::string& result) {
    (void)context;
    if (args.size() < 1) {
        return -1;
    }

    if (args[0].empty()) {
        result = "";
        return 0;
    }

    CURL *curl = curl_easy_init();
    if (!curl) {
        return -1;
    }
    char *output = curl_easy_escape(curl, args[0].c_str(), args[0].length());
    if (!output) {
        curl_easy_cleanup(curl);
        return -1;
    }
    result.assign(output);
    curl_free(output);
    curl_easy_cleanup(curl);

    return 0;
}

// Make a http get request.
// args[0]: service name
// args[1]: url
int service_http_get(const std::vector<std::string>& args,
                      const RequestContext& context,
                    std::string& result) {
    if (args.size() < 2) {
        return -1;
    }

    const RemoteServiceManager* rsm = context.remote_service_manager();
    RemoteServiceParam rsm_param = {
        args[1],
        HTTP_METHOD_GET,
        "",
    };
    RemoteServiceResult rsm_result;
    if (rsm->call(args[0], rsm_param, rsm_result) != 0) {
        return -1;
    }

    result = rsm_result.result;
    return 0;
}

// Make a http post request.
// args[0]: service name
// args[1]: url
// args[2], ...: these arguments will be concatenated and used as the http post body.
int service_http_post(const std::vector<std::string>& args,
        const RequestContext& context, std::string& result) {
    if (args.size() < 2) {
        return -1;
    }

    const RemoteServiceManager* rsm = context.remote_service_manager();
    std::string post_data = "";
    for (size_t i = 2; i < args.size(); i++) {
        if (i == 2) {
            post_data = args[i];
        } else {
            post_data = post_data + "," + args[i];
        }
    }
    APP_LOG(TRACE) << "post data request:" << post_data;
    RemoteServiceParam rsm_param = {
        args[1],
        HTTP_METHOD_POST,
        post_data,
    };
    RemoteServiceResult rsm_result;
    if (rsm->call(args[0], rsm_param, rsm_result) != 0) {
        return -1;
    }

    result = rsm_result.result;
    APP_LOG(TRACE) << "post data result:" << result;
    return 0;
}

} // namespace user_function
} // namespace dmkit

