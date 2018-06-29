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

#ifndef DMKIT_UTILS_H
#define DMKIT_UTILS_H

#include <stdlib.h>
#include <string>
#include "app_log.h"
#include "rapidjson.h"

namespace dmkit {
namespace utils {

static inline void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
        return !std::isspace(ch);
    }));
}

static inline void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

static inline void trim(std::string &s) {
    ltrim(s);
    rtrim(s);
}

static inline bool try_atoi(const std::string& str, int& value) {
    if (str.empty()) {
        return false;
    }

    value = atoi(str.c_str());
    if (value == 0 && str[0] != '0') {
        return false;
    }

    if (value < 0 && str[0] != '-') {
        return false;
    }

    return true;
}

static inline bool try_atof(const std::string& str, double& value) {
    if (str.empty()) {
        return false;
    }

    value = atof(str.c_str());
    const double EPSILON = 1e-9;
    if (fabs(value) < EPSILON && str[0] != '0') {
        return false;
    }

    return true;
}

static inline bool split(const std::string& str, const char sep, std::vector<std::string>& elems) {
    elems.clear();
    if (str.empty()) {
        return false;
    }
    std::istringstream ss(str);
    std::string item;
    while (std::getline(ss, item, sep)) {
        elems.push_back(item);
    } 
    return true;
}

static inline std::string json_to_string(const rapidjson::Value& value) {
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    value.Accept(writer);
    return buffer.GetString();
}

} // namespace utils
} // namespace dmkit

#endif  //DMKIT_UTILS_H
