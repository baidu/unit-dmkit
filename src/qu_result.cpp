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

#include "qu_result.h"
#include "app_log.h"
#include "utils.h"

namespace dmkit {

Slot::Slot(const std::string& key, const std::string& value, const std::string& normalized_value)
    : _key(key), _value(value), _normalized_value(normalized_value) {
}

const std::string& Slot::key() const {
    return this->_key;
}

const std::string& Slot::value() const {
    return this->_value;
}

const std::string& Slot::normalized_value() const {
    return this->_normalized_value;
}

QuResult::QuResult(const std::string& domain,
                   const std::string& intent,
                   const std::vector<Slot>& slots)
    : _domain(domain), _intent(intent), _slots(slots) {
}

// Parse QU result from dialog state in unit response
QuResult* QuResult::parse_from_dialog_state(const std::string& domain, 
                                            const rapidjson::Value& value) {
    if (!value.IsObject()) {
        APP_LOG(WARNING) << "Failed to parse qu result from json, not an object";
        return nullptr;
    }
    
    if (!value.HasMember("intents") || !value["intents"].IsArray() || value["intents"].Size() < 1) {
        APP_LOG(WARNING) << "Failed to parse qu result from json";
    }
    int intents_size = value["intents"].Size();
    std::string intent = value["intents"][intents_size - 1]["name"].GetString();
    std::vector<Slot> slots;
    for (auto& m_slot: value["user_slots"].GetObject()) {
        std::string tag = m_slot.name.GetString();
        for (auto& m_value: m_slot.value.GetObject()["values"].GetObject()) {
            int slot_state = m_value.value.GetObject()["state"].GetInt();
            // Slot state possible values are
            // 0: slot not filled
            // 1: slot is filled by default value
            // 2: slot is filled by SLU
            // 4: slot was filled but has been replaced by other value
            if (slot_state == 0 || slot_state == 4) {
                continue;
            }
            std::string normalized_value = m_value.name.GetString();
            std::string value = m_value.value.GetObject()["original_name"].GetString();
            Slot slot(tag, value, normalized_value);
            slots.push_back(slot);
        }
    }

    QuResult* result = new QuResult(domain, intent, slots);
    APP_LOG(TRACE) << result->to_string();
    return result;
}

const std::string& QuResult::domain() const {
    return this->_domain;
}

const std::string& QuResult::intent() const {
    return this->_intent;
}

const std::vector<Slot>& QuResult::slots() const {
    return this->_slots;
}

std::string QuResult::to_string() const {
    std::string result;
    result += "domain:";
    result += this->domain();
    result += " intent:";
    result += this->intent();
    result += " slots: {";
    for (auto iter = this->slots().begin(); iter != this->slots().end(); ++iter) {
        result += iter->key();
        result += ":";
        result += iter->value();
        if (!iter->normalized_value().empty()) {
            result += "(";
            result += iter->normalized_value();
            result += ")";
        }
        result += " ";
    }
    result += "}";

    return result;
}

} // namespace dmkit
