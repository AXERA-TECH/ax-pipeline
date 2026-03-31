#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "ax_plugin/ax_plugin_client.hpp"
#include "common/ax_image.h"

namespace {

axvsdk::common::AxImage::Ptr MakeTestImageBgr(std::uint32_t w, std::uint32_t h) {
    const std::size_t stride = static_cast<std::size_t>(w) * 3U;
    const std::size_t size = stride * static_cast<std::size_t>(h);
    auto buf = std::make_shared<std::vector<std::uint8_t>>(size);
    // Deterministic pattern.
    for (std::size_t i = 0; i < buf->size(); ++i) {
        (*buf)[i] = static_cast<std::uint8_t>(i & 0xFFU);
    }

    axvsdk::common::ImageDescriptor desc{};
    desc.format = axvsdk::common::PixelFormat::kBgr24;
    desc.width = w;
    desc.height = h;
    desc.strides[0] = stride;

    std::array<axvsdk::common::ExternalImagePlane, axvsdk::common::kMaxImagePlanes> planes{};
    planes[0].virtual_address = buf->data();
    planes[0].physical_address = 0;
    planes[0].block_id = axvsdk::common::kInvalidPoolId;

    // Keep buf alive via lifetime holder.
    return axvsdk::common::AxImage::WrapExternal(desc, planes, buf);
}

bool GetArg(int argc, char** argv, const std::string& key, std::string* out) {
    for (int i = 1; i < argc; ++i) {
        if (!argv[i]) continue;
        const std::string s(argv[i]);
        const std::string k = key + "=";
        if (s.rfind(k, 0) == 0) {
            if (out) *out = s.substr(k.size());
            return true;
        }
    }
    return false;
}

}  // namespace

int main(int argc, char** argv) {
    std::string so_path;
    std::string mode = "inproc";
    std::string crash_test = "0";
    int iters = 1;

    (void)GetArg(argc, argv, "--so", &so_path);
    (void)GetArg(argc, argv, "--mode", &mode);
    (void)GetArg(argc, argv, "--crash-test", &crash_test);
    {
        std::string it;
        if (GetArg(argc, argv, "--iters", &it)) {
            try {
                iters = std::stoi(it);
            } catch (...) {
                iters = 1;
            }
        }
        if (iters < 1) iters = 1;
    }

    if (so_path.empty()) {
        std::cerr << "missing --so=/path/to/plugin.so\n";
        return 2;
    }

    axpipeline::plugin::AxPluginIsolationMode iso = axpipeline::plugin::AxPluginIsolationMode::kInProcess;
    if (mode == "process") {
        iso = axpipeline::plugin::AxPluginIsolationMode::kSubprocess;
    }

    auto client = axpipeline::plugin::CreatePluginClient(iso);
    if (!client) {
        std::cerr << "CreatePluginClient failed\n";
        return 3;
    }

    auto img = MakeTestImageBgr(64, 64);
    if (!img) {
        std::cerr << "MakeTestImage failed\n";
        return 5;
    }

    std::string err;
    for (int iter = 0; iter < iters; ++iter) {
        err.clear();

        if (crash_test == "1" && iso != axpipeline::plugin::AxPluginIsolationMode::kSubprocess) {
            std::cerr << "crash-test requires --mode=process\n";
            return 13;
        }

#if defined(__linux__) || defined(__unix__)
        if (crash_test == "1" && iter == 0) {
            // Must be set before Open() so the subprocess inherits it.
            ::setenv("AX_PLUGIN_BLANK_CRASH", "1", 1);
        } else {
            ::unsetenv("AX_PLUGIN_BLANK_CRASH");
        }
#endif

        if (!client->Open(so_path, "{}", -1, &err)) {
            std::cerr << "Open failed: " << err << "\n";
            return 4;
        }

        if (crash_test == "1" && iter == 0) {
#if defined(__linux__) || defined(__unix__)
            std::vector<axpipeline::ai::Detection> dets;
            const bool ok0 = client->Infer(*img, &dets, &err);
            if (ok0) {
                std::cerr << "expected infer to fail under crash injection\n";
                return 10;
            }
            ::unsetenv("AX_PLUGIN_BLANK_CRASH");

            // After crash, client should be able to restart and work again.
            dets.clear();
            err.clear();
            const bool ok1 = client->Infer(*img, &dets, &err);
            if (!ok1) {
                std::cerr << "expected infer to succeed after clearing crash env, err=" << err << "\n";
                return 11;
            }
            if (!dets.empty()) {
                std::cerr << "expected 0 dets\n";
                return 12;
            }
#else
            std::cerr << "crash-test is only supported on unix\n";
            return 13;
#endif
        } else {
            std::vector<axpipeline::ai::Detection> dets;
            if (!client->Infer(*img, &dets, &err)) {
                std::cerr << "Infer failed: " << err << "\n";
                return 6;
            }
            if (!dets.empty()) {
                std::cerr << "expected 0 dets\n";
                return 7;
            }
        }

        client->Close();
    }
    return 0;
}
