#define AX_PLUGIN_BUILD_DLL 1

#include "ax_plugin/ax_plugin.h"

#include <array>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "json.hpp"

#include "common/ax_image.h"
#include "npu/models/ax_model_det.hpp"

namespace {

using json = nlohmann::json;

axvsdk::common::PixelFormat ParsePixelFormat(const std::string& s) {
    if (s == "nv12" || s == "NV12") return axvsdk::common::PixelFormat::kNv12;
    if (s == "rgb" || s == "RGB" || s == "rgb24" || s == "RGB24") return axvsdk::common::PixelFormat::kRgb24;
    if (s == "bgr" || s == "BGR" || s == "bgr24" || s == "BGR24") return axvsdk::common::PixelFormat::kBgr24;
    return axvsdk::common::PixelFormat::kUnknown;
}

axvsdk::common::ResizeMode ParseResizeMode(const std::string& s) {
    if (s == "keep_aspect" || s == "keep_aspect_ratio") return axvsdk::common::ResizeMode::kKeepAspectRatio;
    return axvsdk::common::ResizeMode::kStretch;
}

axvsdk::common::ResizeAlign ParseResizeAlign(const std::string& s) {
    if (s == "start") return axvsdk::common::ResizeAlign::kStart;
    if (s == "end") return axvsdk::common::ResizeAlign::kEnd;
    return axvsdk::common::ResizeAlign::kCenter;
}

struct PluginCtx {
    axpipeline::npu::AxModelYoloV5 model;
    std::vector<ax_plugin_det_t> out_dets;
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

    if (j.contains("input_format") && j["input_format"].is_string()) {
        opt.base.resize_mode = axvsdk::common::ResizeMode::kKeepAspectRatio;
        // Note: input_format here refers to the model input format, not the source frame.
        // AxModelBase reads model input spec from the model and ignores this.
        (void)ParsePixelFormat(j["input_format"].get<std::string>());
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
    if (j.contains("background_color") && (j["background_color"].is_number_unsigned() || j["background_color"].is_number_integer())) {
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
    if (j.contains("strides") && j["strides"].is_array()) {
        opt.strides.clear();
        for (const auto& s : j["strides"]) {
            if (s.is_number_integer()) opt.strides.push_back(s.get<int>());
        }
    }
    if (j.contains("yolov5_anchors") && j["yolov5_anchors"].is_array()) {
        opt.yolov5_anchors.clear();
        for (const auto& a : j["yolov5_anchors"]) {
            if (a.is_number()) opt.yolov5_anchors.push_back(static_cast<float>(a.get<double>()));
        }
    }

    auto ctx = std::make_unique<PluginCtx>();
    std::string err;
    if (!ctx->model.Init(opt, &err)) {
        return -4;
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
    if (!ctx->model.Infer(*frame, &dets, &err, nullptr)) {
        return -3;
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
