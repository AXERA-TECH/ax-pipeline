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

#include "ai/async_infer.hpp"
#include "ai/ax_resize_map.hpp"
#include "tracking/ax_bytetrack.hpp"

namespace {

std::atomic<bool> g_stop{false};

struct FrameInfo {
    // Decoder/source space (OSD drawing space).
    std::atomic<std::uint32_t> source_w{0};
    std::atomic<std::uint32_t> source_h{0};
    // Inference input space (frame_output callback space).
    std::atomic<std::uint32_t> infer_w{0};
    std::atomic<std::uint32_t> infer_h{0};
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

bool CheckInputUri(const std::string& uri, std::string* error) {
    if (uri.rfind("rtsp://", 0) == 0 || uri.rfind("rtsps://", 0) == 0) return true;
    std::error_code ec;
    if (!std::filesystem::exists(uri, ec)) {
        if (error) *error = "input path not found: " + uri;
        return false;
    }
    return true;
}

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
        if (!CheckInputUri(p.sdk.input.uri, &err)) {
            std::cerr << err << "\n";
            return 3;
        }
    }

    std::signal(SIGINT, HandleSignal);
    std::signal(SIGTERM, HandleSignal);
#ifdef SIGPIPE
    // RTSP clients may disconnect at any time; avoid process termination on SIGPIPE.
    std::signal(SIGPIPE, SIG_IGN);
#endif

    if (!axvsdk::common::InitializeSystem(cfg.system)) {
        std::cerr << "InitializeSystem failed\n";
        return 4;
    }
    struct Guard { ~Guard() { axvsdk::common::ShutdownSystem(); } } guard;

    std::vector<std::unique_ptr<axvsdk::pipeline::Pipeline>> pipelines;
    pipelines.reserve(cfg.pipelines.size());
    std::vector<std::shared_ptr<std::atomic<std::uint64_t>>> counters;
    counters.reserve(cfg.pipelines.size());
    std::vector<std::shared_ptr<axpipeline::ai::AsyncInfer>> npu_workers;
    npu_workers.reserve(cfg.pipelines.size());
    std::vector<std::shared_ptr<FrameInfo>> frame_infos;
    frame_infos.reserve(cfg.pipelines.size());
    std::vector<std::shared_ptr<std::atomic<bool>>> dumped_first_frames;
    dumped_first_frames.reserve(cfg.pipelines.size());

    for (std::size_t i = 0; i < cfg.pipelines.size(); ++i) {
        auto& p = cfg.pipelines[i];

        auto pipe = axvsdk::pipeline::CreatePipeline();
        if (!pipe) {
            std::cerr << "CreatePipeline failed\n";
            return 5;
        }
        if (!pipe->Open(p.sdk)) {
            std::cerr << "pipeline Open failed: name=" << p.name << " idx=" << i << "\n";
            return 6;
        }

        std::shared_ptr<axpipeline::ai::AsyncInfer> npu_worker;
        std::shared_ptr<axpipeline::tracking::ByteTrack> tracker;
        auto fi = std::make_shared<FrameInfo>();
        if (p.npu.enable) {
            axpipeline::ai::AsyncInferOptions nopt{};
            nopt.device_id = p.device_id;
            nopt.plugin_path = p.npu.ax_plugin_path;
            nopt.plugin_init_json = p.npu.ax_plugin_init_json;
            if (p.npu.ax_plugin_isolation == "process") {
                nopt.plugin_isolation = axpipeline::plugin::AxPluginIsolationMode::kSubprocess;
            } else {
                nopt.plugin_isolation = axpipeline::plugin::AxPluginIsolationMode::kInProcess;
            }

            npu_worker = std::make_shared<axpipeline::ai::AsyncInfer>(p.npu_max_fps);
            if (!npu_worker->Init(nopt, &err)) {
                std::cerr << "NPU init failed (ignored): pipeline=" << p.name << " err=" << err << "\n";
                npu_worker.reset();
            }

            if (npu_worker && p.npu.enable_tracking) {
                const auto stream = pipe->GetInputStreamInfo();
                const int fps = stream.frame_rate > 0.0 ? static_cast<int>(std::lround(stream.frame_rate)) : 30;
                axpipeline::tracking::ByteTrackOptions topt{};
                topt.frame_rate = fps > 0 ? fps : 30;
                topt.track_buffer = p.npu.track_buffer > 0 ? static_cast<int>(p.npu.track_buffer) : 30;
                topt.min_score = 0.0F;
                tracker = std::make_shared<axpipeline::tracking::ByteTrack>(topt);
            }

            if (npu_worker) {
                auto* pipe_ptr = pipe.get();
                const auto resize_opts = p.sdk.frame_output.resize;
                npu_worker->SetCallbacks(
                [name = p.name,
                 idx = i,
                 pipe_ptr,
                 fi,
                 resize_opts,
                 enable_osd = p.npu.enable_osd,
                 tracker](const std::vector<axpipeline::ai::Detection>& dets_infer, std::uint64_t seq) {
                    std::vector<axpipeline::ai::Detection> dets = dets_infer;
                    const auto sw = fi->source_w.load(std::memory_order_relaxed);
                    const auto sh = fi->source_h.load(std::memory_order_relaxed);
                    const auto iw = fi->infer_w.load(std::memory_order_relaxed);
                    const auto ih = fi->infer_h.load(std::memory_order_relaxed);
                    if (sw != 0 && sh != 0 && iw != 0 && ih != 0) {
                        const auto map = axpipeline::ai::ComputeInferToSourceMap(sw, sh, iw, ih, resize_opts);
                        axpipeline::ai::MapDetectionsInferToSource(map, sw, sh, &dets);
                    }
                    if (enable_osd && pipe_ptr) {
                        axvsdk::common::DrawFrame osd{};
                        // Keep OSD visible for multiple frames to avoid "blinking" when NPU runs slower than video.
                        osd.hold_frames = 10;
                        if (sw != 0 && sh != 0) {
                            if (tracker) {
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
                                    r.color = axpipeline::tracking::ByteTrack::ColorForTrackId(
                                        static_cast<std::uint64_t>(t.track_id));
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
                                    r.color = 0x00FF00;
                                    osd.rects.push_back(r);
                                }
                            }
                        }
                        if (!osd.rects.empty()) {
                            (void)pipe_ptr->SetOsd(osd);
                        }
                    }
                    if ((seq % 30) == 0) {
                        std::cout << "[npu pipeline=" << name << " idx=" << idx << "] seq=" << seq
                                  << " dets=" << dets_infer.size() << "\n";
                        if (EnvFlagEnabled("AXP_NPU_INFO")) {
                            const std::size_t limit = std::min<std::size_t>(dets.size(), 5U);
                            for (std::size_t di = 0; di < limit; ++di) {
                                const auto& d = dets[di];
                                std::cout << "  det" << di
                                          << " cls=" << d.class_id
                                          << " score=" << d.score
                                          << " box=(" << d.x0 << "," << d.y0 << ")-(" << d.x1 << "," << d.y1 << ")\n";
                            }
                        }
                    }
                },
                [name = p.name, idx = i](const std::string& e, std::uint64_t seq) {
                    std::cerr << "[npu pipeline=" << name << " idx=" << idx << "] seq=" << seq
                              << " error=" << e << "\n";
                });
            }
        }
        npu_workers.push_back(npu_worker);
        frame_infos.push_back(fi);
        auto dumped = std::make_shared<std::atomic<bool>>(false);
        dumped_first_frames.push_back(dumped);

        auto counter = std::make_shared<std::atomic<std::uint64_t>>(0);
        counters.push_back(counter);

        {
            const auto stream = pipe->GetInputStreamInfo();
            if (stream.width != 0 && stream.height != 0) {
                fi->source_w.store(stream.width, std::memory_order_relaxed);
                fi->source_h.store(stream.height, std::memory_order_relaxed);
            }
        }

        pipe->SetFrameCallback([p, idx = i, counter = std::move(counter), npu_worker, fi, dumped](axvsdk::common::AxImage::Ptr frame) {
            if (!frame) return;
            fi->infer_w.store(frame->width(), std::memory_order_relaxed);
            fi->infer_h.store(frame->height(), std::memory_order_relaxed);
            if (fi->source_w.load(std::memory_order_relaxed) == 0 &&
                p.sdk.frame_output.output_image.width == 0 &&
                p.sdk.frame_output.output_image.height == 0) {
                // Fallback: if demux didn't provide stream geometry yet and frame_output follows source,
                // treat callback frame as source space.
                fi->source_w.store(frame->width(), std::memory_order_relaxed);
                fi->source_h.store(frame->height(), std::memory_order_relaxed);
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

        pipelines.push_back(std::move(pipe));
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
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    for (auto& w : npu_workers) {
        if (w) w->Stop();
    }
    for (auto& p : pipelines) p->Stop();
    for (auto& p : pipelines) p->Close();
    return 0;
}
