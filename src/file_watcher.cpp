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

#include "file_watcher.h"
#include <sys/time.h>
#include <sys/stat.h>
#include "app_log.h"
#include "utils.h"

namespace dmkit {

const int FileWatcher::CHECK_INTERVAL_IN_MILLS;

FileWatcher&  FileWatcher::get_instance() {
    static FileWatcher instance;
    return instance;
}

FileWatcher::FileWatcher() {
}

FileWatcher::~FileWatcher() {
    this->_is_running = false;
    if (this->_watcher_thread.joinable()) {
        this->_watcher_thread.join();
    }
}

static int get_file_last_modified_time(const std::string& file_path, std::string& mtime_str) {
    struct stat f_stat;
    if (stat(file_path.c_str(), &f_stat) != 0) {
        LOG(WARNING) << "Failed to get file modified time" << file_path;
        return -1;
    }
    mtime_str = ctime(&f_stat.st_mtime);
    return 0;
}

int FileWatcher::register_file(const std::string file_path,
                               FileChangeCallback cb,
                               void* param,
                               bool level_trigger) {
    LOG(TRACE) << "FileWatcher registering file " << file_path;
    std::string last_modified_time;
    if (get_file_last_modified_time(file_path, last_modified_time) != 0) {
        return -1;
    }

    FileStatus file_status = {file_path, last_modified_time, cb, param, level_trigger};
    std::lock_guard<std::mutex> lock(this->_mutex);
    this->_file_info[file_path] = file_status;
    if (!this->_is_running) {
        this->_is_running = true;
        this->_watcher_thread = std::thread(&FileWatcher::watcher_thread_func, this);
    }
    return 0;
}

int FileWatcher::unregister_file(const std::string file_path) {
    LOG(TRACE) << "FileWatcher unregistering file " << file_path;
    std::lock_guard<std::mutex> lock(this->_mutex);
    if (this->_file_info.erase(file_path) != 1) {
        return -1;
    }
    if (this->_file_info.size() == 0
        && this->_is_running
        && this->_watcher_thread.joinable()) {
        this->_is_running = false;
        this->_watcher_thread.join();
    }
    return 0;
}

void FileWatcher::watcher_thread_func() {
    LOG(TRACE) << "Watcher thread starting...";
    while (this->_is_running) {
        {
            std::lock_guard<std::mutex> lock(this->_mutex);
            for (const auto& file : this->_file_info) {
                std::string last_modified_time;
                get_file_last_modified_time(file.first, last_modified_time);
                if (last_modified_time != file.second.last_modified_time) {
                    LOG(TRACE) << "File Changed. " << file.first << " modified time " << last_modified_time;
                    if (file.second.callback(file.second.param) == 0
                            || !file.second.level_trigger) {
                        if (this->_file_info.find(file.first) != this->_file_info.end()) {
                            this->_file_info[file.first].last_modified_time = last_modified_time;
                        }
                    }
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(FileWatcher::CHECK_INTERVAL_IN_MILLS));
    }
    LOG(TRACE) << "Watcher thread stopping...";
}

} // namespace dmkit
