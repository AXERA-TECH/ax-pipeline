#define AX_PLUGIN_BUILD_DLL 1

#include "ax_plugin/ax_plugin.h"

#include <atomic>
#include <array>
#include <chrono>
#include <cstdio>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "json.hpp"

#include "common/ax_image.h"
#include "npu/models/ax_model_det.hpp"

namespace {

using json = nlohmann::json;

axvsdk::common::ResizeMode ParseResizeMode(const std::string& s) {
    if (s == "keep_aspect" || s == "keep_aspect_ratio") return axvsdk::common::ResizeMode::kKeepAspectRatio;
    return axvsdk::common::ResizeMode::kStretch;
}

axvsdk::common::ResizeAlign ParseResizeAlign(const std::string& s) {
    if (s == "start") return axvsdk::common::ResizeAlign::kStart;
    if (s == "end") return axvsdk::common::ResizeAlign::kEnd;
    return axvsdk::common::ResizeAlign::kCenter;
}

std::uint64_t NowUs() {
    using Clock = std::chrono::steady_clock;
    const auto now = Clock::now().time_since_epoch();
    return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(now).count());
}

std::uint32_t NextNpuAffinityMask3() {
    static std::atomic<unsigned int> g_idx{0};
    const unsigned int idx = g_idx.fetch_add(1, std::memory_order_relaxed);
    switch (idx % 3U) {
    case 0:
        return 0b001U;
    case 1:
        return 0b010U;
    default:
        return 0b100U;
    }
}

struct PluginCtx {
    axpipeline::npu::AxModelYoloV8Split model;
    std::vector<ax_plugin_det_t> out_dets;

    bool debug_timing{false};
    std::uint64_t timing_interval_us{1000000};
    std::uint64_t last_timing_us{0};
    std::uint64_t timing_count{0};
    std::uint64_t timing_pre_sum_us{0};
    std::uint64_t timing_infer_sum_us{0};
    std::uint64_t timing_post_sum_us{0};
    std::uint64_t timing_total_sum_us{0};
};

bool BuildAxImageView(const ax_plugin_image_view_t& view, axvsdk::common::AxImage::Ptr* out) {
    if (out == nullptr) return false;
    *out = nullptr;

    if (view.width == 0 || view.height == 0 || view.plane_count == 0) return false;

    axvsdk::common::ImageDescriptor desc{};
    switch (view.format) {
    case AX_PLUGIN_PIXEL_FORMAT_NV12:
        desc.format = axvsdk::common::PixelFormat::kNv12;
        break;
    case AX_PLUGIN_PIXEL_FORMAT_RGB24:
        desc.format = axvsdk::common::PixelFormat::kRgb24;
        break;
    case AX_PLUGIN_PIXEL_FORMAT_BGR24:
        desc.format = axvsdk::common::PixelFormat::kBgr24;
        break;
    default:
        desc.format = axvsdk::common::PixelFormat::kUnknown;
        break;
    }
    if (desc.format == axvsdk::common::PixelFormat::kUnknown) return false;
    desc.width = view.width;
    desc.height = view.height;
    for (std::size_t i = 0; i < 3 && i < view.plane_count; ++i) {
        desc.strides[i] = view.strides[i];
    }

    std::array<axvsdk::common::ExternalImagePlane, axvsdk::common::kMaxImagePlanes> planes{};
    for (std::size_t i = 0; i < axvsdk::common::kMaxImagePlanes; ++i) {
        planes[i].virtual_address = nullptr;
        planes[i].physical_address = 0;
        planes[i].block_id = axvsdk::common::kInvalidPoolId;
    }
    for (std::size_t i = 0; i < view.plane_count && i < axvsdk::common::kMaxImagePlanes; ++i) {
        planes[i].virtual_address = view.virtual_addrs[i];
        planes[i].physical_address = view.physical_addrs[i];
        planes[i].block_id = view.block_ids[i];
    }

    auto img = axvsdk::common::AxImage::WrapExternal(desc, planes);
    if (!img) return false;
    *out = std::move(img);
    return true;
}

}  // namespace

extern "C" {

int ax_plugin_get_api_version(void) {
    return AX_PLUGIN_API_VERSION;
}

int ax_plugin_init(const char* init_json, int32_t device_id, ax_plugin_handle_t* out_handle) {
    if (out_handle == nullptr) return -1;
    *out_handle = nullptr;

    json j;
    try {
        j = json::parse(init_json ? init_json : "{}");
    } catch (...) {
        return -2;
    }

    axpipeline::npu::YoloDetOptions opt{};
    opt.base.device_id = device_id;
    if (j.contains("device_id") && j["device_id"].is_number_integer()) {
        opt.base.device_id = j["device_id"].get<int>();
    }

    if (j.contains("model_path") && j["model_path"].is_string()) {
        opt.base.model_path = j["model_path"].get<std::string>();
    }
    if (opt.base.model_path.empty()) {
        return -3;
    }

    if (j.contains("npu_affinity")) {
        const auto& a = j["npu_affinity"];
        if (a.is_number_integer()) {
            const auto v = a.get<std::int64_t>();
            if (v >= 0) opt.base.npu_affinity = static_cast<std::uint32_t>(v);
        } else if (a.is_string()) {
            const auto s = a.get<std::string>();
            if (s == "rr" || s == "round_robin") {
                opt.base.npu_affinity = NextNpuAffinityMask3();
            }
        }
    }

    if (j.contains("resize_mode") && j["resize_mode"].is_string()) {
        opt.base.resize_mode = ParseResizeMode(j["resize_mode"].get<std::string>());
    }
    if (j.contains("horizontal_align") && j["horizontal_align"].is_string()) {
        opt.base.h_align = ParseResizeAlign(j["horizontal_align"].get<std::string>());
    }
    if (j.contains("vertical_align") && j["vertical_align"].is_string()) {
        opt.base.v_align = ParseResizeAlign(j["vertical_align"].get<std::string>());
    }
    if (j.contains("background_color") &&
        (j["background_color"].is_number_unsigned() || j["background_color"].is_number_integer())) {
        const auto v = j["background_color"].get<std::int64_t>();
        if (v >= 0) opt.base.background_color = static_cast<std::uint32_t>(v);
    }

    if (j.contains("num_classes") && j["num_classes"].is_number_integer()) {
        opt.num_classes = j["num_classes"].get<int>();
    }
    if (j.contains("conf_threshold") && j["conf_threshold"].is_number()) {
        opt.conf_threshold = static_cast<float>(j["conf_threshold"].get<double>());
    }
    if (j.contains("nms_threshold") && j["nms_threshold"].is_number()) {
        opt.nms_threshold = static_cast<float>(j["nms_threshold"].get<double>());
    }
    if (j.contains("pre_nms_topk") && j["pre_nms_topk"].is_number_integer()) {
        const int v = j["pre_nms_topk"].get<int>();
        if (v >= 0) opt.pre_nms_topk = v;
    }
    if (j.contains("max_det") && j["max_det"].is_number_integer()) {
        const int v = j["max_det"].get<int>();
        if (v >= 0) opt.max_det = v;
    }
    if (j.contains("class_agnostic_nms") && j["class_agnostic_nms"].is_boolean()) {
        opt.class_agnostic_nms = j["class_agnostic_nms"].get<bool>();
    }
    if (j.contains("strides") && j["strides"].is_array()) {
        opt.strides.clear();
        for (const auto& s : j["strides"]) {
            if (s.is_number_integer()) opt.strides.push_back(s.get<int>());
        }
    }
    if (j.contains("yolov8_reg_max") && j["yolov8_reg_max"].is_number_integer()) {
        opt.yolov8_reg_max = j["yolov8_reg_max"].get<int>();
    }

    auto ctx = std::make_unique<PluginCtx>();
    std::string err;
    if (!ctx->model.Init(opt, &err)) {
        return -4;
    }

    if (j.contains("debug_timing") && j["debug_timing"].is_boolean()) {
        ctx->debug_timing = j["debug_timing"].get<bool>();
    }
    if (j.contains("timing_interval_ms") && j["timing_interval_ms"].is_number_integer()) {
        const auto ms = j["timing_interval_ms"].get<std::int64_t>();
        if (ms > 0) ctx->timing_interval_us = static_cast<std::uint64_t>(ms) * 1000ULL;
    }
    ctx->last_timing_us = NowUs();

    if (ctx->debug_timing) {
        const auto in = ctx->model.input_spec();
        const char* fmt = "unknown";
        switch (in.format) {
        case axvsdk::common::PixelFormat::kNv12:
            fmt = "nv12";
            break;
        case axvsdk::common::PixelFormat::kRgb24:
            fmt = "rgb24";
            break;
        case axvsdk::common::PixelFormat::kBgr24:
            fmt = "bgr24";
            break;
        default:
            fmt = "unknown";
            break;
        }
        std::fprintf(stderr, "[ax_plugin_yolov8_split] model input: w=%u h=%u fmt=%s\n", in.width, in.height, fmt);
    }

    *out_handle = reinterpret_cast<ax_plugin_handle_t>(ctx.release());
    return 0;
}

void ax_plugin_deinit(ax_plugin_handle_t handle) {
    auto* ctx = reinterpret_cast<PluginCtx*>(handle);
    if (!ctx) return;
    ctx->model.Deinit();
    delete ctx;
}

int ax_plugin_infer(ax_plugin_handle_t handle,
                    const ax_plugin_image_view_t* image,
                    ax_plugin_det_result_t* out_result) {
    auto* ctx = reinterpret_cast<PluginCtx*>(handle);
    if (!ctx || !image || !out_result) return -1;

    axvsdk::common::AxImage::Ptr frame;
    if (!BuildAxImageView(*image, &frame) || !frame) {
        return -2;
    }

    std::vector<axpipeline::npu::Detection> dets;
    std::string err;
    axpipeline::npu::RunTimings tm{};
    axpipeline::npu::RunTimings* tm_ptr = ctx->debug_timing ? &tm : nullptr;
    if (!ctx->model.Infer(*frame, &dets, &err, tm_ptr)) {
        if (!err.empty()) {
            std::fprintf(stderr, "[ax_plugin_yolov8_split] Infer failed: %s\n", err.c_str());
        } else {
            std::fprintf(stderr, "[ax_plugin_yolov8_split] Infer failed\n");
        }
        return -3;
    }

    if (ctx->debug_timing) {
        ctx->timing_count++;
        ctx->timing_pre_sum_us += tm.preprocess_us;
        ctx->timing_infer_sum_us += tm.infer_us;
        ctx->timing_post_sum_us += tm.postprocess_us;
        ctx->timing_total_sum_us += tm.total_us;

        const auto now_us = NowUs();
        if (now_us - ctx->last_timing_us >= ctx->timing_interval_us && ctx->timing_count > 0) {
            const auto n = ctx->timing_count;
            const auto pre = ctx->timing_pre_sum_us / n;
            const auto inf = ctx->timing_infer_sum_us / n;
            const auto post = ctx->timing_post_sum_us / n;
            const auto total = ctx->timing_total_sum_us / n;
            std::fprintf(stderr,
                         "[ax_plugin_yolov8_split] timing avg_us{pre=%llu infer=%llu post=%llu total=%llu} n=%llu\n",
                         static_cast<unsigned long long>(pre),
                         static_cast<unsigned long long>(inf),
                         static_cast<unsigned long long>(post),
                         static_cast<unsigned long long>(total),
                         static_cast<unsigned long long>(n));
            ctx->last_timing_us = now_us;
            ctx->timing_count = 0;
            ctx->timing_pre_sum_us = 0;
            ctx->timing_infer_sum_us = 0;
            ctx->timing_post_sum_us = 0;
            ctx->timing_total_sum_us = 0;
        }
    }

    ctx->out_dets.clear();
    ctx->out_dets.reserve(dets.size());
    for (const auto& d : dets) {
        ax_plugin_det_t dd{};
        dd.x0 = d.x0;
        dd.y0 = d.y0;
        dd.x1 = d.x1;
        dd.y1 = d.y1;
        dd.score = d.score;
        dd.class_id = d.class_id;
        ctx->out_dets.push_back(dd);
    }

    out_result->dets = ctx->out_dets.empty() ? nullptr : ctx->out_dets.data();
    out_result->det_count = ctx->out_dets.size();
    return 0;
}

void ax_plugin_release_result(ax_plugin_handle_t handle, ax_plugin_det_result_t* result) {
    auto* ctx = reinterpret_cast<PluginCtx*>(handle);
    if (!ctx || result == nullptr) return;
    result->dets = nullptr;
    result->det_count = 0;
}

}  // extern "C"
