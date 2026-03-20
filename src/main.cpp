#include <atomic>
#include <chrono>
#include <csignal>
#include <cstddef>
#include <cstdint>
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

namespace {

std::atomic<bool> g_stop{false};

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

    if (!axvsdk::common::InitializeSystem(cfg.system)) {
        std::cerr << "InitializeSystem failed\n";
        return 4;
    }
    struct Guard { ~Guard() { axvsdk::common::ShutdownSystem(); } } guard;

    std::vector<std::unique_ptr<axvsdk::pipeline::Pipeline>> pipelines;
    pipelines.reserve(cfg.pipelines.size());
    std::vector<std::shared_ptr<std::atomic<std::uint64_t>>> counters;
    counters.reserve(cfg.pipelines.size());

    for (std::size_t i = 0; i < cfg.pipelines.size(); ++i) {
        const auto& p = cfg.pipelines[i];
        auto pipe = axvsdk::pipeline::CreatePipeline();
        if (!pipe) {
            std::cerr << "CreatePipeline failed\n";
            return 5;
        }
        if (!pipe->Open(p.sdk)) {
            std::cerr << "pipeline Open failed: name=" << p.name << " idx=" << i << "\n";
            return 6;
        }

        auto counter = std::make_shared<std::atomic<std::uint64_t>>(0);
        counters.push_back(counter);
        auto fps = std::make_shared<FpsController>(p.npu_max_fps);

        pipe->SetFrameCallback([p, idx = i, counter = std::move(counter), fps = std::move(fps)](axvsdk::common::AxImage::Ptr frame) {
            if (!frame) return;
            const auto n = counter->fetch_add(1, std::memory_order_relaxed) + 1;
            const auto every = p.log_every_n_frames == 0 ? 30U : p.log_every_n_frames;
            if ((n % every) == 0) {
                std::cout << "[pipeline=" << p.name << " idx=" << idx << " dev=" << p.device_id << "] "
                          << "frame=" << n
                          << " fmt=" << static_cast<int>(frame->format())
                          << " w=" << frame->width()
                          << " h=" << frame->height()
                          << " stride0=" << frame->stride(0)
                          << " mem=" << static_cast<int>(frame->memory_type())
                          << " phy0=0x" << std::hex << frame->physical_address(0) << std::dec
                          << "\n";
            }

            fps->Throttle();
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

    for (auto& p : pipelines) p->Stop();
    for (auto& p : pipelines) p->Close();
    return 0;
}
