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

#include "token_manager.h"
#include "rapidjson.h"

namespace dmkit {

TokenManager::TokenManager() {
    this->_client_key_map = new std::unordered_map<std::string, ClientKey>();
    this->_token_cache = new std::unordered_map<std::string, TokenValue>();
}

TokenManager::~TokenManager() {
    delete this->_client_key_map;
    this->_client_key_map = nullptr;
    delete this->_token_cache;
    this->_token_cache = nullptr;
}

int TokenManager::init(const char* dir_path, const char* conf_file) {
   
    std::string file_path;
    if (dir_path != nullptr) {
        file_path += dir_path;    
    }
    if (!file_path.empty() && file_path[file_path.length() - 1] != '/') {
        file_path += '/';
    }
    if (conf_file != nullptr) {
        file_path += conf_file;    
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
        APP_LOG(ERROR) << "Failed to parse Token settings";
        return -1;
    }

    for (rapidjson::Value::ConstMemberIterator bot_iter = doc.MemberBegin();
            bot_iter != doc.MemberEnd(); ++bot_iter) {
        std::string bot_id = bot_iter->name.GetString();
        if (!bot_iter->value.IsObject()) {
            APP_LOG(ERROR) << "Invalid token conf for " << bot_id << ", invalid format";
            return -1;
        }
        const rapidjson::Value& val_token = bot_iter->value;
        if (!val_token.HasMember("api_key") || !val_token["api_key"].IsString()) {
            APP_LOG(ERROR) << "Invalid token conf for " << bot_id << ", missing api_key";
            return -1;
        }
        if (!val_token.HasMember("secret_key") || !val_token["secret_key"].IsString()) {
            APP_LOG(ERROR) << "Invalid token conf for " << bot_id << ", missing secret_key";
            return -1;
        }
        ClientKey client_key;
        client_key.api_key = val_token["api_key"].GetString();
        client_key.secret_key = val_token["secret_key"].GetString();
        (*this->_client_key_map)[bot_id] = client_key;
    }

    return 0;
}

int TokenManager::get_access_token(const std::string bot_id,
        const RemoteServiceManager* remote_service_manager, std::string& access_token) {
    TokenValue token_value;
    if (this->get_token_from_cache(bot_id, token_value) == 0) {
        access_token = token_value.access_token;
        return 0;
    }
    auto client_key_iter = this->_client_key_map->find(bot_id);
    if (client_key_iter == this->_client_key_map->end()) {
        return -1;
    }
    ClientKey client_key = client_key_iter->second;
    if (this->get_token_from_remote(client_key, remote_service_manager, token_value) != 0) {
        return -1;
    }
    this->update_token_cache(bot_id, token_value);
    access_token = token_value.access_token;
    return 0;
}

int TokenManager::get_token_from_cache(const std::string bot_id, TokenValue& token_value) {
    time_t expire_guard = time(nullptr) + 60;
    std::lock_guard<std::mutex> lock(this->_token_cache_mutex);
    auto token_iter = this->_token_cache->find(bot_id);
    if (token_iter == this->_token_cache->end()) {
        return -1;
    }
    TokenValue value = token_iter->second;
    if (value.expire_time < expire_guard) {
        return -1;
    }
    token_value = value;
    return 0;
}

int TokenManager::update_token_cache(const std::string bot_id, const TokenValue token_value) {
    std::lock_guard<std::mutex> lock(this->_token_cache_mutex);
    (*this->_token_cache)[bot_id] = token_value;
    return 0;
}

int TokenManager::get_token_from_remote(const ClientKey client_key, 
        const RemoteServiceManager* remote_service_manager, TokenValue& token_value) {
    std::string url = "https://aip.baidubce.com/oauth/2.0/token?grant_type=client_credentials";
    url += "&client_id=" + client_key.api_key + "&client_secret=" + client_key.secret_key;
    RemoteServiceParam rsp = {
        url,
        HTTP_METHOD_GET,
        ""
    };
    RemoteServiceResult rsr;
    if (remote_service_manager->call("token_auth", rsp, rsr) != 0) {
        APP_LOG(ERROR) << "Failed to get authorization result";
        return -1;
    }
    rapidjson::Document json;
    if (json.Parse(rsr.result.c_str()).HasParseError()) {
        APP_LOG(ERROR) << "Failed to parse authorization result to json";
        return -1;
    }
    token_value.access_token = json["access_token"].GetString();
    token_value.expire_time = time(0) + json["expires_in"].GetInt();
    return 0;
}

} //namespace dmkit
