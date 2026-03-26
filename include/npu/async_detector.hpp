#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "common/ax_image.h"
#include "fps_controller.hpp"
#include "npu/models/ax_model_det.hpp"

namespace axpipeline::npu {

enum class DetModelType {
    kYoloV5 = 0,
    kYoloV8Native,
};

struct AsyncDetectorOptions {
    DetModelType model_type{DetModelType::kYoloV8Native};
    YoloDetOptions yolo{};
};

class AsyncDetector {
public:
    using ResultCallback = std::function<void(const std::vector<Detection>& dets, std::uint64_t seq)>;
    using ErrorCallback = std::function<void(const std::string& error, std::uint64_t seq)>;

    explicit AsyncDetector(double max_fps);
    ~AsyncDetector();

    bool Init(const AsyncDetectorOptions& opt, std::string* error);
    void Stop();

    void Submit(axvsdk::common::AxImage::Ptr frame, std::uint64_t seq);
    void SetCallbacks(ResultCallback on_result, ErrorCallback on_error);

    const ModelInputSpec& input_spec() const noexcept;

private:
    void ThreadMain();

    mutable std::mutex mu_;
    std::condition_variable cv_;
    bool stop_{false};

    axvsdk::common::AxImage::Ptr pending_;
    std::uint64_t pending_seq_{0};

    FpsController limiter_;
    std::unique_ptr<AxModelBase> model_;

    ResultCallback on_result_;
    ErrorCallback on_error_;

    std::thread th_;
};

}  // namespace axpipeline::npu

