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

#include "remote_service_manager.h"
#include <cstdio>
#include <string>
#include "app_log.h"
#include "rapidjson.h"
#include "thread_data_base.h"

namespace dmkit {

RemoteServiceManager::RemoteServiceManager() {
    // 10: bucket_count, initial count of buckets, big enough to avoid resize.
    // 80: load_factor, element_count * 100 / bucket_count.
    this->_channel_map.init(10, 80);
}

RemoteServiceManager::~RemoteServiceManager() {
    for (butil::FlatMap<std::string, RemoteServiceChannel>::iterator 
            iter = this->_channel_map.begin(); iter != this->_channel_map.end(); ++iter) {
        if (nullptr != iter->second.channel) {
            delete iter->second.channel;
            iter->second.channel = nullptr;
        }
    }
}

int RemoteServiceManager::init(const char *path, const char *conf) {
    std::string file_path;
    if (path != nullptr) {
        file_path += path;
    }
    if (!file_path.empty() && file_path[file_path.length() - 1] != '/') {
        file_path += '/';
    }
    if (conf != nullptr) {
        file_path += conf;
    }

    FILE* fp = fopen(file_path.c_str(), "r");
    if (fp == nullptr) {
        APP_LOG(ERROR) << "Failed to open file " << file_path;
        return -1;
    }
    char read_buffer[1024];
    rapidjson::FileReadStream is(fp, read_buffer, sizeof(read_buffer));
    rapidjson::Document doc;
    doc.ParseStream(is);
    fclose(fp);

    if (doc.HasParseError() || !doc.IsObject()) {
        APP_LOG(ERROR) << "Failed to parse RemoteServiceManager settings";
        return -1;
    }

    rapidjson::Value::ConstMemberIterator service_iter;
    for (service_iter = doc.MemberBegin(); service_iter != doc.MemberEnd(); ++service_iter) {
        // Service name as the key for the channel.
        std::string service_name = service_iter->name.GetString();
        if (!service_iter->value.IsObject()) {
            APP_LOG(ERROR) << "Invalid service settings for " << service_name
                << ", expecting type object for service setting. Skipped...";
            continue;
        }
        const rapidjson::Value& settings = service_iter->value;
        rapidjson::Value::ConstMemberIterator setting_iter;
        // Naming service url such as https://www.baidu.com.
        // All supported url format can be found in BRPC docs.
        setting_iter = settings.FindMember("naming_service_url");
        if (setting_iter == settings.MemberEnd() || !setting_iter->value.IsString()) {
            APP_LOG(ERROR) << "Invalid service settings for " << service_name
                << ", expecting type String for property naming_service_url. Skipped...";
            continue;
        }
        std::string naming_service_url = setting_iter->value.GetString();
        // Load balancer name such as random, rr.
        // All supported balancer can be found in BRPC docs. 
        setting_iter = settings.FindMember("load_balancer_name");
        if (setting_iter == settings.MemberEnd() || !setting_iter->value.IsString()) {
            APP_LOG(ERROR) << "Invalid service settings for " << service_name
                << ", expecting type String for property load_balancer_name. Skipped...";
            continue;
        }
        std::string load_balancer_name = setting_iter->value.GetString();        
        // Protocol for the channel.
        // Currently we support http.
        setting_iter = settings.FindMember("protocol");
        if (setting_iter == settings.MemberEnd() || !setting_iter->value.IsString()) {
            APP_LOG(ERROR) << "Invalid service settings for " << service_name
                << ", expecting type String for property protocol. Skipped...";
            continue;
        }
        std::string protocol = setting_iter->value.GetString();
        // Timeout value in millisecond.
        setting_iter = settings.FindMember("timeout_ms");
        if (setting_iter == settings.MemberEnd() || !setting_iter->value.IsInt()) {
            APP_LOG(ERROR) << "Invalid service settings for " << service_name
                << ", expecting type Int for property timeout_ms. Skipped...";
            continue;
        }
        int timeout_ms = setting_iter->value.GetInt();
        // Retry count.
        setting_iter = settings.FindMember("retry");
        if (setting_iter == settings.MemberEnd() || !setting_iter->value.IsInt()) {
            APP_LOG(ERROR) << "Invalid service settings for " << service_name
                << ", expecting type Int for property retry. Skipped...";
            continue;
        }
        int retry = setting_iter->value.GetInt();
        // Headers for a http request.
        std::vector<std::pair<std::string, std::string>> headers;
        setting_iter = settings.FindMember("headers");
        if (setting_iter != settings.MemberEnd() && !setting_iter->value.IsObject()) {
            const rapidjson::Value& obj_headers = setting_iter->value;
            rapidjson::Value::ConstMemberIterator header_iter;
            for (header_iter = obj_headers.MemberBegin(); 
                    header_iter != obj_headers.MemberEnd(); ++header_iter) {
                std::string header_key = header_iter->name.GetString();
                if (!header_iter->value.IsString()) {
                    APP_LOG(ERROR) << "Invalid header value for " << header_key 
                        << ", expecting type String for header value. Skipped...";
                        continue;
                }
                std::string header_value = header_iter->value.GetString();
                headers.push_back(std::make_pair(header_key, header_value));
            }
        }

        brpc::Channel* rpc_channel = nullptr;
        if (protocol == "http") {
            rpc_channel = new brpc::Channel();
            brpc::ChannelOptions options;
            options.protocol = brpc::PROTOCOL_HTTP;
            options.timeout_ms = timeout_ms;
            options.max_retry = retry;
            rpc_channel->Init(naming_service_url.c_str(), load_balancer_name.c_str(), &options);
        } else {
            APP_LOG(ERROR) << "Unsupported protocol [" << protocol
                << "] for service [" << service_name << "], skipped...";
            continue;
        }

        RemoteServiceChannel service_channel {
            .name = service_name,
            .protocol = protocol,
            .channel = rpc_channel,
            .headers = headers
        };
        this->_channel_map.insert(service_name, service_channel);
    }

    return 0;
}

int RemoteServiceManager::call(const std::string& service_name,
                               const RemoteServiceParam& params,
                               RemoteServiceResult& result) const {
    RemoteServiceChannel *service_channel = this->_channel_map.seek(service_name);
    if (nullptr == service_channel) {
        APP_LOG(ERROR) << "Remote service call failed. Cannot find service " << service_name;
        return -1;
    }

    APP_LOG(TRACE) << "Calling service " << service_name;
    
    int ret = 0;
    std::string remote_side;
    int latency = 0;
    if (service_channel->protocol == "http") {
        ret = this->call_http(service_channel->channel,
                               params.url,
                               params.http_method,
                               service_channel->headers,
                               params.payload,
                               result.result,
                               remote_side,
                               latency);
    } else {
        APP_LOG(ERROR) << "Remote service call failed. Unknown protocol" << service_channel->protocol;
        ret = -1;
    }
    std::string log_str;
    log_str += "remote:";
    log_str += remote_side;
    log_str += "|tm:";
    log_str += std::to_string(latency);
    log_str += "|ret:";
    log_str += std::to_string(ret);
    std::string log_key = "service_";
    log_key += service_name;
    
    APP_LOG(TRACE) << "remote_side=" << remote_side << ", cost=" << latency;
    ThreadDataBase* tls = static_cast<ThreadDataBase*>(brpc::thread_local_data());
    // All backend requests are logged.
    tls->add_notice_log(log_key, log_str);

    return ret;
}

int RemoteServiceManager::call_http(brpc::Channel* channel,
                                    const std::string& url,
                                    const HttpMethod method,
                                    const std::vector<std::pair<std::string, std::string>>& headers,
                                    const std::string& payload,
                                    std::string& result,
                                    std::string& remote_side,
                                    int& latency) const {
    brpc::Controller cntl;
    cntl.http_request().uri() = url.c_str();
    if (method == HTTP_METHOD_POST) {
        cntl.http_request().set_method(brpc::HTTP_METHOD_POST);
        cntl.request_attachment().append(payload);    
    }
    for (auto const& header: headers) {
        cntl.http_request().SetHeader(header.first, header.second);
    }
    
    channel->CallMethod(NULL, &cntl, NULL, NULL, NULL);
    if (cntl.Failed()) {
        APP_LOG(WARNING) << "Call failed, error: " << cntl.ErrorText();
        return -1;
    }
    result = cntl.response_attachment().to_string();
    
    remote_side = butil::endpoint2str(cntl.remote_side()).c_str();
    latency = cntl.latency_us() / 1000;
    
    return 0;
}

} // namespace dmkit
