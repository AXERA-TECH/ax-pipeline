#include "npu/async_detector.hpp"

#include <utility>

namespace axpipeline::npu {

AsyncDetector::AsyncDetector(double max_fps) : limiter_(max_fps) {}

AsyncDetector::~AsyncDetector() {
    Stop();
}

bool AsyncDetector::Init(const AsyncDetectorOptions& opt, std::string* error) {
    Stop();

    std::unique_ptr<AxModelBase> model;
    if (opt.model_type == DetModelType::kYoloV5) {
        auto m = std::make_unique<AxModelYoloV5>();
        if (!m->Init(opt.yolo, error)) {
            return false;
        }
        model = std::move(m);
    } else {
        auto m = std::make_unique<AxModelYoloV8Native>();
        if (!m->Init(opt.yolo, error)) {
            return false;
        }
        model = std::move(m);
    }

    {
        std::lock_guard<std::mutex> lock(mu_);
        model_ = std::move(model);
        stop_ = false;
    }

    th_ = std::thread([this]() { ThreadMain(); });
    return true;
}

void AsyncDetector::Stop() {
    {
        std::lock_guard<std::mutex> lock(mu_);
        stop_ = true;
        pending_.reset();
        pending_seq_ = 0;
    }
    cv_.notify_all();
    if (th_.joinable()) {
        th_.join();
    }
    {
        std::lock_guard<std::mutex> lock(mu_);
        model_.reset();
    }
}

void AsyncDetector::Submit(axvsdk::common::AxImage::Ptr frame, std::uint64_t seq) {
    if (!frame) return;
    {
        std::lock_guard<std::mutex> lock(mu_);
        if (stop_ || !model_) return;
        // Keep newest only.
        pending_ = std::move(frame);
        pending_seq_ = seq;
    }
    cv_.notify_one();
}

void AsyncDetector::SetCallbacks(ResultCallback on_result, ErrorCallback on_error) {
    std::lock_guard<std::mutex> lock(mu_);
    on_result_ = std::move(on_result);
    on_error_ = std::move(on_error);
}

const ModelInputSpec& AsyncDetector::input_spec() const noexcept {
    std::lock_guard<std::mutex> lock(mu_);
    static const ModelInputSpec kEmpty{};
    if (!model_) return kEmpty;
    return model_->input_spec();
}

void AsyncDetector::ThreadMain() {
    for (;;) {
        axvsdk::common::AxImage::Ptr frame;
        std::uint64_t seq = 0;
        AxModelBase* model = nullptr;
        ResultCallback on_result;
        ErrorCallback on_error;

        {
            std::unique_lock<std::mutex> lock(mu_);
            cv_.wait(lock, [&]() { return stop_ || pending_ != nullptr; });
            if (stop_) return;
            frame = std::move(pending_);
            seq = pending_seq_;
            pending_seq_ = 0;

            model = model_.get();
            on_result = on_result_;
            on_error = on_error_;
        }

        if (!frame || model == nullptr) {
            continue;
        }

        limiter_.Throttle();

        std::vector<Detection> dets;
        std::string err;
        if (!model->Infer(*frame, &dets, &err, nullptr)) {
            if (on_error) on_error(err, seq);
            continue;
        }
        if (on_result) on_result(dets, seq);
    }
}

}  // namespace axpipeline::npu
