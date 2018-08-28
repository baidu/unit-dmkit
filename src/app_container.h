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

#ifndef DMKIT_APP_CONTAINER_H
#define DMKIT_APP_CONTAINER_H

#include "application_base.h"
#include "brpc.h"

namespace dmkit {

class ThreadLocalDataFactory : public BRPC_NAMESPACE::DataFactory {
public:
    ThreadLocalDataFactory(ApplicationBase* application);
    ~ThreadLocalDataFactory();
    void* CreateData() const;
    void DestroyData(void* d) const;

private:
    ApplicationBase* _application;
};

// Container class which manages the instances of application,
// as well as a thread data factory instance.
class AppContainer {
public:
    AppContainer();
    ~AppContainer();
    int load_application();
    ThreadLocalDataFactory* get_thread_local_data_factory();
    int run(BRPC_NAMESPACE::Controller* cntl);

private:
    // The application instance is shared for all rpc threads
    ApplicationBase* _application;

    ThreadLocalDataFactory* _data_factory;
};

} // namespace dmkit

#endif  //DMKIT_APP_CONTAINER_H
