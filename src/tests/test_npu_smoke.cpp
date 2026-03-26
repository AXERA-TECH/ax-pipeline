#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "common/ax_image.h"
#include "common/ax_system.h"

#include "npu/yolo_detector.hpp"

#if defined(AXSDK_PLATFORM_AXCL)
#include <axcl_rt_memory.h>
#endif

namespace {

struct TestCase {
    std::string name;
    std::string model_path;
    axpipeline::npu::YoloModelType type;
};

bool FileExists(const std::string& p) {
    std::error_code ec;
    return std::filesystem::exists(p, ec);
}

#if defined(AXSDK_PLATFORM_AXCL)
axvsdk::common::AxImage::Ptr MakeAxclDeviceInputFromBgrFile(const axpipeline::npu::ModelInfo& mi,
                                                            const std::string& bgr_path,
                                                            std::string* error) {
    if (mi.input.pixel_format != axvsdk::common::PixelFormat::kBgr24 &&
        mi.input.pixel_format != axvsdk::common::PixelFormat::kRgb24) {
        if (error) *error = "smoke input supports only BGR/RGB packed models";
        return {};
    }
    const std::size_t need = static_cast<std::size_t>(mi.input.width) *
                             static_cast<std::size_t>(mi.input.height) * 3U;

    std::ifstream ifs(bgr_path, std::ios::binary);
    if (!ifs) {
        if (error) *error = "open bgr file failed: " + bgr_path;
        return {};
    }
    std::vector<std::uint8_t> host(need);
    ifs.read(reinterpret_cast<char*>(host.data()), static_cast<std::streamsize>(host.size()));
    if (ifs.gcount() != static_cast<std::streamsize>(host.size())) {
        if (error) *error = "bgr file size mismatch (need " + std::to_string(need) + " bytes): " + bgr_path;
        return {};
    }

    void* dev = nullptr;
    const auto mret = axclrtMalloc(&dev, host.size(), axclrtMemMallocPolicy{});
    if (mret != AXCL_SUCC || dev == nullptr) {
        if (error) *error = "axclrtMalloc(input) failed: " + std::to_string(static_cast<int>(mret));
        return {};
    }
    const auto cpy = axclrtMemcpy(dev, host.data(), host.size(), AXCL_MEMCPY_HOST_TO_DEVICE);
    if (cpy != AXCL_SUCC) {
        (void)axclrtFree(dev);
        if (error) *error = "axclrtMemcpy(H2D,input) failed: " + std::to_string(static_cast<int>(cpy));
        return {};
    }

    axvsdk::common::ImageDescriptor desc{};
    desc.format = mi.input.pixel_format;
    desc.width = mi.input.width;
    desc.height = mi.input.height;
    desc.strides[0] = static_cast<std::size_t>(mi.input.width) * 3U;

    std::array<axvsdk::common::ExternalImagePlane, axvsdk::common::kMaxImagePlanes> planes{};
    planes[0].virtual_address = dev;
    // For AXCL, axclrtMalloc returns a device-physical style pointer. Populate physical_address so
    // the runner can choose D2D instead of mistakenly treating it as host memory.
    planes[0].physical_address = static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(dev));

    // Hold device buffer lifetime.
    auto holder = std::shared_ptr<void>(dev, [](void* p) {
        if (p) (void)axclrtFree(p);
    });

    return axvsdk::common::AxImage::WrapExternal(desc, planes, std::move(holder));
}
#endif

int RunOne(const TestCase& tc) {
    std::cout << "[smoke] model=" << tc.name << " path=" << tc.model_path << "\n";

    // Ensure AX runtime is initialized (especially for AXCL).
    axvsdk::common::SystemOptions sys{};
    sys.enable_vdec = false;
    sys.enable_venc = false;
    sys.enable_ivps = false;
    if (!axvsdk::common::InitializeSystem(sys)) {
        std::cout << "[smoke] SKIP (InitializeSystem failed)\n";
        return 100;
    }
    struct SystemGuard {
        ~SystemGuard() { axvsdk::common::ShutdownSystem(); }
    } guard{};

    axpipeline::npu::YoloDetectorOptions opt{};
    opt.model_path = tc.model_path;
    opt.device_id = -1;  // auto-select any connected device
    opt.post.model_type = tc.type;
    opt.post.num_classes = 80;
    opt.post.conf_threshold = 0.25F;
    opt.post.nms_threshold = 0.45F;
    opt.resize_mode = axvsdk::common::ResizeMode::kKeepAspectRatio;
    opt.h_align = axvsdk::common::ResizeAlign::kCenter;
    opt.v_align = axvsdk::common::ResizeAlign::kCenter;

    axpipeline::npu::YoloDetector det;
    std::string err;
    if (!det.Init(opt, &err)) {
        // Treat missing hardware as a skip so developers can run this binary on machines without cards.
        const bool hw_missing =
            err.find("no AXCL devices") != std::string::npos ||
            err.find("no active AXCL device") != std::string::npos ||
            err.find("device may be offline") != std::string::npos;
        if (hw_missing) {
            std::cout << "[smoke] SKIP (no hardware): " << err << "\n";
            return 100;
        }
        std::cerr << "[smoke] init failed: " << err << "\n";
        return 2;
    }

    const auto& mi = det.model_info();
    std::cout << "[smoke] model input fmt=" << static_cast<int>(mi.input.pixel_format)
              << " w=" << mi.input.width << " h=" << mi.input.height
              << " outputs=" << mi.outputs.size() << "\n";

    const char* smoke_bgr = std::getenv("AXP_NPU_SMOKE_BGR");
    axvsdk::common::AxImage::Ptr src;
#if defined(AXSDK_PLATFORM_AXCL)
    if (smoke_bgr && smoke_bgr[0] != '\0') {
        src = MakeAxclDeviceInputFromBgrFile(mi, smoke_bgr, &err);
        if (!src) {
            std::cerr << "[smoke] failed to load device input from " << smoke_bgr << ": " << err << "\n";
            return 3;
        }
    }
#endif
    if (!src) {
        // Fallback: synthetic black input (not expected to have detections).
        src = axvsdk::common::AxImage::Create(mi.input.pixel_format, mi.input.width, mi.input.height);
        if (!src) {
            std::cerr << "[smoke] failed to allocate src image\n";
            return 3;
        }
        src->Fill(0);
    }

    std::vector<axpipeline::npu::Detection> dets;
    if (!det.Detect(*src, &dets, &err, nullptr)) {
        std::cerr << "[smoke] detect failed: " << err << "\n";
        return 4;
    }

    std::cout << "[smoke] dets=" << dets.size() << "\n";
    if (!dets.empty()) {
        const auto& d = dets.front();
        std::cout << "[smoke] first: cls=" << d.class_id << " score=" << d.score
                  << " box=(" << d.x0 << "," << d.y0 << "," << d.x1 << "," << d.y1 << ")\n";
    }
    if (smoke_bgr && smoke_bgr[0] != '\0' && dets.empty()) {
        std::cerr << "[smoke] expected detections when AXP_NPU_SMOKE_BGR is set, got dets=0\n";
        return 5;
    }
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    const std::vector<TestCase> cases = {
        {"yolov5s", "models/ax650/yolov5s.axmodel", axpipeline::npu::YoloModelType::kYolov5},
        {"yolov8s", "models/ax650/yolov8s.axmodel", axpipeline::npu::YoloModelType::kYolov8},
    };

    int skipped_missing = 0;
    int skipped_no_hw = 0;
    for (const auto& tc : cases) {
        if (!FileExists(tc.model_path)) {
            std::cout << "[smoke] SKIP missing: " << tc.model_path << "\n";
            skipped_missing++;
            continue;
        }
        const int rc = RunOne(tc);
        if (rc == 100) {
            skipped_no_hw++;
            continue;
        }
        if (rc != 0) return rc;
    }

    if (skipped_missing + skipped_no_hw == static_cast<int>(cases.size())) {
        if (skipped_missing == static_cast<int>(cases.size())) {
            std::cout << "[smoke] all cases skipped (no models found)\n";
        } else if (skipped_no_hw == static_cast<int>(cases.size())) {
            std::cout << "[smoke] all cases skipped (no hardware)\n";
        } else {
            std::cout << "[smoke] all cases skipped\n";
        }
    }
    std::cout << "OK\n";
    return 0;
}
