#include <algorithm>
#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "ax_fs.hpp"
#include "cmdline.hpp"

#include "app/http_api_server.hpp"
#include "app/pipeline_service.hpp"
#include "config_loader.hpp"

#include "common/ax_system.h"

namespace {

std::atomic<bool> g_stop{false};

void HandleSignal(int) {
    g_stop.store(true, std::memory_order_relaxed);
}

bool CheckInputUri(const std::string& uri, std::string* error) {
    if (uri.rfind("rtsp://", 0) == 0 || uri.rfind("rtsps://", 0) == 0) return true;
    std::error_code ec;
    if (!axfs::exists(uri, ec)) {
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
    parser.add<std::string>("http_addr", 0, "http bind address (default 127.0.0.1)", false, "127.0.0.1");
    parser.add<int>("http_port", 0, "http port (0 = disable)", false, 0);
    parser.add<std::string>("http_token", 0, "http bearer token (optional)", false, "");

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
    std::signal(SIGPIPE, SIG_IGN);
#endif

    if (cfg.system.enable_vdec) {
        cfg.system.vdec_max_group_count =
            std::max<std::uint32_t>(cfg.system.vdec_max_group_count,
                                    static_cast<std::uint32_t>(cfg.pipelines.size()));
    }

    const int http_port = parser.get<int>("http_port");
    if (http_port > 0 && !cfg.system.enable_venc) {
        std::cerr << "[http] enabling VENC for preview.jpg\n";
        cfg.system.enable_venc = true;
    }

    if (!axvsdk::common::InitializeSystem(cfg.system)) {
        std::cerr << "InitializeSystem failed\n";
        return 4;
    }
    struct Guard { ~Guard() { axvsdk::common::ShutdownSystem(); } } guard;

    axpipeline::app::PipelineService service;
    for (const auto& p : cfg.pipelines) {
        if (!service.AddPipeline(p, true, &err)) {
            std::cerr << "AddPipeline failed: name=" << p.name << " err=" << err << "\n";
            service.StopAll();
            return 5;
        }
    }

    axpipeline::app::HttpApiServer http(&service);
    if (http_port > 0) {
        axpipeline::app::HttpServerOptions hopts{};
        hopts.bind_addr = parser.get<std::string>("http_addr");
        hopts.port = http_port;
        hopts.bearer_token = parser.get<std::string>("http_token");
        if (!http.Start(hopts, &err)) {
            std::cerr << "HTTP server start failed: " << err << "\n";
            service.StopAll();
            return 6;
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
            const auto snaps = service.ListSnapshots();
            for (const auto& s : snaps) {
                std::cout << "[stats name=" << s.name << " dev=" << s.device_id << "] decoded=" << s.stats.decoded_frames
                          << " submit_fail=" << s.stats.branch_submit_failures
                          << " npu_ok=" << s.npu_ok
                          << " npu_err=" << s.npu_err;
                for (std::size_t j = 0; j < s.stats.output_stats.size(); ++j) {
                    const auto& o = s.stats.output_stats[j];
                    std::cout << " out" << j << "{"
                              << "submitted=" << o.submitted_frames
                              << "dropped=" << o.dropped_frames
                              << "encoded_pkts=" << o.encoded_packets
                              << "key_pkts=" << o.key_packets
                              << "q=" << o.current_queue_depth
                              << "/" << o.queue_capacity
                              << "}";
                }
                std::cout << "\n";
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    http.Stop();
    service.StopAll();
    return 0;
}
