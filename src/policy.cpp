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

#include "policy.h"
#include "app_log.h"
#include "utils.h"

namespace dmkit {

Policy::Policy(const PolicyTrigger& trigger, 
               const std::vector<PolicyParam>& params, 
               const std::vector<PolicyOutput>& outputs)
    : _trigger(trigger), _params(params), _outputs(outputs) {
}

const PolicyTrigger& Policy::trigger() const {
    return this->_trigger;
}

const std::vector<PolicyParam>& Policy::params() const {
    return this->_params;
}

const std::vector<PolicyOutput>& Policy::outputs() const {
    return this->_outputs;
}

// Parse a policy from json configuration.
// A sample policy is as following:
//    {
//        "trigger": {
//            "intent": "INTENT_XXX",
//            "slots": [
//                "slot_1",
//                "slot_2"
//            ],
//            "state": "001"
//        },
//        "params": [
//            {
//                "name": "param_name", 
//                "type": "slot_val", 
//                "value": "slot_tag"
//            }
//        ],
//        "output": [
//            {
//                "assertion": [
//                    {
//                        "type": "eq", 
//                        "value": "1,{%param_name%}"
//                    }
//                ],
//                "session": {
//                    "state": "002"
//                    "context": {
//                        "key1": "value1",
//                        "key2": "value2"
//                    }
//                },
//                "meta":{
//                    "key1": "value1",
//                    "key2": "value2"
//                },
//                "result": [
//                    {
//                        "type":"tts",
//                        "value": "Hello World"
//                    }
//                ]
//            }
//        ]
//    }
Policy* Policy::parse_from_json_value(const rapidjson::Value& value) {
    if (!value.IsObject()) {
        LOG(WARNING) << "Failed to parse policy from json, not an object: " << utils::json_to_string(value);
        return nullptr;
    }

    if (!value.HasMember("trigger") || !value["trigger"].IsObject()
            || !value["trigger"].HasMember("intent") || !value["trigger"]["intent"].IsString()) {
        LOG(WARNING) << "Failed to parse policy from json, invalid trigger";
        return nullptr;
    }
    PolicyTrigger trigger;
    trigger.intent = value["trigger"]["intent"].GetString();
    
    if (value["trigger"].HasMember("slots") && value["trigger"]["slots"].IsArray()) {
        for (auto& v : value["trigger"]["slots"].GetArray()) {
            trigger.slots.push_back(v.GetString());
        }
    }
    if (value["trigger"].HasMember("state") && value["trigger"]["state"].IsString()) {
        trigger.state = value["trigger"]["state"].GetString();
    }

    std::vector<PolicyParam> params;
    if (value.HasMember("params") && value["params"].IsArray()) {
        for (auto& v : value["params"].GetArray()) {
            PolicyParam param;
            param.name = v["name"].GetString();
            param.type = v["type"].GetString();
            param.value = v["value"].GetString();
            if (v.HasMember("required") && v["required"].IsBool()) {
                param.required = v["required"].GetBool();
            } else {
                param.required = false;
            }
            if (v.HasMember("default") && v["default"].IsString()) {
                param.default_value = v["default"].GetString();
            }
            params.push_back(param);
        }
    }
    
    std::vector<PolicyOutput> outputs;
    if (value.HasMember("output") && value["output"].IsArray()) {
        for (auto& v : value["output"].GetArray()) {
            PolicyOutput output;
            if (v.HasMember("assertion") && v["assertion"].IsArray()) {
                for (auto& v_assertion: v["assertion"].GetArray()) {
                    std::string assertion_type = v_assertion["type"].GetString();
                    std::string assertion_value = v_assertion["value"].GetString();
                    PolicyOutputAssertion assertion = {assertion_type, assertion_value};
                    output.assertions.push_back(assertion);
                }
            }
            if (v.HasMember("session") && v["session"].IsObject()) {
                if (v["session"].HasMember("state") && v["session"]["state"].IsString()) {
                    output.session.state = v["session"]["state"].GetString();
                }
                if (v["session"].HasMember("context") && v["session"]["context"].IsObject()) {
                    for (auto& m_context: v["session"]["context"].GetObject()) {
                        std::string context_key = m_context.name.GetString();
                        std::string context_value = m_context.value.GetString();
                        KVPair context = {context_key, context_value};
                        output.session.context.push_back(context);
                    }
                }
            }
            if (v.HasMember("meta") && v["meta"].IsObject()) {
                for (auto& m_meta: v["meta"].GetObject()) {
                    std::string meta_key = m_meta.name.GetString();
                    std::string meta_value = m_meta.value.GetString();
                    KVPair meta = {meta_key, meta_value};
                    output.meta.push_back(meta);
                }
            }
            for (auto& v_result : v["result"].GetArray()) {
                PolicyOutputResult result;
                result.type = v_result["type"].GetString();
                if (v_result["value"].IsString()) {
                    result.values.push_back(v_result["value"].GetString());
                } else if (v_result["value"].IsArray()) {
                    for (auto& v_result_value : v_result["value"].GetArray()) {
                        result.values.push_back(v_result_value.GetString());
                    }
                }
                if (v_result.HasMember("extra") && v_result["extra"].IsString()) {
                    result.extra = v_result["extra"].GetString();
                }
                output.results.push_back(result);
            }

            outputs.push_back(output);
        }
    }
    
    return new Policy(trigger, params, outputs);;
}

std::string PolicyOutputSession::to_json_str(const PolicyOutputSession& session) {
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    writer.StartObject();
    writer.Key("domain");
    writer.String(session.domain.c_str(), session.domain.length());
    writer.Key("state");
    writer.String(session.state.c_str(), session.state.length());
    writer.Key("context");
    writer.StartObject();
    for (auto const& object: session.context) {
        writer.Key(object.key.c_str(), object.key.length());
        writer.String(object.value.c_str(), object.value.length());
    }
    writer.EndObject();
    writer.EndObject();
    return buffer.GetString();
}

PolicyOutputSession PolicyOutputSession::from_json_str(const std::string& json_str) {
    PolicyOutputSession session;
    rapidjson::Document session_doc;
    if (session_doc.Parse(json_str.c_str()).HasParseError() || !session_doc.IsObject()) {
        return session;
    }
    if (session_doc.HasMember("domain")) {
        session.domain = session_doc["domain"].GetString();
    }
    if (session_doc.HasMember("state")) {
        session.state = session_doc["state"].GetString();
    }
    if (!session_doc.HasMember("context")) {
        return session;
    }
    for (auto& m_object: session_doc["context"].GetObject()) {
        std::string context_key = m_object.name.GetString();
        std::string context_value = m_object.value.GetString();
        KVPair context = {context_key, context_value};
        session.context.push_back(context);
    }

    return session;
}

std::string PolicyOutput::to_json_str(const PolicyOutput& output) {
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    writer.StartObject();
    writer.Key("meta");
    writer.StartObject();
    for (auto const& meta: output.meta) {
        writer.Key(meta.key.c_str(), meta.key.length());
        writer.String(meta.value.c_str(), meta.value.length());
    }
    writer.EndObject();
    writer.Key("result");
    writer.StartArray();
    for (auto const& result: output.results) {
        writer.StartObject();
        writer.Key("type");
        writer.String(result.type.c_str(), result.type.length());
        writer.Key("value");
        writer.String(result.values[0].c_str(), result.values[0].length());
        if (!result.extra.empty()) {
            rapidjson::Document extra_doc;
            if (!extra_doc.Parse(result.extra.c_str()).HasParseError() && extra_doc.IsObject()) {
                for (auto& v_extra: extra_doc.GetObject()) {
                    std::string extra_key = v_extra.name.GetString();
                    if (extra_key == "type" || extra_key == "value") {
                        LOG(WARNING) << "Unsupported extra key " << extra_key;
                    }
                    if (v_extra.value.IsString()) {
                        std::string extra_value = v_extra.value.GetString();
                        writer.Key(extra_key.c_str());
                        writer.String(extra_value.c_str(), extra_value.length());
                    } else if (v_extra.value.IsBool()) {
                        writer.Key(extra_key.c_str());
                        writer.Bool(v_extra.value.GetBool());
                    } else if (v_extra.value.IsInt()) {
                        writer.Key(extra_key.c_str());
                        writer.Int(v_extra.value.GetInt());
                    } else if (v_extra.value.IsDouble()) {
                        writer.Key(extra_key.c_str());
                        writer.Double(v_extra.value.GetDouble());
                    } else {
                        LOG(WARNING) << "Unknown extra value type " << v_extra.value.GetType();
                    }
                }
            } else {
                LOG(WARNING) << "Failed to parse result extra json: " <<  result.extra;
            }
        }
        writer.EndObject();
    }
    writer.EndArray();

    writer.EndObject();
    return buffer.GetString();
}

} // namespace dmkit
