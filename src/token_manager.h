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

#include <ctime>
#include <memory>
#include <mutex>
#include <unordered_map>
#include "app_log.h"
#include "remote_service_manager.h"

#ifndef DMKIT_TOKEN_MANAGER_H
#define DMKIT_TOKEN_MANAGER_H

namespace dmkit {

struct ClientKey {
    std::string api_key;
    std::string secret_key;
};

struct TokenValue {
    // access token
    std::string access_token;
    // timestamp when the access token expires
    std::time_t expire_time;

};

// Type for client key map.
typedef std::unordered_map<std::string, ClientKey> ClientKeyMap;

// A Manager class to manage and cache access token accessing unit bot api.
class TokenManager {
public:
    TokenManager();

    ~TokenManager();

    // Initialization with a json configuration file.
    int init(const char* dir_path, const char* conf_file);

    // Reload config.
    int reload();

    // Callback when conf change.
    static int client_key_conf_change_callback(void* param);

    // Get access token with bot id.
    int get_access_token(const std::string bot_id,
            const RemoteServiceManager* remote_service_manager, std::string& access_token);

private:
    int get_token_from_cache(const std::string bot_id, TokenValue& token_value);
    int update_token_cache(const std::string bot_id, const TokenValue token_value);

    int get_token_from_remote(const ClientKey client_key,
            const RemoteServiceManager* remote_service_manager, TokenValue& token_value);

    ClientKeyMap* load_client_key_map();

    std::string _client_key_conf_path;
    std::shared_ptr<ClientKeyMap> _p_client_key_map;

    std::unordered_map<std::string, TokenValue>* _token_cache;
    std::mutex _token_cache_mutex;
};

}

#endif

