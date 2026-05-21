#define AX_PLUGIN_BUILD_DLL 1

#include "ax_plugin/ax_plugin.h"

#include <algorithm>
#include <atomic>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "json.hpp"

#include "ai/ax_detection.hpp"
#include "common/ax_image.h"
#include "common/ax_image_processor.h"
#include "tracking/ax_bytetrack.hpp"

#include "npu/runner/ax_model_runner.hpp"

#if defined(AXPIPELINE_HAVE_AXCL)
#include "../common/npu/src/npu/runner/axcl/ax_model_runner_axcl.hpp"
#include "../common/npu/src/npu/runner/axcl/axcl_manager.h"
#endif

#if defined(AXPIPELINE_HAVE_MSP)
#include "../common/npu/src/npu/runner/ax650/ax_model_runner_ax650.hpp"
#endif

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

struct ModelBaseOptions {
    int device_id{-1};
    std::string model_path;

    axvsdk::common::ResizeMode resize_mode{axvsdk::common::ResizeMode::kKeepAspectRatio};
    axvsdk::common::ResizeAlign h_align{axvsdk::common::ResizeAlign::kCenter};
    axvsdk::common::ResizeAlign v_align{axvsdk::common::ResizeAlign::kCenter};
    std::uint32_t background_color{0};  // 0xRRGGBB

    // NPU affinity bitmask (e.g. 0b001/0b010/0b100). 0 = default.
    std::uint32_t npu_affinity{0};
};

struct PcdOptions {
    ModelBaseOptions base{};

    int num_classes{3};
    float conf_threshold{0.25F};
    float nms_threshold{0.45F};
    int max_det{50};
    std::vector<int> strides{16, 32};
};

struct LetterboxInfo {
    float scale{1.0F};
    float pad_x{0.0F};
    float pad_y{0.0F};
    std::uint32_t dst_w{0};
    std::uint32_t dst_h{0};
};

LetterboxInfo ComputeLetterbox(std::uint32_t src_w,
                               std::uint32_t src_h,
                               std::uint32_t dst_w,
                               std::uint32_t dst_h,
                               axvsdk::common::ResizeAlign h_align,
                               axvsdk::common::ResizeAlign v_align) {
    LetterboxInfo lb{};
    lb.dst_w = dst_w;
    lb.dst_h = dst_h;
    if (src_w == 0 || src_h == 0 || dst_w == 0 || dst_h == 0) {
        return lb;
    }

    const float sx = static_cast<float>(dst_w) / static_cast<float>(src_w);
    const float sy = static_cast<float>(dst_h) / static_cast<float>(src_h);
    lb.scale = std::min(sx, sy);

    const std::uint32_t new_w = static_cast<std::uint32_t>(std::round(static_cast<float>(src_w) * lb.scale));
    const std::uint32_t new_h = static_cast<std::uint32_t>(std::round(static_cast<float>(src_h) * lb.scale));
    const std::uint32_t pad_w = (dst_w > new_w) ? (dst_w - new_w) : 0U;
    const std::uint32_t pad_h = (dst_h > new_h) ? (dst_h - new_h) : 0U;

    const std::uint32_t pad_x0 = [&]() -> std::uint32_t {
        switch (h_align) {
        case axvsdk::common::ResizeAlign::kStart:
            return 0U;
        case axvsdk::common::ResizeAlign::kEnd:
            return pad_w;
        case axvsdk::common::ResizeAlign::kCenter:
        default:
            return pad_w / 2U;
        }
    }();

    const std::uint32_t pad_y0 = [&]() -> std::uint32_t {
        switch (v_align) {
        case axvsdk::common::ResizeAlign::kStart:
            return 0U;
        case axvsdk::common::ResizeAlign::kEnd:
            return pad_h;
        case axvsdk::common::ResizeAlign::kCenter:
        default:
            return pad_h / 2U;
        }
    }();

    lb.pad_x = static_cast<float>(pad_x0);
    lb.pad_y = static_cast<float>(pad_y0);
    return lb;
}

void UndoLetterbox(const LetterboxInfo& lb, std::uint32_t src_w, std::uint32_t src_h, std::vector<axpipeline::ai::Detection>* dets) {
    if (!dets || dets->empty()) return;
    if (lb.scale <= 0.0F) return;

    for (auto& d : *dets) {
        d.x0 = (d.x0 - lb.pad_x) / lb.scale;
        d.y0 = (d.y0 - lb.pad_y) / lb.scale;
        d.x1 = (d.x1 - lb.pad_x) / lb.scale;
        d.y1 = (d.y1 - lb.pad_y) / lb.scale;

        if (d.x1 < d.x0) std::swap(d.x0, d.x1);
        if (d.y1 < d.y0) std::swap(d.y0, d.y1);

        d.x0 = std::max(0.0F, std::min(d.x0, static_cast<float>(src_w)));
        d.y0 = std::max(0.0F, std::min(d.y0, static_cast<float>(src_h)));
        d.x1 = std::max(0.0F, std::min(d.x1, static_cast<float>(src_w)));
        d.y1 = std::max(0.0F, std::min(d.y1, static_cast<float>(src_h)));
    }
}

enum class InputLayout {
    kUnknown = 0,
    kNHWC,
    kNCHW,
};

struct InputMeta {
    InputLayout layout{InputLayout::kUnknown};
    std::uint32_t width{0};
    std::uint32_t height{0};
    std::uint32_t channels{0};
};

bool InferInputMeta(const ax_runner_tensor_t& in, InputMeta* out, std::string* error) {
    if (!out) return false;
    *out = {};
    if (in.vShape.size() != 4) {
        if (error) *error = "unexpected input dims: rank=" + std::to_string(in.vShape.size());
        return false;
    }

    const auto n = in.vShape[0];
    const auto d1 = in.vShape[1];
    const auto d2 = in.vShape[2];
    const auto d3 = in.vShape[3];
    (void)n;

    if (d1 == 3U) {
        out->layout = InputLayout::kNCHW;
        out->channels = 3;
        out->height = d2;
        out->width = d3;
    } else if (d3 == 3U) {
        out->layout = InputLayout::kNHWC;
        out->channels = 3;
        out->height = d1;
        out->width = d2;
    } else {
        if (error) *error = "cannot infer input layout from shape";
        return false;
    }

    if (out->width == 0 || out->height == 0) {
        if (error) *error = "invalid input shape";
        return false;
    }
    return true;
}

bool ReadFileToBuffer(const std::string& path, std::vector<char>* out, std::string* error) {
    if (out == nullptr) return false;
    out->clear();
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) {
        if (error) *error = "open failed: " + path;
        return false;
    }
    ifs.seekg(0, std::ios::end);
    const auto end = ifs.tellg();
    if (end <= 0) {
        if (error) *error = "empty file: " + path;
        return false;
    }
    out->resize(static_cast<std::size_t>(end));
    ifs.seekg(0, std::ios::beg);
    if (!ifs.read(out->data(), static_cast<std::streamsize>(out->size()))) {
        if (error) *error = "read failed: " + path;
        return false;
    }
    return true;
}

struct TensorView {
    const float* data{nullptr};
    std::vector<unsigned int> shape;
    std::size_t bytes{0};
    std::string name;
};

struct DenseTensorView {
    const float* data{nullptr};
    int channels{0};
    std::size_t anchors{0};
    // If true: layout is [C][N] (channel-first). Else: [N][C] (anchor-first).
    bool channel_first{true};
};

bool MakeDenseView(const TensorView& t, int expected_channels, std::size_t expected_anchors, DenseTensorView* out) {
    if (!out) return false;
    *out = {};
    if (t.data == nullptr) return false;
    if (expected_channels <= 0 || expected_anchors == 0) return false;

    std::vector<unsigned int> dims = t.shape;
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

inline float IoUInclusive(const axpipeline::ai::Detection& a, const axpipeline::ai::Detection& b) noexcept {
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

void NmsPerClass(std::vector<axpipeline::ai::Detection>* dets, float iou_thr, std::size_t max_det) {
    if (!dets || dets->empty()) return;
    if (iou_thr < 0.0F) iou_thr = 0.0F;

    std::sort(dets->begin(), dets->end(), [](const auto& a, const auto& b) { return a.score > b.score; });

    if (iou_thr <= 0.0F) {
        if (max_det > 0 && dets->size() > max_det) dets->resize(max_det);
        return;
    }

    std::vector<axpipeline::ai::Detection> keep;
    keep.reserve((max_det > 0) ? std::min(dets->size(), max_det) : dets->size());
    for (const auto& d : *dets) {
        bool ok = true;
        for (const auto& k : keep) {
            if (d.class_id != k.class_id) continue;
            if (IoUInclusive(d, k) > iou_thr) {
                ok = false;
                break;
            }
        }
        if (!ok) continue;
        keep.push_back(d);
        if (max_det > 0 && keep.size() >= max_det) break;
    }
    *dets = std::move(keep);
}

class PcdModel final {
public:
    bool Init(const PcdOptions& opt, std::string* error) {
        Deinit();
        opt_ = opt;
        if (opt_.base.model_path.empty()) {
            if (error) *error = "model_path is empty";
            return false;
        }

        if (!ReadFileToBuffer(opt_.base.model_path, &model_bytes_, error)) {
            return false;
        }

        if (!imgproc_) {
            imgproc_ = axvsdk::common::CreateImageProcessor();
            if (!imgproc_) {
                if (error) *error = "CreateImageProcessor failed";
                return false;
            }
        }

        if (opt_.base.device_id < 0) {
            opt_.base.device_id = 0;
        }

#if defined(AXPIPELINE_HAVE_AXCL)
        if (!axcl_Dev_IsInit(opt_.base.device_id)) {
            if (axcl_Dev_Init(opt_.base.device_id) != 0) {
                if (error) *error = "axcl_Dev_Init failed for device " + std::to_string(opt_.base.device_id);
                return false;
            }
        }
        runner_ = std::make_shared<ax_runner_axcl>();
#elif defined(AXPIPELINE_HAVE_MSP)
        runner_ = std::make_shared<ax_runner_ax650>();
#else
        (void)error;
        return false;
#endif

        if (!runner_) {
            if (error) *error = "create runner failed";
            return false;
        }

        if (opt_.base.npu_affinity != 0) {
            runner_->set_init_affinity(static_cast<int>(opt_.base.npu_affinity));
        }

        const int ret = runner_->init(model_bytes_.data(), static_cast<unsigned int>(model_bytes_.size()), opt_.base.device_id);
        if (ret != 0) {
            if (error) *error = "runner init failed: ret=" + std::to_string(ret);
            return false;
        }

        if (opt_.base.npu_affinity != 0) {
            (void)runner_->set_affinity(static_cast<int>(opt_.base.npu_affinity));
        }

        if (runner_->get_num_inputs() < 1) {
            if (error) *error = "model has no inputs";
            return false;
        }
        if (!InferInputMeta(runner_->get_input(0), &input_, error)) {
            return false;
        }

        if (input_.channels != 3) {
            if (error) *error = "unsupported input channels: " + std::to_string(input_.channels);
            return false;
        }

        const std::size_t expect_bytes =
            static_cast<std::size_t>(input_.width) * static_cast<std::size_t>(input_.height) * 3U;
        if (runner_->get_input(0).nSize < static_cast<int>(expect_bytes)) {
            if (error) *error = "runner input buffer too small for expected RGB bytes";
            return false;
        }

        if (opt_.strides.empty()) {
            if (error) *error = "strides is empty";
            return false;
        }

        anchor_cx_.clear();
        anchor_cy_.clear();
        anchor_stride_.clear();
        for (const int s : opt_.strides) {
            if (s <= 0) continue;
            const std::uint32_t gh = input_.height / static_cast<std::uint32_t>(s);
            const std::uint32_t gw = input_.width / static_cast<std::uint32_t>(s);
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

        axvsdk::common::ImageDescriptor desc{};
        desc.format = axvsdk::common::PixelFormat::kRgb24;
        desc.width = input_.width;
        desc.height = input_.height;
        desc.strides[0] = static_cast<std::size_t>(desc.width) * 3U;

        axvsdk::common::ImageAllocationOptions alloc{};
        alloc.memory_type = axvsdk::common::MemoryType::kCmm;
        alloc.cache_mode = axvsdk::common::CacheMode::kNonCached;
        alloc.alignment = 0x1000;
        alloc.token = "PcdPreprocess";

        preprocess_rgb_ = axvsdk::common::AxImage::Create(desc, alloc);
        if (!preprocess_rgb_) {
            if (error) *error = "allocate preprocess buffer failed";
            return false;
        }

        host_rgb_.resize(expect_bytes);
        return true;
    }

    void Deinit() {
        if (runner_) {
            runner_->deinit();
        }
        runner_.reset();
        model_bytes_.clear();
        input_ = {};
        preprocess_rgb_.reset();
        host_rgb_.clear();
        anchor_cx_.clear();
        anchor_cy_.clear();
        anchor_stride_.clear();
    }

    bool Infer(const axvsdk::common::AxImage& frame, std::vector<axpipeline::ai::Detection>* out, std::string* error) {
        if (out == nullptr) return false;
        out->clear();
        if (!runner_ || !preprocess_rgb_) {
            if (error) *error = "model not initialized";
            return false;
        }

        const auto src_w = frame.width();
        const auto src_h = frame.height();
        const auto lb = ComputeLetterbox(src_w, src_h, input_.width, input_.height, opt_.base.h_align, opt_.base.v_align);

        axvsdk::common::ImageProcessRequest req{};
        req.output_image.format = axvsdk::common::PixelFormat::kRgb24;
        req.output_image.width = input_.width;
        req.output_image.height = input_.height;
        req.resize.mode = opt_.base.resize_mode;
        req.resize.horizontal_align = opt_.base.h_align;
        req.resize.vertical_align = opt_.base.v_align;
        req.resize.background_color = opt_.base.background_color;

        if (!imgproc_->Process(frame, req, *preprocess_rgb_)) {
            if (error) *error = "preprocess failed";
            return false;
        }

        const std::size_t bytes = static_cast<std::size_t>(input_.width) * static_cast<std::size_t>(input_.height) * 3U;
        const auto phy0 = preprocess_rgb_->physical_address(0);
        const auto* vir0 = preprocess_rgb_->plane_data(0);

        const std::uint8_t* rgb_hwc = nullptr;
#if defined(AXPIPELINE_HAVE_AXCL)
        (void)vir0;
        // AXCL: preprocess output is device memory; copy back to host for layout conversion.
        if (phy0 == 0) {
            if (error) *error = "preprocess rgb has no physical address";
            return false;
        }
        const void* src_dev = reinterpret_cast<const void*>(static_cast<std::uintptr_t>(phy0));
        if (axcl_Memcpy(host_rgb_.data(), src_dev, bytes, AXCL_MEMCPY_DEVICE_TO_HOST, opt_.base.device_id) != 0) {
            if (error) *error = "axcl_Memcpy(D2H preprocess rgb) failed";
            return false;
        }
        rgb_hwc = host_rgb_.data();
#else
        (void)phy0;
        // MSP: CMM is CPU addressable.
        if (vir0 == nullptr) {
            if (error) *error = "preprocess rgb plane not accessible";
            return false;
        }
        if (preprocess_rgb_->stride(0) == static_cast<std::size_t>(input_.width) * 3U) {
            // Tight-packed: direct view.
            rgb_hwc = vir0;
        } else {
            // Should not happen (we set stride), but handle defensively.
            for (std::uint32_t y = 0; y < input_.height; ++y) {
                std::memcpy(host_rgb_.data() + static_cast<std::size_t>(y) * static_cast<std::size_t>(input_.width) * 3U,
                            vir0 + static_cast<std::size_t>(y) * preprocess_rgb_->stride(0),
                            static_cast<std::size_t>(input_.width) * 3U);
            }
            rgb_hwc = host_rgb_.data();
        }
#endif
        if (rgb_hwc == nullptr) {
            if (error) *error = "preprocess rgb pointer is null";
            return false;
        }

        auto& in = runner_->get_input(0);
        if (in.pVirAddr == nullptr || in.nSize <= 0) {
            if (error) *error = "invalid runner input staging buffer";
            return false;
        }
        auto* dst = static_cast<std::uint8_t*>(in.pVirAddr);

        if (input_.layout == InputLayout::kNHWC) {
            std::memcpy(dst, rgb_hwc, bytes);
        } else if (input_.layout == InputLayout::kNCHW) {
            const std::size_t plane = static_cast<std::size_t>(input_.width) * static_cast<std::size_t>(input_.height);
            std::uint8_t* r = dst;
            std::uint8_t* g = dst + plane;
            std::uint8_t* b = dst + plane * 2U;
            for (std::size_t i = 0; i < plane; ++i) {
                r[i] = rgb_hwc[i * 3U + 0];
                g[i] = rgb_hwc[i * 3U + 1];
                b[i] = rgb_hwc[i * 3U + 2];
            }
        } else {
            if (error) *error = "unknown input layout";
            return false;
        }

        // Run.
        const int ret = runner_->inference();
        if (ret != 0) {
            if (error) *error = "runner inference failed: ret=" + std::to_string(ret);
            return false;
        }

        // Collect outputs (already synced in runner for both backends).
        std::vector<TensorView> outputs;
        const int nout = runner_->get_num_outputs();
        outputs.reserve(static_cast<std::size_t>(nout));
        for (int i = 0; i < nout; ++i) {
            const auto& o = runner_->get_output(i);
            TensorView tv{};
            tv.data = reinterpret_cast<const float*>(o.pVirAddr);
            tv.shape = o.vShape;
            tv.name = o.sName;
            tv.bytes = static_cast<std::size_t>(o.nSize);
            outputs.push_back(std::move(tv));
        }

        // Decode.
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

        std::vector<axpipeline::ai::Detection> dets;
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

            axpipeline::ai::Detection d{};
            d.x0 = cx - l * s;
            d.y0 = cy - t * s;
            d.x1 = cx + r * s;
            d.y1 = cy + b * s;
            d.score = score;
            d.class_id = best_cls;
            d.track_id = -1;
            dets.push_back(d);
        }

        // NMS + mapping back to source coords.
        NmsPerClass(&dets, opt_.nms_threshold, (opt_.max_det > 0) ? static_cast<std::size_t>(opt_.max_det) : 0U);
        if (opt_.base.resize_mode == axvsdk::common::ResizeMode::kKeepAspectRatio) {
            UndoLetterbox(lb, src_w, src_h, &dets);
        } else {
            // stretch: map linearly
            const float sx = static_cast<float>(src_w) / static_cast<float>(input_.width);
            const float sy = static_cast<float>(src_h) / static_cast<float>(input_.height);
            for (auto& d : dets) {
                d.x0 *= sx;
                d.x1 *= sx;
                d.y0 *= sy;
                d.y1 *= sy;
            }
        }

        *out = std::move(dets);
        return true;
    }

private:
    PcdOptions opt_{};
    std::vector<char> model_bytes_{};
    std::shared_ptr<ax_runner_base> runner_{};
    InputMeta input_{};

    std::unique_ptr<axvsdk::common::ImageProcessor> imgproc_{};
    axvsdk::common::AxImage::Ptr preprocess_rgb_{};
    std::vector<std::uint8_t> host_rgb_{};

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
    PcdModel model;
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

    std::vector<axpipeline::ai::Detection> dets;
    std::string err;
    if (!ctx->model.Infer(*frame, &dets, &err)) {
        if (!err.empty()) {
            std::fprintf(stderr, "[ax_plugin_pcd] Infer failed: %s\n", err.c_str());
        } else {
            std::fprintf(stderr, "[ax_plugin_pcd] Infer failed\n");
        }
        return -3;
    }

    ctx->out_dets.clear();
    if (ctx->tracker) {
        const auto tracks = ctx->tracker->Update(dets);
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
