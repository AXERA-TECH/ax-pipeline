#include "npu/models/ax_model_det.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace axpipeline::npu {

namespace {

enum class Layout {
    kUnknown = 0,
    kNHWC,
    kNCHW,
};

inline float Sigmoid(float x) {
    return 1.0F / (1.0F + std::exp(-x));
}

inline float Clamp(float v, float lo, float hi) {
    return std::max(lo, std::min(v, hi));
}

inline float Logit(float p) {
    const float pp = Clamp(p, 1e-6F, 1.0F - 1e-6F);
    return std::log(pp / (1.0F - pp));
}

float IoU(const Detection& a, const Detection& b) {
    const float x0 = std::max(a.x0, b.x0);
    const float y0 = std::max(a.y0, b.y0);
    const float x1 = std::min(a.x1, b.x1);
    const float y1 = std::min(a.y1, b.y1);
    const float w = std::max(0.0F, x1 - x0);
    const float h = std::max(0.0F, y1 - y0);
    const float inter = w * h;
    const float area_a = std::max(0.0F, a.x1 - a.x0) * std::max(0.0F, a.y1 - a.y0);
    const float area_b = std::max(0.0F, b.x1 - b.x0) * std::max(0.0F, b.y1 - b.y0);
    const float uni = area_a + area_b - inter;
    if (uni <= 0.0F) return 0.0F;
    return inter / uni;
}

void Nms(std::vector<Detection>* dets, float nms_threshold) {
    if (!dets || dets->empty()) return;
    std::sort(dets->begin(), dets->end(), [](const Detection& a, const Detection& b) { return a.score > b.score; });
    std::vector<Detection> keep;
    keep.reserve(dets->size());
    for (const auto& d : *dets) {
        bool ok = true;
        for (const auto& k : keep) {
            if (IoU(d, k) > nms_threshold) {
                ok = false;
                break;
            }
        }
        if (ok) keep.push_back(d);
    }
    *dets = std::move(keep);
}

struct FeatureView {
    const float* ptr{nullptr};
    Layout layout{Layout::kUnknown};
    int feat_h{0};
    int feat_w{0};
    int channels{0};
};

bool MakeFeatureView(const AxModelBase::TensorView& t, int expected_channels, FeatureView* out) {
    if (!out) return false;
    *out = {};
    if (t.data == nullptr) return false;
    if (t.shape.size() != 4) return false;

    const int d1 = static_cast<int>(t.shape[1]);
    const int d2 = static_cast<int>(t.shape[2]);
    const int d3 = static_cast<int>(t.shape[3]);

    // Prefer explicit match with expected channels.
    if (d1 == expected_channels) {
        out->layout = Layout::kNCHW;
        out->channels = d1;
        out->feat_h = d2;
        out->feat_w = d3;
        out->ptr = t.data;
        return out->feat_h > 0 && out->feat_w > 0;
    }
    if (d3 == expected_channels) {
        out->layout = Layout::kNHWC;
        out->channels = d3;
        out->feat_h = d1;
        out->feat_w = d2;
        out->ptr = t.data;
        return out->feat_h > 0 && out->feat_w > 0;
    }

    // Fallback: assume NCHW (common for AX models) if ambiguous.
    out->layout = Layout::kNCHW;
    out->channels = d1;
    out->feat_h = d2;
    out->feat_w = d3;
    out->ptr = t.data;
    return out->feat_h > 0 && out->feat_w > 0;
}

inline float At(const FeatureView& tv, int h, int w, int c) {
    if (tv.layout == Layout::kNHWC) {
        return tv.ptr[(h * tv.feat_w + w) * tv.channels + c];
    }
    // NCHW: [C,H,W]
    return tv.ptr[(c * tv.feat_h + h) * tv.feat_w + w];
}

bool DecodeYolov5One(const FeatureView& tv,
                     int stride,
                     const float* anchors6,  // 3*(w,h)
                     int num_classes,
                     float conf_thr,
                     std::vector<Detection>* out) {
    if (!anchors6 || !out) return false;
    const int A = 3;
    const int step = num_classes + 5;
    if (tv.channels != A * step) return false;

    const float obj_logit_thr = Logit(conf_thr);
    for (int h = 0; h < tv.feat_h; ++h) {
        for (int w = 0; w < tv.feat_w; ++w) {
            for (int a = 0; a < A; ++a) {
                const int base = a * step;
                const float obj_logit = At(tv, h, w, base + 4);
                if (obj_logit < obj_logit_thr) continue;

                int best_cls = 0;
                float best_cls_logit = -std::numeric_limits<float>::infinity();
                for (int c = 0; c < num_classes; ++c) {
                    const float v = At(tv, h, w, base + 5 + c);
                    if (v > best_cls_logit) {
                        best_cls_logit = v;
                        best_cls = c;
                    }
                }
                if (best_cls_logit < obj_logit_thr) continue;

                const float obj = Sigmoid(obj_logit);
                const float score = obj * Sigmoid(best_cls_logit);
                if (score < conf_thr) continue;

                const float dx = Sigmoid(At(tv, h, w, base + 0));
                const float dy = Sigmoid(At(tv, h, w, base + 1));
                const float dw = Sigmoid(At(tv, h, w, base + 2));
                const float dh = Sigmoid(At(tv, h, w, base + 3));

                const float cx = (dx * 2.0F - 0.5F + static_cast<float>(w)) * static_cast<float>(stride);
                const float cy = (dy * 2.0F - 0.5F + static_cast<float>(h)) * static_cast<float>(stride);

                const float aw = anchors6[a * 2 + 0];
                const float ah = anchors6[a * 2 + 1];
                const float bw = dw * dw * 4.0F * aw;
                const float bh = dh * dh * 4.0F * ah;

                Detection det{};
                det.x0 = cx - bw * 0.5F;
                det.y0 = cy - bh * 0.5F;
                det.x1 = cx + bw * 0.5F;
                det.y1 = cy + bh * 0.5F;
                det.score = score;
                det.class_id = best_cls;
                out->push_back(det);
            }
        }
    }
    return true;
}

bool DecodeYolov8NativeOne(const FeatureView& tv,
                           int stride,
                           int num_classes,
                           int reg_max,
                           float conf_thr,
                           std::vector<Detection>* out) {
    if (!out) return false;
    const int step = num_classes + 4 * reg_max;
    if (tv.channels != step) return false;

    const int cls_offset = 4 * reg_max;
    const float logit_thr = Logit(conf_thr);
    for (int h = 0; h < tv.feat_h; ++h) {
        for (int w = 0; w < tv.feat_w; ++w) {
            int best_cls = 0;
            float best_logit = -std::numeric_limits<float>::infinity();
            for (int c = 0; c < num_classes; ++c) {
                const float v = At(tv, h, w, cls_offset + c);
                if (v > best_logit) {
                    best_logit = v;
                    best_cls = c;
                }
            }
            if (best_logit < logit_thr) continue;
            const float score = Sigmoid(best_logit);
            if (score < conf_thr) continue;

            float ltrb[4];
            for (int k = 0; k < 4; ++k) {
                const int base = k * reg_max;
                float alpha = At(tv, h, w, base);
                for (int i = 1; i < reg_max; ++i) {
                    alpha = std::max(alpha, At(tv, h, w, base + i));
                }
                float expsum = 0.0F;
                float exwsum = 0.0F;
                for (int i = 0; i < reg_max; ++i) {
                    const float raw = At(tv, h, w, base + i);
                    const float e = std::exp(raw - alpha);
                    expsum += e;
                    exwsum += static_cast<float>(i) * e;
                }
                const float dis = (expsum <= 0.0F) ? 0.0F : (exwsum / expsum);
                ltrb[k] = dis * static_cast<float>(stride);
            }

            const float cx = (static_cast<float>(w) + 0.5F) * static_cast<float>(stride);
            const float cy = (static_cast<float>(h) + 0.5F) * static_cast<float>(stride);

            Detection det{};
            det.x0 = cx - ltrb[0];
            det.y0 = cy - ltrb[1];
            det.x1 = cx + ltrb[2];
            det.y1 = cy + ltrb[3];
            det.score = score;
            det.class_id = best_cls;
            out->push_back(det);
        }
    }
    return true;
}

}  // namespace

bool AxModelYoloV5::Init(const YoloDetOptions& opt, std::string* error) {
    opt_ = opt;
    return AxModelBase::Init(opt_.base, error);
}

bool AxModelYoloV5::ValidateModel(std::string* error) {
    // Expect one output per stride.
    // (Runner already initialized; we validate in Postprocess via outputs.size()).
    if (opt_.strides.empty()) {
        if (error) *error = "strides is empty";
        return false;
    }
    if (opt_.yolov5_anchors.size() != opt_.strides.size() * 6U) {
        if (error) *error = "yolov5_anchors size mismatch";
        return false;
    }
    return true;
}

bool AxModelYoloV5::Postprocess(const std::vector<TensorView>& outputs,
                                const LetterboxInfo& /*lb*/,
                                std::uint32_t /*src_w*/,
                                std::uint32_t /*src_h*/,
                                std::vector<Detection>* out,
                                std::string* error) {
    if (!out) return false;
    out->clear();
    if (outputs.size() != opt_.strides.size()) {
        if (error) *error = "unexpected output tensor count: got " + std::to_string(outputs.size()) +
                            " expect " + std::to_string(opt_.strides.size());
        return false;
    }

    const int cls = opt_.num_classes;
    const float conf = opt_.conf_threshold;

    std::vector<Detection> dets;
    dets.reserve(1024);
    for (std::size_t i = 0; i < outputs.size(); ++i) {
        const int stride = opt_.strides[i];
        const int step = cls + 5;
        const int expected_ch = 3 * step;
        FeatureView tv{};
        if (!MakeFeatureView(outputs[i], expected_ch, &tv)) {
            if (error) *error = "invalid yolov5 output shape at index " + std::to_string(i);
            return false;
        }
        const float* anchors6 = opt_.yolov5_anchors.data() + i * 6U;
        if (!DecodeYolov5One(tv, stride, anchors6, cls, conf, &dets)) {
            if (error) *error = "DecodeYolov5One failed at index " + std::to_string(i);
            return false;
        }
    }

    Nms(&dets, opt_.nms_threshold);
    *out = std::move(dets);
    return true;
}

bool AxModelYoloV8Native::Init(const YoloDetOptions& opt, std::string* error) {
    opt_ = opt;
    return AxModelBase::Init(opt_.base, error);
}

bool AxModelYoloV8Native::ValidateModel(std::string* error) {
    if (opt_.strides.empty()) {
        if (error) *error = "strides is empty";
        return false;
    }
    if (opt_.yolov8_reg_max <= 1) {
        if (error) *error = "invalid yolov8_reg_max";
        return false;
    }
    return true;
}

bool AxModelYoloV8Native::Postprocess(const std::vector<TensorView>& outputs,
                                      const LetterboxInfo& /*lb*/,
                                      std::uint32_t /*src_w*/,
                                      std::uint32_t /*src_h*/,
                                      std::vector<Detection>* out,
                                      std::string* error) {
    if (!out) return false;
    out->clear();
    if (outputs.size() != opt_.strides.size()) {
        if (error) *error = "unexpected output tensor count: got " + std::to_string(outputs.size()) +
                            " expect " + std::to_string(opt_.strides.size());
        return false;
    }

    const int cls = opt_.num_classes;
    const int reg = opt_.yolov8_reg_max;
    const int expected_ch = cls + 4 * reg;

    std::vector<Detection> dets;
    dets.reserve(1024);
    for (std::size_t i = 0; i < outputs.size(); ++i) {
        const int stride = opt_.strides[i];
        FeatureView tv{};
        if (!MakeFeatureView(outputs[i], expected_ch, &tv)) {
            if (error) *error = "invalid yolov8 output shape at index " + std::to_string(i);
            return false;
        }
        if (!DecodeYolov8NativeOne(tv, stride, cls, reg, opt_.conf_threshold, &dets)) {
            if (error) *error = "DecodeYolov8NativeOne failed at index " + std::to_string(i);
            return false;
        }
    }

    Nms(&dets, opt_.nms_threshold);
    *out = std::move(dets);
    return true;
}

}  // namespace axpipeline::npu

