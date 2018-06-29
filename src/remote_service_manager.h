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

#include <string>
#include <utility>
#include <vector>
#include <brpc/channel.h>
#include <butil/containers/flat_map.h>

namespace dmkit {

enum HttpMethod {
    // Set to the same number as the method defined in baidu::rpc::HttpMethod
    HTTP_METHOD_GET         =   1,
    HTTP_METHOD_POST        =   3,
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
    brpc::Channel *channel;
    // Headers for http procotol
    std::vector<std::pair<std::string, std::string>> headers;
};

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
    
    // Call a remote service with specifid service name.
    int call(const std::string& servie_name,
             const RemoteServiceParam& params,
             RemoteServiceResult &result) const;

private:
    // Http is the most common protocol.
    int call_http(brpc::Channel* channel,
                  const std::string& url,
                  const HttpMethod method,
                  const std::vector<std::pair<std::string, std::string>>& headers,
                  const std::string& payload,
                  std::string& result,
                  std::string& remote_side,
                  int& latency) const;

    butil::FlatMap<std::string, RemoteServiceChannel> _channel_map;
};

} // namespace dmkit

#endif  //DMKIT_THREAD_DATA_BASE_H
