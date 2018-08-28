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

#include "app_container.h"
#include <chrono>
#include "app_log.h"
#include "brpc.h"
#include "dialog_manager.h"

namespace dmkit {

ThreadLocalDataFactory::ThreadLocalDataFactory(ApplicationBase* application) {
    this->_application = application;
}

ThreadLocalDataFactory::~ThreadLocalDataFactory() {
    this->_application = nullptr;
}

void* ThreadLocalDataFactory::CreateData() const {
    return this->_application->create_thread_data();
}

void ThreadLocalDataFactory::DestroyData(void* d) const {
    this->_application->destroy_thread_data(d);
}

AppContainer::AppContainer() {
    this->_application = nullptr;
    this->_data_factory = nullptr;
}

AppContainer::~AppContainer() {
    delete this->_application;
    this->_application = nullptr;

    delete this->_data_factory;
    this->_data_factory = nullptr;
}

int AppContainer::load_application() {
    if (nullptr != this->_application) {
        APP_LOG(ERROR) << "an application has already be loaded";
        return -1;
    }

    // The real application is created here
    this->_application = new DialogManager();

    if (nullptr == this->_application || 0 != this->_application->init()) {
        APP_LOG(ERROR) << "failed to init application!!!";
        return -1;
    }

    this->_data_factory = new ThreadLocalDataFactory(this->_application);

    return 0;
}

ThreadLocalDataFactory* AppContainer::get_thread_local_data_factory() {
    if (nullptr == this->_data_factory) {
        APP_LOG(ERROR) << "Data factory has not been initialized!!!";
        return nullptr;
    }

    return this->_data_factory;
}

int AppContainer::run(BRPC_NAMESPACE::Controller* cntl) {
    if (nullptr == this->_application) {
        APP_LOG(ERROR) << "No application is not loaded for processing!!!";
        return -1;
    }

    auto time_start = std::chrono::steady_clock::now();

    // Need to reset thread data status before running the application
    ThreadDataBase* tls = static_cast<ThreadDataBase*>(BRPC_NAMESPACE::thread_local_data());
    tls->reset();
    APP_LOG(TRACE) << "Running application";
    int result = this->_application->run(cntl);

    auto time_end = std::chrono::steady_clock::now();
    std::chrono::duration<double> diff = std::chrono::duration_cast<std::chrono::duration<double>>(time_end - time_start);
    double total_cost = diff.count() * 1000;
    APP_LOG(TRACE) << "Application run cost(ms): " << total_cost;
    tls->add_notice_log("tm", std::to_string(total_cost));
    APP_LOG(NOTICE) << tls->get_notice_log();

    return result;
}

} // namespace dmkit

