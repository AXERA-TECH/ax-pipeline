#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "ai/async_infer.hpp"
#include "ai/ax_detection.hpp"
#include "config_loader.hpp"
#include "pipeline/ax_pipeline.h"
#include "tracking/ax_bytetrack.hpp"

namespace axpipeline::app {

struct PipelineSnapshot {
    std::string name;
    std::int32_t device_id{-1};
    bool running{false};
    axvsdk::pipeline::PipelineStats stats{};
    std::uint64_t npu_ok{0};
    std::uint64_t npu_err{0};
};

struct PreviewOptions {
    int quality{85};              // 1..100
    std::uint32_t max_width{0};   // 0 = keep
    std::uint32_t max_height{0};  // 0 = keep
    bool with_boxes{false};
};

class PipelineInstance {
public:
    explicit PipelineInstance(ConfigLoader::PipelineCfg cfg);
    ~PipelineInstance();

    PipelineInstance(const PipelineInstance&) = delete;
    PipelineInstance& operator=(const PipelineInstance&) = delete;

    std::string name() const;
    ConfigLoader::PipelineCfg config() const;

    bool Open(std::string* error);
    bool Start(std::string* error);
    void Stop() noexcept;
    void Close() noexcept;

    // Replace the whole pipeline config (may restart decode/encode).
    bool Reconfigure(const ConfigLoader::PipelineCfg& cfg, bool autostart, std::string* error);
    // Update NPU-related config. This may restart the NPU worker, and may optionally restart the pipeline
    // if frame_output changes.
    bool UpdateNpu(const ConfigLoader::PipelineCfg::NpuCfg& npu,
                   double npu_max_fps,
                   bool autostart,
                   std::string* error);

    PipelineSnapshot Snapshot() const;

    // Returns a JPEG preview image (host memory). The preview is based on GetLatestFrame().
    bool GetPreviewJpeg(const PreviewOptions& opt, std::vector<std::uint8_t>* out_jpeg, std::string* error);

    // Add/remove encoding+mux outputs without restarting demux/vdec (if backend supports it).
    bool AddOutput(const axvsdk::pipeline::PipelineOutputConfig& output,
                   std::size_t* out_index,
                   std::string* error);
    bool RemoveOutput(std::size_t index, std::string* error);

private:
    struct FrameInfo {
        std::atomic<std::uint32_t> source_w{0};
        std::atomic<std::uint32_t> source_h{0};
        std::atomic<std::uint32_t> infer_w{0};
        std::atomic<std::uint32_t> infer_h{0};
    };

    bool BuildPipeline(std::string* error);
    void BuildFrameCallback();
    void StartNpuIfEnabled();
    void StopNpu() noexcept;
    void ClearOsdIfAny() noexcept;

    // Caller must hold mu_.
    void StoreLastDetections(std::vector<ai::Detection> dets, std::uint64_t seq) noexcept;
    // Caller must hold mu_.
    std::vector<ai::Detection> GetLastDetectionsLocked() const;

    mutable std::mutex mu_;
    ConfigLoader::PipelineCfg cfg_;
    std::unique_ptr<axvsdk::pipeline::Pipeline> pipe_;
    std::shared_ptr<ai::AsyncInfer> npu_worker_;
    std::shared_ptr<tracking::ByteTrack> tracker_;
    std::shared_ptr<std::atomic<std::uint64_t>> npu_ok_;
    std::shared_ptr<std::atomic<std::uint64_t>> npu_err_;
    std::shared_ptr<std::atomic<std::uint64_t>> frame_counter_;
    std::shared_ptr<FrameInfo> frame_info_;

    std::uint64_t last_det_seq_{0};
    std::vector<ai::Detection> last_dets_;

    bool running_{false};
};

}  // namespace axpipeline::app
