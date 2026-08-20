#pragma once

#include <atomic>
#include <memory>
#include <string>
#include <thread>

#include "app/pipeline_service.hpp"

namespace axpipeline::app {

struct HttpServerOptions {
    std::string bind_addr{"127.0.0.1"};
    int port{0};  // 0 = disabled
    std::string bearer_token;  // optional
    // 控制台页面:留空 = 用编进二进制的内嵌页;指定目录 = 优先从磁盘读(方便改 UI 不重编)
    std::string webroot;
};

class HttpApiServer {
public:
    explicit HttpApiServer(PipelineService* service);
    ~HttpApiServer();

    HttpApiServer(const HttpApiServer&) = delete;
    HttpApiServer& operator=(const HttpApiServer&) = delete;

    bool Start(const HttpServerOptions& opt, std::string* error);
    void Stop() noexcept;
    bool running() const noexcept { return running_.load(); }

private:
    class Impl;

    PipelineService* service_{nullptr};
    std::unique_ptr<Impl> impl_;
    std::atomic<bool> running_{false};
};

}  // namespace axpipeline::app

