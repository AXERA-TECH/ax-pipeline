#include <atomic>
#include <chrono>
#include <cmath>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "cmdline.hpp"
#include "json.hpp"

#include "config_loader.hpp"
#include "fps_controller.hpp"

#include "common/ax_system.h"
#include "pipeline/ax_pipeline.h"
#include "pipeline/ax_camera_pipeline.h"

#include "npu/async_detector.hpp"
#include "tracking/ax_bytetrack.hpp"

namespace {

std::atomic<bool> g_stop{false};

struct FrameInfo {
    std::atomic<std::uint32_t> w{0};
    std::atomic<std::uint32_t> h{0};
};

bool EnvFlagEnabled(const char* name) {
    const char* v = std::getenv(name);
    if (!v) return false;
    const std::string s(v);
    return s == "1" || s == "true" || s == "TRUE" || s == "yes" || s == "YES";
}

void HandleSignal(int) {
    g_stop.store(true, std::memory_order_relaxed);
}

bool CheckInputUri(axpipeline::ConfigLoader::InputType type, const std::string& uri, std::string* error) {
    if (type == axpipeline::ConfigLoader::InputType::kCamera) {
        std::error_code ec;
        if (!std::filesystem::exists(uri, ec)) {
            if (error) *error = "camera device not found: " + uri;
            return false;
        }
        return true;
    }
    if (uri.rfind("rtsp://", 0) == 0 || uri.rfind("rtsps://", 0) == 0) return true;
    std::error_code ec;
    if (!std::filesystem::exists(uri, ec)) {
        if (error) *error = "input path not found: " + uri;
        return false;
    }
    return true;
}

// Pipeline 统一接口包装
class PipelineWrapper {
public:
    virtual ~PipelineWrapper() = default;
    virtual bool Open() = 0;
    virtual bool Start() = 0;
    virtual void Stop() = 0;
    virtual void Close() = 0;
    virtual void SetFrameCallback(axvsdk::pipeline::FrameCallback cb) = 0;
    virtual bool SetOsd(const axvsdk::common::DrawFrame& osd) = 0;
    virtual axvsdk::pipeline::PipelineStats GetStats() = 0;
    virtual axvsdk::pipeline::CameraPipelineStats GetCameraStats() = 0;
    virtual std::uint32_t GetWidth() = 0;
    virtual std::uint32_t GetHeight() = 0;
    virtual double GetFps() = 0;
    virtual bool IsCamera() const = 0;
};

// 标准 Pipeline 包装
class StandardPipelineWrapper : public PipelineWrapper {
public:
    StandardPipelineWrapper(axvsdk::pipeline::PipelineConfig cfg) : config_(std::move(cfg)) {}

    bool Open() override {
        pipeline_ = axvsdk::pipeline::CreatePipeline();
        if (!pipeline_) return false;
        return pipeline_->Open(config_);
    }

    bool Start() override { return pipeline_->Start(); }
    void Stop() override { pipeline_->Stop(); }
    void Close() override { pipeline_->Close(); }

    void SetFrameCallback(axvsdk::pipeline::FrameCallback cb) override {
        pipeline_->SetFrameCallback(std::move(cb));
    }

    bool SetOsd(const axvsdk::common::DrawFrame& osd) override {
        return pipeline_->SetOsd(osd);
    }

    axvsdk::pipeline::PipelineStats GetStats() override {
        return pipeline_->GetStats();
    }

    axvsdk::pipeline::CameraPipelineStats GetCameraStats() override {
        return {};  // 不适用
    }

    std::uint32_t GetWidth() override {
        auto info = pipeline_->GetInputStreamInfo();
        return info.width;
    }

    std::uint32_t GetHeight() override {
        auto info = pipeline_->GetInputStreamInfo();
        return info.height;
    }

    double GetFps() override {
        auto info = pipeline_->GetInputStreamInfo();
        return info.frame_rate;
    }

    bool IsCamera() const override { return false; }

private:
    axvsdk::pipeline::PipelineConfig config_;
    std::unique_ptr<axvsdk::pipeline::Pipeline> pipeline_;
};

// 摄像头 Pipeline 包装
class CameraPipelineWrapper : public PipelineWrapper {
public:
    CameraPipelineWrapper(axvsdk::pipeline::CameraPipelineConfig cfg) : config_(std::move(cfg)) {}

    bool Open() override {
        pipeline_ = axvsdk::pipeline::CreateCameraPipeline();
        if (!pipeline_) return false;
        return pipeline_->Open(config_);
    }

    bool Start() override { return pipeline_->Start(); }
    void Stop() override { pipeline_->Stop(); }
    void Close() override { pipeline_->Close(); }

    void SetFrameCallback(axvsdk::pipeline::FrameCallback cb) override {
        pipeline_->SetFrameCallback([cb](axvsdk::common::AxImage::Ptr frame) {
            if (cb) cb(std::move(frame));
        });
    }

    bool SetOsd(const axvsdk::common::DrawFrame& osd) override {
        return pipeline_->SetOsd(osd);
    }

    axvsdk::pipeline::PipelineStats GetStats() override {
        return {};  // 不适用
    }

    axvsdk::pipeline::CameraPipelineStats GetCameraStats() override {
        return pipeline_->GetStats();
    }

    std::uint32_t GetWidth() override {
        return pipeline_->GetWidth();
    }

    std::uint32_t GetHeight() override {
        return pipeline_->GetHeight();
    }

    double GetFps() override {
        return pipeline_->GetFps();
    }

    bool IsCamera() const override { return true; }

private:
    axvsdk::pipeline::CameraPipelineConfig config_;
    std::unique_ptr<axvsdk::pipeline::CameraPipeline> pipeline_;
};

}  // namespace

int main(int argc, char** argv) {
    cmdline::parser parser;
    parser.set_program_name("ax_pipeline_app");
    parser.add<std::string>("config", 'c', "config json path", true);
    parser.add<int>("time", 't', "run duration seconds (0 = until SIGINT)", false, 0);

    if (!parser.parse(argc, argv)) {
        std::cerr << parser.usage();
        return 1;
    }

    std::string err;
    axpipeline::ConfigLoader::AppCfg cfg{};
    if (!axpipeline::ConfigLoader::LoadFromFile(parser.get<std::string>("config"), &cfg, &err)) {
        std::cerr << "LoadConfig failed: " << err << "\n";
        return 2;
    }

    for (const auto& p : cfg.pipelines) {
        std::string input_uri = (p.input_type == axpipeline::ConfigLoader::InputType::kCamera) 
            ? p.camera_sdk.device_path : p.sdk.input.uri;
        if (!CheckInputUri(p.input_type, input_uri, &err)) {
            std::cerr << err << "\n";
            return 3;
        }
    }

    std::signal(SIGINT, HandleSignal);
    std::signal(SIGTERM, HandleSignal);
#ifdef SIGPIPE
    std::signal(SIGPIPE, SIG_IGN);
#endif

    if (!axvsdk::common::InitializeSystem(cfg.system)) {
        std::cerr << "InitializeSystem failed\n";
        return 4;
    }
    struct Guard { ~Guard() { axvsdk::common::ShutdownSystem(); } } guard;

    std::vector<std::unique_ptr<PipelineWrapper>> pipelines;
    pipelines.reserve(cfg.pipelines.size());
    std::vector<std::shared_ptr<std::atomic<std::uint64_t>>> counters;
    counters.reserve(cfg.pipelines.size());
    std::vector<std::shared_ptr<axpipeline::npu::AsyncDetector>> npu_workers;
    npu_workers.reserve(cfg.pipelines.size());
    std::vector<std::shared_ptr<FrameInfo>> frame_infos;
    frame_infos.reserve(cfg.pipelines.size());
    std::vector<std::shared_ptr<std::atomic<bool>>> dumped_first_frames;
    dumped_first_frames.reserve(cfg.pipelines.size());

    for (std::size_t i = 0; i < cfg.pipelines.size(); ++i) {
        auto& p = cfg.pipelines[i];

        // 创建对应的 Pipeline 包装
        std::unique_ptr<PipelineWrapper> wrapper;
        if (p.input_type == axpipeline::ConfigLoader::InputType::kCamera) {
            wrapper = std::make_unique<CameraPipelineWrapper>(p.camera_sdk);
        } else {
            wrapper = std::make_unique<StandardPipelineWrapper>(p.sdk);
        }

        if (!wrapper->Open()) {
            std::cerr << "pipeline Open failed: name=" << p.name << " idx=" << i << " type=" 
                      << (p.input_type == axpipeline::ConfigLoader::InputType::kCamera ? "camera" : "standard") << "\n";
            return 6;
        }

        std::shared_ptr<axpipeline::npu::AsyncDetector> npu_worker;
        std::shared_ptr<axpipeline::tracking::ByteTrack> tracker;
        auto fi = std::make_shared<FrameInfo>();
        if (p.npu.enable) {
            axpipeline::npu::AsyncDetectorOptions nopt{};
            nopt.yolo.base.device_id = p.device_id;
            nopt.yolo.base.model_path = p.npu.model_path;
            nopt.yolo.base.resize_mode = axvsdk::common::ResizeMode::kKeepAspectRatio;
            nopt.yolo.base.h_align = axvsdk::common::ResizeAlign::kCenter;
            nopt.yolo.base.v_align = axvsdk::common::ResizeAlign::kCenter;
            nopt.yolo.base.background_color = 0;
            nopt.yolo.num_classes = static_cast<int>(p.npu.num_classes);
            nopt.yolo.conf_threshold = static_cast<float>(p.npu.conf_threshold);
            nopt.yolo.nms_threshold = static_cast<float>(p.npu.nms_threshold);
            if (p.npu.model_type == "yolov5" || p.npu.model_type == "YOLOV5") {
                nopt.model_type = axpipeline::npu::DetModelType::kYoloV5;
            } else {
                nopt.model_type = axpipeline::npu::DetModelType::kYoloV8Native;
            }

            npu_worker = std::make_shared<axpipeline::npu::AsyncDetector>(p.npu_max_fps);
            if (!npu_worker->Init(nopt, &err)) {
                std::cerr << "NPU init failed: pipeline=" << p.name << " err=" << err << "\n";
                return 5;
            }

            if (p.npu.enable_tracking) {
                const int fps = wrapper->GetFps() > 0.0 ? static_cast<int>(std::lround(wrapper->GetFps())) : 30;
                axpipeline::tracking::ByteTrackOptions topt{};
                topt.frame_rate = fps > 0 ? fps : 30;
                topt.track_buffer = p.npu.track_buffer > 0 ? static_cast<int>(p.npu.track_buffer) : 30;
                topt.min_score = static_cast<float>(p.npu.conf_threshold);
                tracker = std::make_shared<axpipeline::tracking::ByteTrack>(topt);
            }
            if (EnvFlagEnabled("AXP_NPU_INFO")) {
                const auto& in = npu_worker->input_spec();
                std::cout << "[npu init pipeline=" << p.name << " dev=" << p.device_id << "] input{fmt="
                          << static_cast<int>(in.format)
                          << " w=" << in.width
                          << " h=" << in.height
                          << "}\n";
            }

            auto* wrapper_ptr = wrapper.get();
            npu_worker->SetCallbacks(
                [name = p.name, idx = i, wrapper_ptr, fi, enable_osd = p.npu.enable_osd, tracker]
                (const std::vector<axpipeline::npu::Detection>& dets, std::uint64_t seq) {
                    if (enable_osd && wrapper_ptr) {
                        const auto fw = fi->w.load(std::memory_order_relaxed);
                        const auto fh = fi->h.load(std::memory_order_relaxed);
                        axvsdk::common::DrawFrame osd{};
                        osd.hold_frames = 10;
                        if (fw != 0 && fh != 0) {
                            if (tracker) {
                                const auto tracks = tracker->Update(dets);
                                osd.rects.reserve(tracks.size());
                                for (const auto& t : tracks) {
                                    float x0f = t.x0, y0f = t.y0, x1f = t.x1, y1f = t.y1;
                                    if (x1f < x0f) std::swap(x0f, x1f);
                                    if (y1f < y0f) std::swap(y0f, y1f);
                                    x0f = std::max(0.0F, std::min(x0f, static_cast<float>(fw - 1)));
                                    y0f = std::max(0.0F, std::min(y0f, static_cast<float>(fh - 1)));
                                    x1f = std::max(0.0F, std::min(x1f, static_cast<float>(fw - 1)));
                                    y1f = std::max(0.0F, std::min(y1f, static_cast<float>(fh - 1)));
                                    const auto w = static_cast<std::int32_t>(x1f - x0f);
                                    const auto h = static_cast<std::int32_t>(y1f - y0f);
                                    if (w <= 1 || h <= 1) continue;
                                    axvsdk::common::DrawRect r{};
                                    r.x = static_cast<std::int32_t>(x0f);
                                    r.y = static_cast<std::int32_t>(y0f);
                                    r.width = static_cast<std::uint32_t>(w);
                                    r.height = static_cast<std::uint32_t>(h);
                                    r.thickness = 2;
                                    r.alpha = 255;
                                    r.color = axpipeline::tracking::ByteTrack::ColorForTrackId(
                                        static_cast<std::uint64_t>(t.track_id));
                                    osd.rects.push_back(r);
                                }
                            } else {
                                osd.rects.reserve(dets.size());
                                for (const auto& d : dets) {
                                    if (d.score < 0.01F) continue;
                                    float x0f = d.x0, y0f = d.y0, x1f = d.x1, y1f = d.y1;
                                    if (x1f < x0f) std::swap(x0f, x1f);
                                    if (y1f < y0f) std::swap(y0f, y1f);
                                    x0f = std::max(0.0F, std::min(x0f, static_cast<float>(fw - 1)));
                                    y0f = std::max(0.0F, std::min(y0f, static_cast<float>(fh - 1)));
                                    x1f = std::max(0.0F, std::min(x1f, static_cast<float>(fw - 1)));
                                    y1f = std::max(0.0F, std::min(y1f, static_cast<float>(fh - 1)));
                                    const auto w = static_cast<std::int32_t>(x1f - x0f);
                                    const auto h = static_cast<std::int32_t>(y1f - y0f);
                                    if (w <= 1 || h <= 1) continue;
                                    axvsdk::common::DrawRect r{};
                                    r.x = static_cast<std::int32_t>(x0f);
                                    r.y = static_cast<std::int32_t>(y0f);
                                    r.width = static_cast<std::uint32_t>(w);
                                    r.height = static_cast<std::uint32_t>(h);
                                    r.thickness = 2;
                                    r.alpha = 255;
                                    r.color = 0x00FF00;
                                    osd.rects.push_back(r);
                                }
                            }
                        }
                        if (!osd.rects.empty()) {
                            (void)wrapper_ptr->SetOsd(osd);
                        }
                    }
                    if ((seq % 30) == 0) {
                        std::cout << "[npu pipeline=" << name << " idx=" << idx << "] seq=" << seq
                                  << " dets=" << dets.size() << "\n";
                    }
                },
                [name = p.name, idx = i](const std::string& e, std::uint64_t seq) {
                    std::cerr << "[npu pipeline=" << name << " idx=" << idx << "] seq=" << seq
                              << " error=" << e << "\n";
                });
        }
        npu_workers.push_back(npu_worker);
        frame_infos.push_back(fi);
        auto dumped = std::make_shared<std::atomic<bool>>(false);
        dumped_first_frames.push_back(dumped);

        auto counter = std::make_shared<std::atomic<std::uint64_t>>(0);
        counters.push_back(counter);

        {
            const auto w = wrapper->GetWidth();
            const auto h = wrapper->GetHeight();
            if (w != 0 && h != 0) {
                fi->w.store(w, std::memory_order_relaxed);
                fi->h.store(h, std::memory_order_relaxed);
            }
        }

        wrapper->SetFrameCallback([p, idx = i, counter = std::move(counter), npu_worker, fi, dumped]
            (axvsdk::common::AxImage::Ptr frame) {
            if (!frame) return;
            if (fi->w.load(std::memory_order_relaxed) == 0) {
                fi->w.store(frame->width(), std::memory_order_relaxed);
            }
            if (fi->h.load(std::memory_order_relaxed) == 0) {
                fi->h.store(frame->height(), std::memory_order_relaxed);
            }
            const auto n = counter->fetch_add(1, std::memory_order_relaxed) + 1;

            const auto every = p.log_every_n_frames == 0 ? 30U : p.log_every_n_frames;
            if ((n % every) == 0) {
                std::cout << "[pipeline=" << p.name << " idx=" << idx << " dev=" << p.device_id << "] "
                          << "frame=" << n
                          << " fmt=" << static_cast<int>(frame->format())
                          << " w=" << frame->width()
                          << " h=" << frame->height()
                          << " stride0=" << frame->stride(0)
                          << " stride1=" << frame->stride(1)
                          << " mem=" << static_cast<int>(frame->memory_type())
                          << " phy0=0x" << std::hex << frame->physical_address(0) << std::dec
                          << " phy1=0x" << std::hex << frame->physical_address(1) << std::dec
                          << "\n";
            }

            if (npu_worker) {
                npu_worker->Submit(std::move(frame), n);
            }
        });

        pipelines.push_back(std::move(wrapper));
    }

    for (std::size_t i = 0; i < pipelines.size(); ++i) {
        if (!pipelines[i]->Start()) {
            std::cerr << "pipeline Start failed: idx=" << i << "\n";
            for (auto& p : pipelines) p->Stop();
            return 7;
        }
    }

    const auto duration_s = parser.get<int>("time");
    const auto deadline = duration_s <= 0 ? std::chrono::steady_clock::time_point::max()
                                          : std::chrono::steady_clock::now() + std::chrono::seconds(duration_s);

    auto last_stats = std::chrono::steady_clock::now();
    while (!g_stop.load(std::memory_order_relaxed) && std::chrono::steady_clock::now() < deadline) {
        const auto now = std::chrono::steady_clock::now();
        if (now - last_stats >= std::chrono::seconds(1)) {
            last_stats = now;
            for (std::size_t i = 0; i < pipelines.size(); ++i) {
                if (pipelines[i]->IsCamera()) {
                    const auto st = pipelines[i]->GetCameraStats();
                    std::cout << "[stats idx=" << i << "] captured=" << st.captured_frames
                              << " submit_fail=" << st.submit_failures;
                    for (std::size_t j = 0; j < st.output_stats.size(); ++j) {
                        std::cout << " out" << j << "{"
                                  << "submitted=" << st.output_stats[j].submitted_frames
                                  << "dropped=" << st.output_stats[j].dropped_frames
                                  << "encoded_pkts=" << st.output_stats[j].encoded_packets
                                  << "key_pkts=" << st.output_stats[j].key_packets
                                  << "q=" << st.output_stats[j].current_queue_depth
                                  << "/" << st.output_stats[j].queue_capacity
                                  << "}";
                    }
                    std::cout << "\n";
                } else {
                    const auto st = pipelines[i]->GetStats();
                    std::cout << "[stats idx=" << i << "] decoded=" << st.decoded_frames
                              << " submit_fail=" << st.branch_submit_failures;
                    for (std::size_t j = 0; j < st.output_stats.size(); ++j) {
                        std::cout << " out" << j << "{"
                                  << "submitted=" << st.output_stats[j].submitted_frames
                                  << "dropped=" << st.output_stats[j].dropped_frames
                                  << "encoded_pkts=" << st.output_stats[j].encoded_packets
                                  << "key_pkts=" << st.output_stats[j].key_packets
                                  << "q=" << st.output_stats[j].current_queue_depth
                                  << "/" << st.output_stats[j].queue_capacity
                                  << "}";
                    }
                    std::cout << "\n";
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    for (auto& w : npu_workers) {
        if (w) w->Stop();
    }
    for (auto& p : pipelines) p->Stop();
    for (auto& p : pipelines) p->Close();
    return 0;
}
