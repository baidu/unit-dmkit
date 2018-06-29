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

#ifndef DMKIT_POLICY_H
#define DMKIT_POLICY_H

#include <string>
#include <vector>
#include "rapidjson.h"

namespace dmkit {

struct KVPair {
    std::string key;
    std::string value;
};

// A trigger define the condition under which a policy is choosen,
// including intent and slots from qu result, as well as the current dm state.
struct PolicyTrigger {
    std::string intent;
    std::vector<std::string> slots;
    std::string state;
};

// The parameter required in a policy.
struct PolicyParam {
    std::string name;
    std::string type;
    std::string value;
    std::string default_value;
    bool required;
};

// Session for policy output, including current domain, user defines contexts
// and a state which DM will move to.
struct PolicyOutputSession {
    std::string domain;
    std::string state;
    std::vector<KVPair> context;

    static std::string to_json_str(const PolicyOutputSession& session);

    static PolicyOutputSession from_json_str(const std::string& json_str);
};

// A result item of dm output.
struct PolicyOutputResult {
    std::string type;
    std::vector<std::string> values;
};

struct PolicyOutputQuSlot {
    std::string key;
    std::string value;
    std::string normalized_value;
};

// In case the Qu result is required as well, currently not used.
struct PolicyOutputQu {
    std::string domain;
    std::string intent;
    std::vector<PolicyOutputQuSlot> slots;
};

// An assertion defines the condition under which a result is choosen.
struct PolicyOutputAssertion {
    std::string type;
    std::string value;
};

// Schema for DMKit output
struct PolicyOutput {
    std::vector<PolicyOutputAssertion> assertions;
    PolicyOutputQu qu;
    std::vector<KVPair> meta;
    PolicyOutputSession session;
    std::vector<PolicyOutputResult> results;

    static std::string to_json_str(const PolicyOutput& output);
};

// A policy defines a processing(params) & response(output),
// given a trigger(intent+slots+state) condition.
class Policy {
public:
    Policy(const PolicyTrigger& trigger,
           const std::vector<PolicyParam>& params, 
           const std::vector<PolicyOutput>& outputs);
    const PolicyTrigger& trigger() const;
    const std::vector<PolicyParam>& params() const;
    const std::vector<PolicyOutput>& outputs() const;

    static Policy* parse_from_json_value(const rapidjson::Value& value);

private:
    PolicyTrigger _trigger;
    std::vector<PolicyParam> _params;
    std::vector<PolicyOutput> _outputs;
};

} // namespace dmkit

#endif  //DMKIT_POLICY_H
