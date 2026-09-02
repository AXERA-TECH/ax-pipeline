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
    // system.device_id:HTTP 动态添加/修改的 pipeline 未显式写 device_id 时继承它,
    // 与 config 文件加载路径的行为一致(否则插件落到卡0,多卡下跨卡读数据、检测恒为0)。
    int default_device_id{-1};
    // 启动配置的 system 段(JSON 文本),/api/v1/config/export 原样带出
    std::string system_config_json{"{}"};
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

