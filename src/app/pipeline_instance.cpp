#include "app/pipeline_instance.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <utility>

#include "ai/ax_resize_map.hpp"
#include "codec/ax_jpeg_codec.h"
#include "common/ax_drawer.h"
#include "common/ax_image_processor.h"

namespace axpipeline::app {

namespace {

bool EnvFlagEnabled(const char* name) {
    const char* v = std::getenv(name);
    if (!v) return false;
    const std::string s(v);
    return s == "1" || s == "true" || s == "TRUE" || s == "yes" || s == "YES";
}

std::uint32_t AlignEven(std::uint32_t v) noexcept {
    return (v % 2U) ? (v - 1U) : v;
}

}  // namespace

PipelineInstance::PipelineInstance(ConfigLoader::PipelineCfg cfg)
    : cfg_(std::move(cfg)),
      npu_ok_(std::make_shared<std::atomic<std::uint64_t>>(0)),
      npu_err_(std::make_shared<std::atomic<std::uint64_t>>(0)),
      frame_counter_(std::make_shared<std::atomic<std::uint64_t>>(0)),
      frame_info_(std::make_shared<FrameInfo>()) {}

PipelineInstance::~PipelineInstance() {
    Stop();
    Close();
}

std::string PipelineInstance::name() const {
    std::lock_guard<std::mutex> lock(mu_);
    return cfg_.name;
}

ConfigLoader::PipelineCfg PipelineInstance::config() const {
    std::lock_guard<std::mutex> lock(mu_);
    return cfg_;
}

bool PipelineInstance::BuildPipeline(std::string* error) {
    auto p = axvsdk::pipeline::CreatePipeline();
    if (!p) {
        if (error) *error = "CreatePipeline failed";
        return false;
    }
    if (!p->Open(cfg_.sdk)) {
        if (error) *error = "pipeline Open failed";
        return false;
    }

    pipe_ = std::move(p);

    // Best-effort: populate source geometry early.
    {
        const auto stream = pipe_->GetInputStreamInfo();
        if (stream.width != 0 && stream.height != 0) {
            frame_info_->source_w.store(stream.width, std::memory_order_relaxed);
            frame_info_->source_h.store(stream.height, std::memory_order_relaxed);
        }
    }

    BuildFrameCallback();
    return true;
}

void PipelineInstance::BuildFrameCallback() {
    auto* pipe_ptr = pipe_.get();
    const auto p = cfg_;
    const auto counter = frame_counter_;
    const auto npu_worker = npu_worker_;
    const auto fi = frame_info_;

    pipe_ptr->SetFrameCallback([p, counter, npu_worker, fi](axvsdk::common::AxImage::Ptr frame) {
        if (!frame) return;
        fi->infer_w.store(frame->width(), std::memory_order_relaxed);
        fi->infer_h.store(frame->height(), std::memory_order_relaxed);
        if (fi->source_w.load(std::memory_order_relaxed) == 0 &&
            p.sdk.frame_output.output_image.width == 0 &&
            p.sdk.frame_output.output_image.height == 0) {
            fi->source_w.store(frame->width(), std::memory_order_relaxed);
            fi->source_h.store(frame->height(), std::memory_order_relaxed);
        }

        const auto n = counter->fetch_add(1, std::memory_order_relaxed) + 1;

        const auto every = p.log_every_n_frames == 0 ? 30U : p.log_every_n_frames;
        if ((n % every) == 0) {
            std::cout << "[pipeline=" << p.name << " dev=" << p.device_id << "] "
                      << "frame=" << n
                      << " fmt=" << static_cast<int>(frame->format())
                      << " w=" << frame->width()
                      << " h=" << frame->height()
                      << " stride0=" << frame->stride(0)
                      << " stride1=" << frame->stride(1)
                      << " mem=" << static_cast<int>(frame->memory_type())
                      << " phy0=0x" << std::hex << frame->physical_address(0) << std::dec
                      << " phy1=0x" << std::hex << frame->physical_address(1) << std::dec;
            if (EnvFlagEnabled("AXP_NPU_INFO")) {
                std::cout << " vir0=0x" << std::hex
                          << reinterpret_cast<std::uintptr_t>(frame->virtual_address(0))
                          << " vir1=0x"
                          << reinterpret_cast<std::uintptr_t>(frame->virtual_address(1))
                          << std::dec;
            }
            std::cout << "\n";
        }

        if (npu_worker) {
            npu_worker->Submit(std::move(frame), n);
        }
    });
}

void PipelineInstance::StoreLastDetections(std::vector<ai::Detection> dets, std::uint64_t seq) noexcept {
    last_det_seq_ = seq;
    last_dets_ = std::move(dets);
}

std::vector<ai::Detection> PipelineInstance::GetLastDetectionsLocked() const {
    return last_dets_;
}

void PipelineInstance::StartNpuIfEnabled() {
    if (!pipe_) return;
    if (!cfg_.npu.enable) {
        npu_worker_.reset();
        tracker_.reset();
        return;
    }

    ai::AsyncInferOptions nopt{};
    nopt.device_id = cfg_.device_id;
    nopt.plugin_path = cfg_.npu.ax_plugin_path;
    nopt.plugin_init_json = cfg_.npu.ax_plugin_init_json;
    if (cfg_.npu.ax_plugin_isolation == "process") {
        nopt.plugin_isolation = plugin::AxPluginIsolationMode::kSubprocess;
    } else {
        nopt.plugin_isolation = plugin::AxPluginIsolationMode::kInProcess;
    }

    auto worker = std::make_shared<ai::AsyncInfer>(cfg_.npu_max_fps);
    std::string err;
    if (!worker->Init(nopt, &err)) {
        std::cerr << "NPU init failed (ignored): pipeline=" << cfg_.name << " err=" << err << "\n";
        worker.reset();
    }

    std::shared_ptr<tracking::ByteTrack> tracker;
    if (worker && cfg_.npu.enable_tracking) {
        const auto stream = pipe_->GetInputStreamInfo();
        const int fps = stream.frame_rate > 0.0 ? static_cast<int>(std::lround(stream.frame_rate)) : 30;
        tracking::ByteTrackOptions topt{};
        topt.frame_rate = fps > 0 ? fps : 30;
        topt.track_buffer = cfg_.npu.track_buffer > 0 ? static_cast<int>(cfg_.npu.track_buffer) : 30;
        topt.min_score = 0.0F;
        tracker = std::make_shared<tracking::ByteTrack>(topt);
    }

    if (worker) {
        npu_ok_->store(0, std::memory_order_relaxed);
        npu_err_->store(0, std::memory_order_relaxed);

        auto* pipe_ptr = pipe_.get();
        auto fi = frame_info_;
        const auto resize_opts = cfg_.sdk.frame_output.resize;
        const bool enable_osd = cfg_.npu.enable_osd;

        worker->SetCallbacks(
            [this,
             name = cfg_.name,
             pipe_ptr,
             fi,
             resize_opts,
             enable_osd,
             ok = npu_ok_,
             tracker](const std::vector<ai::Detection>& dets_infer, std::uint64_t seq) {
                if (ok) ok->fetch_add(1, std::memory_order_relaxed);
                {
                    std::lock_guard<std::mutex> lock(mu_);
                    StoreLastDetections(std::vector<ai::Detection>(dets_infer.begin(), dets_infer.end()), seq);
                }

                std::vector<ai::Detection> dets = dets_infer;
                const auto sw = fi->source_w.load(std::memory_order_relaxed);
                const auto sh = fi->source_h.load(std::memory_order_relaxed);
                const auto iw = fi->infer_w.load(std::memory_order_relaxed);
                const auto ih = fi->infer_h.load(std::memory_order_relaxed);
                if (sw != 0 && sh != 0 && iw != 0 && ih != 0) {
                    const auto map = ai::ComputeInferToSourceMap(sw, sh, iw, ih, resize_opts);
                    ai::MapDetectionsInferToSource(map, sw, sh, &dets);
                }

                if (enable_osd && pipe_ptr) {
                    axvsdk::common::DrawFrame osd{};
                    osd.hold_frames = 10;
                    if (sw != 0 && sh != 0) {
                        const bool plugin_has_track_id = std::any_of(
                            dets.begin(), dets.end(), [](const ai::Detection& d) { return d.track_id >= 0; });
                        if (tracker && !plugin_has_track_id) {
                            const auto tracks = tracker->Update(dets);
                            osd.rects.reserve(tracks.size());
                            for (const auto& t : tracks) {
                                float x0f = t.x0;
                                float y0f = t.y0;
                                float x1f = t.x1;
                                float y1f = t.y1;
                                if (x1f < x0f) std::swap(x0f, x1f);
                                if (y1f < y0f) std::swap(y0f, y1f);
                                x0f = std::max(0.0F, std::min(x0f, static_cast<float>(sw - 1)));
                                y0f = std::max(0.0F, std::min(y0f, static_cast<float>(sh - 1)));
                                x1f = std::max(0.0F, std::min(x1f, static_cast<float>(sw - 1)));
                                y1f = std::max(0.0F, std::min(y1f, static_cast<float>(sh - 1)));
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
                                r.color = tracking::ByteTrack::ColorForTrackId(static_cast<std::uint64_t>(t.track_id));
                                osd.rects.push_back(r);
                            }
                        } else {
                            osd.rects.reserve(dets.size());
                            for (const auto& d : dets) {
                                if (d.score < 0.01F) continue;
                                float x0f = d.x0;
                                float y0f = d.y0;
                                float x1f = d.x1;
                                float y1f = d.y1;
                                if (x1f < x0f) std::swap(x0f, x1f);
                                if (y1f < y0f) std::swap(y0f, y1f);
                                x0f = std::max(0.0F, std::min(x0f, static_cast<float>(sw - 1)));
                                y0f = std::max(0.0F, std::min(y0f, static_cast<float>(sh - 1)));
                                x1f = std::max(0.0F, std::min(x1f, static_cast<float>(sw - 1)));
                                y1f = std::max(0.0F, std::min(y1f, static_cast<float>(sh - 1)));
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
                                if (d.track_id >= 0) {
                                    r.color = tracking::ByteTrack::ColorForTrackId(static_cast<std::uint64_t>(d.track_id));
                                } else {
                                    r.color = 0x00FF00;
                                }
                                osd.rects.push_back(r);
                            }
                        }
                    }
                    if (!osd.rects.empty()) {
                        (void)pipe_ptr->SetOsd(osd);
                    }
                }

                if ((seq % 30) == 0) {
                    std::cout << "[npu pipeline=" << name << "] seq=" << seq << " dets=" << dets_infer.size() << "\n";
                }
            },
            [name = cfg_.name, errc = npu_err_](const std::string& e, std::uint64_t seq) {
                if (errc) errc->fetch_add(1, std::memory_order_relaxed);
                std::cerr << "[npu pipeline=" << name << "] seq=" << seq << " error=" << e << "\n";
            });
    }

    npu_worker_ = std::move(worker);
    tracker_ = std::move(tracker);
    BuildFrameCallback();
}

void PipelineInstance::StopNpu() noexcept {
    if (npu_worker_) {
        npu_worker_->Stop();
    }
    npu_worker_.reset();
    tracker_.reset();
}

void PipelineInstance::ClearOsdIfAny() noexcept {
    if (pipe_) {
        pipe_->ClearOsd();
    }
}

bool PipelineInstance::Open(std::string* error) {
    std::lock_guard<std::mutex> lock(mu_);
    if (pipe_) return true;
    if (!BuildPipeline(error)) return false;
    StartNpuIfEnabled();
    return true;
}

bool PipelineInstance::Start(std::string* error) {
    std::lock_guard<std::mutex> lock(mu_);
    if (!pipe_) {
        if (!BuildPipeline(error)) return false;
        StartNpuIfEnabled();
    }
    if (running_) return true;
    if (!pipe_->Start()) {
        if (error) *error = "pipeline Start failed";
        return false;
    }
    running_ = true;
    return true;
}

void PipelineInstance::Stop() noexcept {
    std::lock_guard<std::mutex> lock(mu_);
    running_ = false;
    StopNpu();
    if (pipe_) {
        pipe_->Stop();
    }
}

void PipelineInstance::Close() noexcept {
    std::lock_guard<std::mutex> lock(mu_);
    running_ = false;
    StopNpu();
    if (pipe_) {
        pipe_->Stop();
        pipe_->Close();
        pipe_.reset();
    }
    last_dets_.clear();
    last_det_seq_ = 0;
    frame_counter_->store(0, std::memory_order_relaxed);
    frame_info_->source_w.store(0, std::memory_order_relaxed);
    frame_info_->source_h.store(0, std::memory_order_relaxed);
    frame_info_->infer_w.store(0, std::memory_order_relaxed);
    frame_info_->infer_h.store(0, std::memory_order_relaxed);
}

bool PipelineInstance::Reconfigure(const ConfigLoader::PipelineCfg& cfg, bool autostart, std::string* error) {
    std::lock_guard<std::mutex> lock(mu_);
    const auto old_cfg = cfg_;
    const bool was_running = running_;

    // Stop current.
    running_ = false;
    StopNpu();
    ClearOsdIfAny();
    if (pipe_) {
        pipe_->Stop();
        pipe_->Close();
        pipe_.reset();
    }

    cfg_ = cfg;
    frame_counter_->store(0, std::memory_order_relaxed);
    last_dets_.clear();
    last_det_seq_ = 0;

    std::string open_err;
    if (!BuildPipeline(&open_err)) {
        // Rollback best-effort.
        cfg_ = old_cfg;
        (void)BuildPipeline(nullptr);
        StartNpuIfEnabled();
        if (was_running && pipe_) {
            (void)pipe_->Start();
            running_ = true;
        }
        if (error) *error = open_err.empty() ? "reconfigure open failed" : open_err;
        return false;
    }

    StartNpuIfEnabled();
    if (autostart && was_running) {
        if (!pipe_->Start()) {
            const std::string start_err = "reconfigure start failed";
            // Rollback to old config if we cannot start the new one.
            StopNpu();
            ClearOsdIfAny();
            if (pipe_) {
                pipe_->Stop();
                pipe_->Close();
                pipe_.reset();
            }
            cfg_ = old_cfg;
            (void)BuildPipeline(nullptr);
            StartNpuIfEnabled();
            if (was_running && pipe_) {
                (void)pipe_->Start();
                running_ = true;
            }
            if (error) *error = start_err;
            return false;
        }
        running_ = true;
    }
    return true;
}

bool PipelineInstance::UpdateNpu(const ConfigLoader::PipelineCfg::NpuCfg& npu,
                                double npu_max_fps,
                                bool autostart,
                                std::string* error) {
    std::lock_guard<std::mutex> lock(mu_);
    const bool was_running = running_;

    cfg_.npu = npu;
    cfg_.npu_max_fps = npu_max_fps;

    StopNpu();
    ClearOsdIfAny();
    StartNpuIfEnabled();

    if (autostart && was_running && pipe_ && !running_) {
        if (!pipe_->Start()) {
            if (error) *error = "pipeline Start failed after npu update";
            return false;
        }
        running_ = true;
    }
    return true;
}

PipelineSnapshot PipelineInstance::Snapshot() const {
    std::lock_guard<std::mutex> lock(mu_);
    PipelineSnapshot s{};
    s.name = cfg_.name;
    s.device_id = cfg_.device_id;
    s.running = running_;
    if (pipe_) {
        s.stats = pipe_->GetStats();
    }
    s.npu_ok = npu_ok_ ? npu_ok_->load(std::memory_order_relaxed) : 0;
    s.npu_err = npu_err_ ? npu_err_->load(std::memory_order_relaxed) : 0;
    return s;
}

bool PipelineInstance::AddOutput(const axvsdk::pipeline::PipelineOutputConfig& output,
                                std::size_t* out_index,
                                std::string* error) {
    std::lock_guard<std::mutex> lock(mu_);
    if (!pipe_) {
        if (!BuildPipeline(error)) {
            return false;
        }
        StartNpuIfEnabled();
    }

    std::string err;
    std::size_t idx = 0;
    if (!pipe_->AddOutput(output, &idx, &err)) {
        if (error) *error = err.empty() ? "pipeline AddOutput failed" : err;
        return false;
    }

    cfg_.sdk.outputs.push_back(output);
    if (out_index) {
        *out_index = idx;
    }
    return true;
}

bool PipelineInstance::RemoveOutput(std::size_t index, std::string* error) {
    std::lock_guard<std::mutex> lock(mu_);
    if (!pipe_) {
        if (error) *error = "pipeline not opened";
        return false;
    }

    std::string err;
    if (!pipe_->RemoveOutput(index, &err)) {
        if (error) *error = err.empty() ? "pipeline RemoveOutput failed" : err;
        return false;
    }
    if (index < cfg_.sdk.outputs.size()) {
        cfg_.sdk.outputs.erase(cfg_.sdk.outputs.begin() + static_cast<std::ptrdiff_t>(index));
    }
    return true;
}

bool PipelineInstance::GetPreviewJpeg(const PreviewOptions& opt,
                                     std::vector<std::uint8_t>* out_jpeg,
                                     std::string* error) {
    if (out_jpeg == nullptr) return false;
    out_jpeg->clear();

    std::lock_guard<std::mutex> lock(mu_);
    if (!pipe_) {
        if (error) *error = "pipeline not opened";
        return false;
    }

    auto frame = pipe_->GetLatestFrame();
    if (!frame) {
        if (error) *error = "no latest frame";
        return false;
    }

    const std::uint32_t src_w = frame->width();
    const std::uint32_t src_h = frame->height();

    std::uint32_t dst_w = src_w;
    std::uint32_t dst_h = src_h;
    if (opt.max_width > 0 && opt.max_height > 0 && src_w > 0 && src_h > 0) {
        const double sx = static_cast<double>(opt.max_width) / static_cast<double>(src_w);
        const double sy = static_cast<double>(opt.max_height) / static_cast<double>(src_h);
        const double s = std::min(1.0, std::min(sx, sy));
        dst_w = static_cast<std::uint32_t>(std::lround(static_cast<double>(src_w) * s));
        dst_h = static_cast<std::uint32_t>(std::lround(static_cast<double>(src_h) * s));
        // NV12 requires even dimensions.
        dst_w = std::max<std::uint32_t>(2U, AlignEven(dst_w));
        dst_h = std::max<std::uint32_t>(2U, AlignEven(dst_h));
    }

    // Always generate NV12 preview for consistent drawer/JPEG encode behavior.
    axvsdk::common::ImageDescriptor desc{};
    desc.format = axvsdk::common::PixelFormat::kNv12;
    desc.width = dst_w;
    desc.height = dst_h;

    axvsdk::common::ImageAllocationOptions alloc{};
    alloc.memory_type = axvsdk::common::MemoryType::kCmm;
    alloc.cache_mode = axvsdk::common::CacheMode::kNonCached;
    alloc.token = "ax-pipeline-preview";
    auto preview = axvsdk::common::AxImage::Create(desc, alloc);
    if (!preview) {
        if (error) *error = "alloc preview image failed";
        return false;
    }

    auto proc = axvsdk::common::CreateImageProcessor();
    if (!proc) {
        if (error) *error = "CreateImageProcessor failed";
        return false;
    }
    axvsdk::common::ImageProcessRequest req{};
    req.output_image = desc;
    req.resize.mode = axvsdk::common::ResizeMode::kKeepAspectRatio;
    req.resize.background_color = 0;
    if (!proc->Process(*frame, req, *preview)) {
        if (error) *error = "preview resize/convert failed";
        return false;
    }

    if (opt.with_boxes) {
        auto dets = GetLastDetectionsLocked();
        if (!dets.empty()) {
            const float sx = (src_w > 0) ? (static_cast<float>(dst_w) / static_cast<float>(src_w)) : 1.0F;
            const float sy = (src_h > 0) ? (static_cast<float>(dst_h) / static_cast<float>(src_h)) : 1.0F;

            axvsdk::common::DrawFrame osd{};
            osd.hold_frames = 1;
            osd.rects.reserve(dets.size());
            for (const auto& d : dets) {
                if (d.score < 0.01F) continue;
                float x0f = d.x0 * sx;
                float y0f = d.y0 * sy;
                float x1f = d.x1 * sx;
                float y1f = d.y1 * sy;
                if (x1f < x0f) std::swap(x0f, x1f);
                if (y1f < y0f) std::swap(y0f, y1f);
                x0f = std::max(0.0F, std::min(x0f, static_cast<float>(dst_w - 1)));
                y0f = std::max(0.0F, std::min(y0f, static_cast<float>(dst_h - 1)));
                x1f = std::max(0.0F, std::min(x1f, static_cast<float>(dst_w - 1)));
                y1f = std::max(0.0F, std::min(y1f, static_cast<float>(dst_h - 1)));
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
                r.color = (d.track_id >= 0) ? tracking::ByteTrack::ColorForTrackId(static_cast<std::uint64_t>(d.track_id))
                                            : 0x00FF00;
                osd.rects.push_back(r);
            }

            if (!osd.rects.empty()) {
                auto drawer = axvsdk::common::CreateDrawer();
                if (drawer) {
                    (void)drawer->Draw(osd, *preview);
                }
            }
        }
    }

    axvsdk::codec::JpegEncodeOptions jopt{};
    jopt.quality = static_cast<std::uint32_t>(std::max(1, std::min(100, opt.quality)));
    *out_jpeg = axvsdk::codec::EncodeJpegToMemory(*preview, jopt);
    if (out_jpeg->empty()) {
        if (error) *error = "jpeg encode failed";
        return false;
    }
    return true;
}

}  // namespace axpipeline::app
