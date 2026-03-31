#include "ai/async_infer.hpp"

#include <utility>

namespace axpipeline::ai {

AsyncInfer::AsyncInfer(double max_fps) : limiter_(max_fps) {}

AsyncInfer::~AsyncInfer() {
    Stop();
}

bool AsyncInfer::Init(const AsyncInferOptions& opt, std::string* error) {
    Stop();

    std::string err;
    plugin_ = plugin::CreatePluginClient(opt.plugin_isolation);
    if (!plugin_) {
        if (error) *error = "CreatePluginClient failed";
        return false;
    }
    if (!plugin_->Open(opt.plugin_path, opt.plugin_init_json, opt.device_id, &err)) {
        if (error) *error = err;
        plugin_.reset();
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(mu_);
        stop_ = false;
    }

    th_ = std::thread([this]() { ThreadMain(); });
    return true;
}

void AsyncInfer::Stop() noexcept {
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
        on_result_ = {};
        on_error_ = {};
    }
    if (plugin_) {
        plugin_->Close();
        plugin_.reset();
    }
}

void AsyncInfer::Submit(axvsdk::common::AxImage::Ptr frame, std::uint64_t seq) {
    if (!frame) return;
    {
        std::lock_guard<std::mutex> lock(mu_);
        if (stop_) return;
        // Keep newest only.
        pending_ = std::move(frame);
        pending_seq_ = seq;
    }
    cv_.notify_one();
}

void AsyncInfer::SetCallbacks(ResultCallback on_result, ErrorCallback on_error) {
    std::lock_guard<std::mutex> lock(mu_);
    on_result_ = std::move(on_result);
    on_error_ = std::move(on_error);
}

void AsyncInfer::ThreadMain() {
    for (;;) {
        axvsdk::common::AxImage::Ptr frame;
        std::uint64_t seq = 0;
        ResultCallback on_result;
        ErrorCallback on_error;

        {
            std::unique_lock<std::mutex> lock(mu_);
            cv_.wait(lock, [&]() { return stop_ || pending_ != nullptr; });
            if (stop_) return;
            frame = std::move(pending_);
            seq = pending_seq_;
            pending_seq_ = 0;
            on_result = on_result_;
            on_error = on_error_;
        }

        if (!frame) continue;

        limiter_.Throttle();

        std::vector<Detection> dets;
        std::string err;
        bool ok = false;
        try {
            ok = plugin_ && plugin_->Infer(*frame, &dets, &err);
        } catch (const std::exception& e) {
            ok = false;
            err = std::string("plugin client threw: ") + e.what();
        } catch (...) {
            ok = false;
            err = "plugin client threw";
        }
        if (!ok) {
            if (on_error) on_error(err, seq);
            continue;
        }
        if (on_result) on_result(dets, seq);
    }
}

}  // namespace axpipeline::ai
