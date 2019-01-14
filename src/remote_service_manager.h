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

#ifndef DMKIT_REMOTE_SERVICE_MANAGER_H
#define DMKIT_REMOTE_SERVICE_MANAGER_H

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include "brpc.h"
#include "butil.h"

namespace dmkit {

enum HttpMethod {
    // Set to the same number as the method defined in baidu::rpc::HttpMethod
    HTTP_METHOD_DELETE      =   0,
    HTTP_METHOD_GET         =   1,
    HTTP_METHOD_POST        =   3,
    HTTP_METHOD_PUT         =   4
};

struct RemoteServiceParam {
    // Url for http service
    std::string url;
    // Http method
    HttpMethod http_method;
    // Request body
    std::string payload;
};

struct RemoteServiceResult {
    // Response body
    std::string result;
};

struct RemoteServiceChannel {
    // Name of the channel for rpc call
    std::string name;
    // Protocol such as http
    std::string protocol;
    // Rpc channel instance
    BRPC_NAMESPACE::Channel *channel;
    // timeout in milliseconds
    int timeout_ms;
    // retry count
    int max_retry;
    // Headers for http procotol
    std::vector<std::pair<std::string, std::string>> headers;
};

// Type for channel map
typedef std::unordered_map<std::string, RemoteServiceChannel> ChannelMap;

// A configurable remote service manager class.
// All remote service channels are created with configuration file
// when initialization. Caller calls a remote service by supplying
// the service name and other parameters.
class RemoteServiceManager {
public:
    RemoteServiceManager();

    ~RemoteServiceManager();

    // Initalization with a json configuration file.
    int init(const char *path, const char *conf);

    // Reload config.
    int reload();

    // Callback when service conf changed.
    static int service_conf_change_callback(void* param);

    // Call a remote service with specifid service name.
    int call(const std::string& servie_name,
             const RemoteServiceParam& params,
             RemoteServiceResult &result) const;

private:
    // Http is the most common protocol.
   int call_http_by_BRPC_NAMESPACE(BRPC_NAMESPACE::Channel* channel,
                         const std::string& url,
                         const HttpMethod method,
                         const std::vector<std::pair<std::string, std::string>>& headers,
                         const std::string& payload,
                         std::string& result,
                         std::string& remote_side,
                         int& latency) const;

    int call_http_by_curl(const std::string& url,
                          const HttpMethod method,
                          const std::vector<std::pair<std::string, std::string>>& headers,
                          const std::string& payload,
                          const int timeout_ms,
                          const int max_retry,
                          std::string& result,
                          std::string& remote_side,
                          int& latency) const;

    ChannelMap* load_channel_map();

    std::string _conf_file_path;
    std::shared_ptr<ChannelMap> _p_channel_map;
};

} // namespace dmkit

#endif  //DMKIT_THREAD_DATA_BASE_H
