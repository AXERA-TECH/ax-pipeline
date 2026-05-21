#define AX_PLUGIN_BUILD_DLL 1

#include "ax_plugin/ax_plugin.h"

#include <algorithm>
#include <atomic>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "json.hpp"

#include "common/ax_image.h"
#include "npu/models/ax_model_base.hpp"
#include "tracking/ax_bytetrack.hpp"

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

inline float Sigmoid(float x) noexcept {
    return 1.0F / (1.0F + std::exp(-x));
}

struct PcdOptions {
    axpipeline::npu::ModelInitOptions base{};

    int num_classes{3};
    float conf_threshold{0.25F};
    float nms_threshold{0.45F};
    int max_det{50};
    std::vector<int> strides{16, 32};
};

struct DenseTensorView {
    const float* data{nullptr};
    int channels{0};
    std::size_t anchors{0};
    // If true: layout is [C][N] (channel-first). Else: [N][C] (anchor-first).
    bool channel_first{true};
};

bool MakeDenseView(const axpipeline::npu::AxModelBase::TensorView& t,
                   int expected_channels,
                   std::size_t expected_anchors,
                   DenseTensorView* out) {
    if (!out) return false;
    *out = {};
    if (t.data == nullptr) return false;
    if (expected_channels <= 0 || expected_anchors == 0) return false;

    std::vector<unsigned int> dims = t.shape;
    // Squeeze batch/degenerate dims (keep at least 2 dims).
    if (dims.size() > 2) {
        for (auto it = dims.begin(); dims.size() > 2 && it != dims.end();) {
            if (*it == 1U) {
                it = dims.erase(it);
                continue;
            }
            ++it;
        }
    }

    auto prod = [](const std::vector<unsigned int>& v) -> std::size_t {
        std::size_t p = 1;
        for (auto d : v) p *= static_cast<std::size_t>(d);
        return p;
    };
    if (prod(dims) != expected_channels * expected_anchors) {
        return false;
    }

    if (dims.size() == 2) {
        if (dims[0] == static_cast<unsigned int>(expected_channels) &&
            dims[1] == static_cast<unsigned int>(expected_anchors)) {
            out->data = t.data;
            out->channels = expected_channels;
            out->anchors = expected_anchors;
            out->channel_first = true;
            return true;
        }
        if (dims[1] == static_cast<unsigned int>(expected_channels) &&
            dims[0] == static_cast<unsigned int>(expected_anchors)) {
            out->data = t.data;
            out->channels = expected_channels;
            out->anchors = expected_anchors;
            out->channel_first = false;
            return true;
        }
        return false;
    }

    if (dims.size() == 3) {
        // [C, H, W] or [H, W, C]
        if (dims[0] == static_cast<unsigned int>(expected_channels)) {
            out->data = t.data;
            out->channels = expected_channels;
            out->anchors = expected_anchors;
            out->channel_first = true;
            return true;
        }
        if (dims[2] == static_cast<unsigned int>(expected_channels)) {
            out->data = t.data;
            out->channels = expected_channels;
            out->anchors = expected_anchors;
            out->channel_first = false;
            return true;
        }
        return false;
    }

    return false;
}

inline float DenseAt(const DenseTensorView& tv, int c, std::size_t n) noexcept {
    if (tv.channel_first) {
        return tv.data[static_cast<std::size_t>(c) * tv.anchors + n];
    }
    return tv.data[n * static_cast<std::size_t>(tv.channels) + static_cast<std::size_t>(c)];
}

inline float IoUInclusive(const axpipeline::npu::Detection& a, const axpipeline::npu::Detection& b) noexcept {
    const float x0 = std::max(a.x0, b.x0);
    const float y0 = std::max(a.y0, b.y0);
    const float x1 = std::min(a.x1, b.x1);
    const float y1 = std::min(a.y1, b.y1);
    const float w = std::max(0.0F, x1 - x0 + 1.0F);
    const float h = std::max(0.0F, y1 - y0 + 1.0F);
    const float inter = w * h;
    const float area_a = std::max(0.0F, a.x1 - a.x0 + 1.0F) * std::max(0.0F, a.y1 - a.y0 + 1.0F);
    const float area_b = std::max(0.0F, b.x1 - b.x0 + 1.0F) * std::max(0.0F, b.y1 - b.y0 + 1.0F);
    const float uni = area_a + area_b - inter;
    if (uni <= 0.0F) return 0.0F;
    return inter / uni;
}

void NmsPerClass(std::vector<axpipeline::npu::Detection>* dets, float iou_thr) {
    if (!dets || dets->empty()) return;
    if (iou_thr <= 0.0F) return;

    std::sort(dets->begin(), dets->end(), [](const auto& a, const auto& b) { return a.score > b.score; });

    std::vector<axpipeline::npu::Detection> keep;
    keep.reserve(dets->size());
    for (const auto& d : *dets) {
        bool ok = true;
        for (const auto& k : keep) {
            if (d.class_id != k.class_id) continue;
            if (IoUInclusive(d, k) > iou_thr) {
                ok = false;
                break;
            }
        }
        if (ok) keep.push_back(d);
    }
    *dets = std::move(keep);
}

class AxModelPcd final : public axpipeline::npu::AxModelBase {
public:
    bool Init(const PcdOptions& opt, std::string* error) {
        opt_ = opt;
        return AxModelBase::Init(opt_.base, error);
    }

private:
    bool ValidateModel(std::string* error) override {
        if (opt_.strides.empty()) {
            if (error) *error = "strides is empty";
            return false;
        }
        if (opt_.num_classes <= 0) {
            if (error) *error = "num_classes must be > 0";
            return false;
        }

        const auto in = input_spec();
        if (in.width == 0 || in.height == 0) {
            if (error) *error = "invalid model input spec";
            return false;
        }

        anchor_cx_.clear();
        anchor_cy_.clear();
        anchor_stride_.clear();

        for (const int s : opt_.strides) {
            if (s <= 0) continue;
            const std::uint32_t gh = in.height / static_cast<std::uint32_t>(s);
            const std::uint32_t gw = in.width / static_cast<std::uint32_t>(s);
            for (std::uint32_t y = 0; y < gh; ++y) {
                for (std::uint32_t x = 0; x < gw; ++x) {
                    anchor_cx_.push_back((static_cast<float>(x) + 0.5F) * static_cast<float>(s));
                    anchor_cy_.push_back((static_cast<float>(y) + 0.5F) * static_cast<float>(s));
                    anchor_stride_.push_back(static_cast<float>(s));
                }
            }
        }

        if (anchor_cx_.empty()) {
            if (error) *error = "anchor grid is empty (check strides vs model input size)";
            return false;
        }
        return true;
    }

    bool Postprocess(const std::vector<TensorView>& outputs,
                     const axpipeline::npu::LetterboxInfo& /*lb*/,
                     std::uint32_t /*src_w*/,
                     std::uint32_t /*src_h*/,
                     std::vector<axpipeline::npu::Detection>* out,
                     std::string* error) override {
        if (!out) return false;
        out->clear();

        if (outputs.size() < 2) {
            if (error) *error = "unexpected output tensor count: got " + std::to_string(outputs.size());
            return false;
        }

        const auto in = input_spec();
        const std::size_t anchors = anchor_cx_.size();

        DenseTensorView boxes{};
        DenseTensorView scores{};
        bool ok = false;
        for (std::size_t i = 0; i < outputs.size() && !ok; ++i) {
            for (std::size_t j = 0; j < outputs.size() && !ok; ++j) {
                if (i == j) continue;
                if (!MakeDenseView(outputs[i], 4, anchors, &boxes)) continue;
                if (!MakeDenseView(outputs[j], opt_.num_classes, anchors, &scores)) continue;
                ok = true;
            }
        }
        if (!ok) {
            if (error) *error = "cannot match output tensors to (boxes=4,N) and (scores=C,N)";
            return false;
        }

        std::vector<axpipeline::npu::Detection> dets;
        dets.reserve(static_cast<std::size_t>(opt_.max_det > 0 ? opt_.max_det : 256));

        const float conf_thr = opt_.conf_threshold;
        for (std::size_t n = 0; n < anchors; ++n) {
            int best_cls = 0;
            float best_prob = 0.0F;
            for (int c = 0; c < opt_.num_classes; ++c) {
                const float p = Sigmoid(DenseAt(scores, c, n));
                if (p > best_prob) {
                    best_prob = p;
                    best_cls = c;
                }
            }

            // Same behavior as the reference python code:
            // - filter by obj (= max prob)
            // - final conf = obj * max_prob (i.e. square)
            const float obj = best_prob;
            if (obj <= conf_thr) continue;
            const float score = obj * best_prob;

            const float l = DenseAt(boxes, 0, n);
            const float t = DenseAt(boxes, 1, n);
            const float r = DenseAt(boxes, 2, n);
            const float b = DenseAt(boxes, 3, n);

            const float s = anchor_stride_[n];
            const float cx = anchor_cx_[n];
            const float cy = anchor_cy_[n];

            float x0 = cx - l * s;
            float y0 = cy - t * s;
            float x1 = cx + r * s;
            float y1 = cy + b * s;

            x0 = std::max(0.0F, std::min(x0, static_cast<float>(in.width)));
            y0 = std::max(0.0F, std::min(y0, static_cast<float>(in.height)));
            x1 = std::max(0.0F, std::min(x1, static_cast<float>(in.width)));
            y1 = std::max(0.0F, std::min(y1, static_cast<float>(in.height)));
            if (x1 <= x0) x1 = std::min(static_cast<float>(in.width), x0 + 1.0F);
            if (y1 <= y0) y1 = std::min(static_cast<float>(in.height), y0 + 1.0F);

            axpipeline::npu::Detection d{};
            d.x0 = x0;
            d.y0 = y0;
            d.x1 = x1;
            d.y1 = y1;
            d.score = score;
            d.class_id = best_cls;
            dets.push_back(d);
        }

        if (dets.empty()) {
            *out = {};
            return true;
        }

        std::sort(dets.begin(), dets.end(), [](const auto& a, const auto& b) { return a.score > b.score; });
        if (opt_.max_det > 0 && dets.size() > static_cast<std::size_t>(opt_.max_det)) {
            dets.resize(static_cast<std::size_t>(opt_.max_det));
        }

        NmsPerClass(&dets, opt_.nms_threshold);
        *out = std::move(dets);
        return true;
    }

    PcdOptions opt_{};
    std::vector<float> anchor_cx_;
    std::vector<float> anchor_cy_;
    std::vector<float> anchor_stride_;
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

struct PluginCtx {
    AxModelPcd model;
    std::vector<ax_plugin_det_t> out_dets;
    std::unique_ptr<axpipeline::tracking::ByteTrack> tracker;
};

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

    PcdOptions opt{};
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
    } else if (j.contains("iou_threshold") && j["iou_threshold"].is_number()) {
        opt.nms_threshold = static_cast<float>(j["iou_threshold"].get<double>());
    }
    if (j.contains("max_det") && j["max_det"].is_number_integer()) {
        const int v = j["max_det"].get<int>();
        if (v > 0) opt.max_det = v;
    }
    if (j.contains("strides") && j["strides"].is_array()) {
        opt.strides.clear();
        for (const auto& s : j["strides"]) {
            if (s.is_number_integer()) opt.strides.push_back(s.get<int>());
        }
    }

    auto ctx = std::make_unique<PluginCtx>();
    std::string err;
    if (!ctx->model.Init(opt, &err)) {
        std::fprintf(stderr, "[ax_plugin_pcd] init failed: %s\n", err.c_str());
        return -4;
    }

    bool enable_tracking = false;
    axpipeline::tracking::ByteTrackOptions topt{};
    topt.frame_rate = 30;
    topt.track_buffer = 30;
    topt.min_score = 0.0F;
    if (j.contains("enable_tracking") && j["enable_tracking"].is_boolean()) {
        enable_tracking = j["enable_tracking"].get<bool>();
    }
    if (j.contains("track_fps") && j["track_fps"].is_number_integer()) {
        const int v = j["track_fps"].get<int>();
        if (v > 0) topt.frame_rate = v;
    }
    if (j.contains("track_buffer") && j["track_buffer"].is_number_integer()) {
        const int v = j["track_buffer"].get<int>();
        if (v > 0) topt.track_buffer = v;
    }
    if (j.contains("track_min_score") && j["track_min_score"].is_number()) {
        const auto v = static_cast<float>(j["track_min_score"].get<double>());
        if (v >= 0.0F) topt.min_score = v;
    }
    if (enable_tracking) {
        ctx->tracker = std::make_unique<axpipeline::tracking::ByteTrack>(topt);
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
        if (!err.empty()) {
            std::fprintf(stderr, "[ax_plugin_pcd] Infer failed: %s\n", err.c_str());
        } else {
            std::fprintf(stderr, "[ax_plugin_pcd] Infer failed\n");
        }
        return -3;
    }

    ctx->out_dets.clear();
    if (ctx->tracker) {
        std::vector<axpipeline::ai::Detection> dets_ai;
        dets_ai.reserve(dets.size());
        for (const auto& d : dets) {
            axpipeline::ai::Detection dd{};
            dd.x0 = d.x0;
            dd.y0 = d.y0;
            dd.x1 = d.x1;
            dd.y1 = d.y1;
            dd.score = d.score;
            dd.class_id = d.class_id;
            dets_ai.push_back(dd);
        }

        const auto tracks = ctx->tracker->Update(dets_ai);
        ctx->out_dets.reserve(tracks.size());
        for (const auto& t : tracks) {
            ax_plugin_det_t dd{};
            dd.x0 = t.x0;
            dd.y0 = t.y0;
            dd.x1 = t.x1;
            dd.y1 = t.y1;
            dd.score = t.score;
            dd.class_id = t.class_id;
            dd.track_id = t.track_id;
            ctx->out_dets.push_back(dd);
        }
    } else {
        ctx->out_dets.reserve(dets.size());
        for (const auto& d : dets) {
            ax_plugin_det_t dd{};
            dd.x0 = d.x0;
            dd.y0 = d.y0;
            dd.x1 = d.x1;
            dd.y1 = d.y1;
            dd.score = d.score;
            dd.class_id = d.class_id;
            dd.track_id = -1;
            ctx->out_dets.push_back(dd);
        }
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
