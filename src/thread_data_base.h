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

#ifndef DMKIT_THREAD_DATA_BASE_H
#define DMKIT_THREAD_DATA_BASE_H

#include <string>
#include <vector>

namespace dmkit {

class ThreadDataBase {
public:
    ThreadDataBase() {}
    
    virtual ~ThreadDataBase() {}
    
    virtual void reset() {
        this->_log_id.clear();
        this->_notice_log.clear();
    }
    
    // Set a logid for current request, this logid will show in all logs for current request
    void set_log_id(const std::string& log_id) { this->_log_id = log_id; }
    
    const std::string get_log_id() { return this->_log_id; }

    // Add and save notice log inside thread data so that each request will log only one notice log
    void add_notice_log(const std::string& key, const std::string& value) {
        std::string log;
        log += key;
        log += "=";
        log += value;
        this->_notice_log.push_back(log);
    }

    // Get notice log as a string in the format "key1=value1 key2=value2"
    const std::string get_notice_log() {
        std::string log_str;
        for (auto v: this->_notice_log) {
            log_str += v;
            log_str += " ";
        }
        return log_str;
    }

private:
    std::string _log_id;
    std::vector<std::string> _notice_log;
};

} // namespace dmkit

#endif  //DMKIT_THREAD_DATA_BASE_H
