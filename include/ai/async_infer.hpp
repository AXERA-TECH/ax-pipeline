#pragma once

#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "ai/ax_detection.hpp"
#include "ax_plugin/ax_plugin_client.hpp"
#include "common/ax_image.h"
#include "fps_controller.hpp"

namespace axpipeline::ai {

struct AsyncInferOptions {
    std::string plugin_path;
    std::string plugin_init_json;
    std::int32_t device_id{-1};
    plugin::AxPluginIsolationMode plugin_isolation{plugin::AxPluginIsolationMode::kInProcess};
};

class AsyncInfer {
public:
    using ResultCallback = std::function<void(const std::vector<Detection>& dets, std::uint64_t seq)>;
    using ErrorCallback = std::function<void(const std::string& error, std::uint64_t seq)>;

    explicit AsyncInfer(double max_fps);
    ~AsyncInfer();

    AsyncInfer(const AsyncInfer&) = delete;
    AsyncInfer& operator=(const AsyncInfer&) = delete;

    bool Init(const AsyncInferOptions& opt, std::string* error);
    void Stop() noexcept;

    void Submit(axvsdk::common::AxImage::Ptr frame, std::uint64_t seq);
    void SetCallbacks(ResultCallback on_result, ErrorCallback on_error);

private:
    void ThreadMain();

    mutable std::mutex mu_;
    std::condition_variable cv_;
    bool stop_{false};

    axvsdk::common::AxImage::Ptr pending_;
    std::uint64_t pending_seq_{0};

    FpsController limiter_;
    std::unique_ptr<plugin::AxPluginClient> plugin_;

    ResultCallback on_result_;
    ErrorCallback on_error_;

    std::thread th_;
};

}  // namespace axpipeline::ai
