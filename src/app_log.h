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

#ifndef DMKIT_APP_LOG_H
#define DMKIT_APP_LOG_H

#include <brpc/server.h>
#include <butil/logging.h>
#include "thread_data_base.h"

namespace dmkit {

// Wrapper for application logging to include trace id for each log during a request.
#define APP_LOG(severity)  \
    LOG(severity) << "logid=" << (brpc::thread_local_data() == nullptr ? "" : \
      (static_cast<dmkit::ThreadDataBase*>(brpc::thread_local_data()))->get_log_id()) \
      << " "

} // namespace dmkit

#endif  //DMKIT_APP_LOG_H
