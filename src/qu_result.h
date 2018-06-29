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

#ifndef DMKIT_QU_RESULT_H
#define DMKIT_QU_RESULT_H

#include <string>
#include <vector>
#include "rapidjson.h"

namespace dmkit {

class Slot {
public:
    Slot(const std::string& key, const std::string& value, const std::string& normalized_value);
    
    const std::string& key() const;
    const std::string& value() const;
    const std::string& normalized_value() const;

private:
    std::string _key;
    std::string _value;
    std::string _normalized_value;
};

// Query Understanding, or natural language understanding (NLU) results for user input,
// generally includes domain, intent and slots.
class QuResult {
public:
    QuResult(const std::string& domain, const std::string& intent, const std::vector<Slot>& slots);

    static QuResult* parse_from_dialog_state(const std::string& domain,
                                             const rapidjson::Value& value);

    const std::string& domain() const;
    const std::string& intent() const;
    const std::vector<Slot>& slots() const;

    std::string to_string() const;
private:
    std::string _domain;
    std::string _intent;
    std::vector<Slot> _slots;
};

} // namespace dmkit

#endif  //DMKIT_QU_RESULT_H
