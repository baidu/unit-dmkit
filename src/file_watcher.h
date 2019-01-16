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

#ifndef DMKIT_FILE_WATCHER_H
#define DMKIT_FILE_WATCHER_H

#include <atomic>
#include <mutex>
#include <thread>
#include <string>
#include <unordered_map>

namespace dmkit {

// Callback function when file changed
typedef int (*FileChangeCallback)(void* param);

struct FileStatus {
    std::string file_path;
    std::string last_modified_time;
    FileChangeCallback callback;
    void* param;
    bool level_trigger;
};

// A file watcher singleton implemention
class FileWatcher {
public:
    static FileWatcher& get_instance();

    int register_file(const std::string file_path,
                      FileChangeCallback cb,
                      void* param,
                      bool level_trigger=false);

    int unregister_file(const std::string file_path);

    // Do not need copy constructor and assignment operator for a singleton class
    FileWatcher(FileWatcher const&) = delete;
    void operator=(FileWatcher const&) = delete;
private:
    FileWatcher();
    virtual ~FileWatcher();

    void watcher_thread_func();

    std::mutex _mutex;
    std::atomic<bool> _is_running;
    std::thread _watcher_thread;
    std::unordered_map<std::string, FileStatus> _file_info;
    static const int CHECK_INTERVAL_IN_MILLS = 1000;
};

} // namespace dmkit

#endif  //DMKIT_FILE_WATCHER_H
