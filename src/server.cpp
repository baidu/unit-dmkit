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

#include <gflags/gflags.h>
#include "app_container.h"
#include "app_log.h"
#include "brpc.h"
#include "http.pb.h"

DEFINE_int32(port, -1, "TCP Port of this server");
DEFINE_int32(internal_port, -1, "Only allow builtin services at this port");
DEFINE_int32(idle_timeout_s, -1, "Connection will be closed if there is no read/write operations during this time");
DEFINE_int32(max_concurrency, 0, "Limit of requests processing in parallel");
DEFINE_string(url_path, "", "Url path of the app");
DEFINE_bool(log_to_file, false, "Log to file");

namespace dmkit {

// Service with static path.
class HttpServiceImpl : public HttpService {
public:
    HttpServiceImpl() {};

    virtual ~HttpServiceImpl() {};

    int init() {
        return this->_app_container.load_application();
    }

    ThreadLocalDataFactory* get_thread_local_data_factory() {
        return this->_app_container.get_thread_local_data_factory();
    }

    void run(google::protobuf::RpcController* cntl_base,
              const HttpRequest*,
              HttpResponse*,
              google::protobuf::Closure* done) {
        // This object helps you to call done->Run() in RAII style. If you need
        // to process the request asynchronously, pass done_guard.release().
        BRPC_NAMESPACE::ClosureGuard done_guard(done);

        BRPC_NAMESPACE::Controller* cntl = static_cast<BRPC_NAMESPACE::Controller*>(cntl_base);
        this->_app_container.run(cntl);
    }

private:
    AppContainer _app_container;
};

} // namespace dmkit

int main(int argc, char* argv[]) {
    // Parse gflags. We recommend you to use gflags as well.
    GFLAGS_NS::SetCommandLineOption("flagfile", "conf/gflags.conf");
    GFLAGS_NS::ParseCommandLineFlags(&argc, &argv, true);

#ifdef BUTIL_ENABLE_COMLOG_SINK
    if (FLAGS_log_to_file) {
        if (logging::ComlogSink::GetInstance()->SetupFromConfig("conf/log.conf") != 0) {
            APP_LOG(ERROR) << "Fail to setup comlog from conf/log.conf";
            return -1;
        }
    }
#endif

    // Generally you only need one Server.
    BRPC_NAMESPACE::Server server;
    dmkit::HttpServiceImpl http_svc;

    if (http_svc.init() != 0) {
        APP_LOG(ERROR) << "Fail to init http_svc";
        return -1;
    }

    // Add services into server. Notice the second parameter, because the
    // service is put on stack, we don't want server to delete it, otherwise
    // use baidu::rpc::SERVER_OWNS_SERVICE.
    std::string mapping = FLAGS_url_path + " => run";
    if (server.AddService(&http_svc,
                          BRPC_NAMESPACE::SERVER_DOESNT_OWN_SERVICE,
                          mapping.c_str()) != 0) {
        APP_LOG(ERROR) << "Fail to add http_svc";
        return -1;
    }

    // Start the server.
    BRPC_NAMESPACE::ServerOptions options;
    options.idle_timeout_sec = FLAGS_idle_timeout_s;
    options.max_concurrency = FLAGS_max_concurrency;
    options.internal_port = FLAGS_internal_port;
    options.thread_local_data_factory = http_svc.get_thread_local_data_factory();

    APP_LOG(TRACE) << "Starting server...";
    if (server.Start(FLAGS_port, &options) != 0) {
        APP_LOG(ERROR) << "Fail to start server";
        return -1;
    }

    // Wait until Ctrl-C is pressed, then Stop() and Join() the server.
    server.RunUntilAskedToQuit();
    return 0;
}
