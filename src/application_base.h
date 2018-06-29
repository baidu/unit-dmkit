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

#ifndef DMKIT_APPLICATION_BASE_H
#define DMKIT_APPLICATION_BASE_H

#include <brpc/controller.h>
#include <brpc/server.h>
#include "thread_data_base.h"

namespace dmkit {

// Base class for applications
// Notice that the same application instance will be used for all request thread,
// This call should be thread safe.
class ApplicationBase {
public:
    ApplicationBase() {};
    virtual ~ApplicationBase() {};

    // Interface for application to do global initialization.
    // This method will be invoke only once when server starts.
    virtual int init() = 0;

    // Interface for application to handle requests, it should be thread safe.
    virtual int run(brpc::Controller* cntl) = 0;

    // Interface for application to register customized thread data.
    virtual void* create_thread_data() const {
        return new ThreadDataBase();
    }

    // Interface to destroy thread data, the data instance was created by create_thread_data.
    virtual void destroy_thread_data(void* d) const {
        delete static_cast<ThreadDataBase*>(d);
    }

    // Set log id for current request,
    // application should set log id as early as possible when processing request
    virtual void set_log_id(const std::string& log_id) {
        ThreadDataBase* tls = static_cast<ThreadDataBase*>(brpc::thread_local_data());
        tls->set_log_id(log_id);
    }

    // Add a key/value notice log which will be logged when finish processing request
    virtual void add_notice_log(const std::string& key, const std::string& value) {
        ThreadDataBase* tls = static_cast<ThreadDataBase*>(brpc::thread_local_data());
        tls->add_notice_log(key, value);
    }
};

} // namespace dmkit

#endif  //DMKIT_APPLICATION_BASE_H

