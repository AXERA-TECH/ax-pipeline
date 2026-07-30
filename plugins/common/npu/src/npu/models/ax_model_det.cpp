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

inline bool ScoreGreater(const Detection& a, const Detection& b) noexcept {
    return a.score > b.score;
}

void LimitTopK(std::vector<Detection>* dets, std::size_t k) {
    if (!dets || k == 0 || dets->size() <= k) return;
    auto mid = dets->begin() + static_cast<std::ptrdiff_t>(k);
    std::nth_element(dets->begin(), mid, dets->end(), ScoreGreater);
    dets->resize(k);
}

inline float Area(const Detection& d) noexcept {
    const float w = std::max(0.0F, d.x1 - d.x0);
    const float h = std::max(0.0F, d.y1 - d.y0);
    return w * h;
}

float IoU(const Detection& a, float area_a, const Detection& b, float area_b) {
    const float x0 = std::max(a.x0, b.x0);
    const float y0 = std::max(a.y0, b.y0);
    const float x1 = std::min(a.x1, b.x1);
    const float y1 = std::min(a.y1, b.y1);
    const float w = std::max(0.0F, x1 - x0);
    const float h = std::max(0.0F, y1 - y0);
    const float inter = w * h;
    const float uni = area_a + area_b - inter;
    if (uni <= 0.0F) return 0.0F;
    return inter / uni;
}

void Nms(std::vector<Detection>* dets, float nms_threshold, std::size_t max_det, bool class_agnostic) {
    if (!dets || dets->empty()) return;
    if (nms_threshold < 0.0F) nms_threshold = 0.0F;

    std::sort(dets->begin(), dets->end(), ScoreGreater);

    if (nms_threshold <= 0.0F) {
        if (max_det > 0 && dets->size() > max_det) {
            dets->resize(max_det);
        }
        return;
    }

    const std::size_t reserve_n = (max_det > 0) ? std::min(dets->size(), max_det) : dets->size();
    std::vector<Detection> keep;
    keep.reserve(reserve_n);
    std::vector<float> keep_area;
    keep_area.reserve(reserve_n);

    for (const auto& d : *dets) {
        const float d_area = Area(d);
        bool ok = true;
        for (std::size_t i = 0; i < keep.size(); ++i) {
            if (!class_agnostic && d.class_id != keep[i].class_id) continue;
            if (IoU(d, d_area, keep[i], keep_area[i]) > nms_threshold) {
                ok = false;
                break;
            }
        }
        if (!ok) continue;

        keep.push_back(d);
        keep_area.push_back(d_area);
        if (max_det > 0 && keep.size() >= max_det) break;
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
    // Hoist a per-cell base pointer + channel stride once, instead of calling At() per element
    // (which recomputes the flat index and re-branches on layout every access).
    // NHWC: channels are contiguous (cstep=1, vectorizable); NCHW: strided by feat_h*feat_w.
    const bool nhwc = (tv.layout == Layout::kNHWC);
    const std::ptrdiff_t cstep = nhwc ? 1 : static_cast<std::ptrdiff_t>(tv.feat_h) * tv.feat_w;
    for (int h = 0; h < tv.feat_h; ++h) {
        for (int w = 0; w < tv.feat_w; ++w) {
            const int idx = h * tv.feat_w + w;
            const float* cell0 = nhwc
                ? tv.ptr + static_cast<std::ptrdiff_t>(idx) * tv.channels
                : tv.ptr + idx;
            for (int a = 0; a < A; ++a) {
                const int base = a * step;
                const float obj_logit = cell0[static_cast<std::ptrdiff_t>(base + 4) * cstep];
                if (obj_logit < obj_logit_thr) continue;

                const float* cls0 = cell0 + static_cast<std::ptrdiff_t>(base + 5) * cstep;
                int best_cls = 0;
                float best_cls_logit = cls0[0];
                for (int c = 1; c < num_classes; ++c) {
                    const float v = cls0[static_cast<std::ptrdiff_t>(c) * cstep];
                    if (v > best_cls_logit) {
                        best_cls_logit = v;
                        best_cls = c;
                    }
                }
                if (best_cls_logit < obj_logit_thr) continue;

                const float obj = Sigmoid(obj_logit);
                const float score = obj * Sigmoid(best_cls_logit);
                if (score < conf_thr) continue;

                const float dx = Sigmoid(cell0[static_cast<std::ptrdiff_t>(base + 0) * cstep]);
                const float dy = Sigmoid(cell0[static_cast<std::ptrdiff_t>(base + 1) * cstep]);
                const float dw = Sigmoid(cell0[static_cast<std::ptrdiff_t>(base + 2) * cstep]);
                const float dh = Sigmoid(cell0[static_cast<std::ptrdiff_t>(base + 3) * cstep]);

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
    // Hoist a per-cell base pointer + channel stride once (see DecodeYolov5One).
    const bool nhwc = (tv.layout == Layout::kNHWC);
    const std::ptrdiff_t cstep = nhwc ? 1 : static_cast<std::ptrdiff_t>(tv.feat_h) * tv.feat_w;
    for (int h = 0; h < tv.feat_h; ++h) {
        for (int w = 0; w < tv.feat_w; ++w) {
            const int idx = h * tv.feat_w + w;
            const float* cell0 = nhwc
                ? tv.ptr + static_cast<std::ptrdiff_t>(idx) * tv.channels
                : tv.ptr + idx;

            const float* cls0 = cell0 + static_cast<std::ptrdiff_t>(cls_offset) * cstep;
            int best_cls = 0;
            float best_logit = cls0[0];
            for (int c = 1; c < num_classes; ++c) {
                const float v = cls0[static_cast<std::ptrdiff_t>(c) * cstep];
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
                const float* p = cell0 + static_cast<std::ptrdiff_t>(k * reg_max) * cstep;
                float alpha = p[0];
                for (int i = 1; i < reg_max; ++i) {
                    alpha = std::max(alpha, p[static_cast<std::ptrdiff_t>(i) * cstep]);
                }
                float expsum = 0.0F;
                float exwsum = 0.0F;
                for (int i = 0; i < reg_max; ++i) {
                    const float e = std::exp(p[static_cast<std::ptrdiff_t>(i) * cstep] - alpha);
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

    if (opt_.pre_nms_topk > 0) {
        LimitTopK(&dets, static_cast<std::size_t>(opt_.pre_nms_topk));
    }
    Nms(&dets, opt_.nms_threshold, static_cast<std::size_t>(opt_.max_det), opt_.class_agnostic_nms);
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
    const float conf = opt_.conf_threshold;
    const float logit_thr = Logit(conf);
    const std::size_t pre_topk = (opt_.pre_nms_topk > 0) ? static_cast<std::size_t>(opt_.pre_nms_topk) : 0U;

    // Fast path: select top-K candidates by class logit, then decode DFL only for them.
    if (pre_topk > 0) {
        struct Candidate {
            const FeatureView* tv{nullptr};
            int stride{0};
            int h{0};
            int w{0};
            int class_id{0};
            float cls_logit{-std::numeric_limits<float>::infinity()};
        };

        struct CandidateLogitGreater {
            bool operator()(const Candidate& a, const Candidate& b) const noexcept { return a.cls_logit > b.cls_logit; }
        };

        auto push_topk = [&](std::vector<Candidate>* heap, const Candidate& c) {
            if (heap == nullptr || pre_topk == 0) return;
            if (heap->size() < pre_topk) {
                heap->push_back(c);
                std::push_heap(heap->begin(), heap->end(), CandidateLogitGreater{});
                return;
            }
            if (heap->empty()) return;
            // Min-heap by cls_logit: smallest is at front().
            if (c.cls_logit <= heap->front().cls_logit) return;
            std::pop_heap(heap->begin(), heap->end(), CandidateLogitGreater{});
            heap->back() = c;
            std::push_heap(heap->begin(), heap->end(), CandidateLogitGreater{});
        };

        std::vector<FeatureView> tvs(outputs.size());
        std::vector<int> strides(outputs.size(), 0);
        for (std::size_t i = 0; i < outputs.size(); ++i) {
            const int stride = opt_.strides[i];
            FeatureView tv{};
            if (!MakeFeatureView(outputs[i], expected_ch, &tv)) {
                if (error) *error = "invalid yolov8 output shape at index " + std::to_string(i);
                return false;
            }
            tvs[i] = tv;
            strides[i] = stride;
        }

        std::vector<Candidate> heap;
        heap.reserve(pre_topk);

        const int cls_offset = 4 * reg;
        for (std::size_t i = 0; i < tvs.size(); ++i) {
            const auto& tv = tvs[i];
            const int stride = strides[i];
            if (!tv.ptr || tv.feat_h <= 0 || tv.feat_w <= 0 || tv.channels <= 0) continue;

            if (tv.layout == Layout::kNHWC) {
                for (int h = 0; h < tv.feat_h; ++h) {
                    for (int w = 0; w < tv.feat_w; ++w) {
                        const int idx = h * tv.feat_w + w;
                        const float* cell = tv.ptr + static_cast<std::ptrdiff_t>(idx) * tv.channels;

                        int best_cls = 0;
                        float best_logit = -std::numeric_limits<float>::infinity();
                        const float* cls_ptr = cell + cls_offset;
                        for (int c = 0; c < cls; ++c) {
                            const float v = cls_ptr[c];
                            if (v > best_logit) {
                                best_logit = v;
                                best_cls = c;
                            }
                        }
                        if (best_logit < logit_thr) continue;

                        Candidate cand{};
                        cand.tv = &tv;
                        cand.stride = stride;
                        cand.h = h;
                        cand.w = w;
                        cand.class_id = best_cls;
                        cand.cls_logit = best_logit;
                        push_topk(&heap, cand);
                    }
                }
            } else {
                const int hw = tv.feat_h * tv.feat_w;
                const float* cls_base = tv.ptr + static_cast<std::ptrdiff_t>(cls_offset) * hw;

                // NCHW layout is cache-unfriendly when iterating [h,w] then scanning channels.
                // Do it channel-first: accumulate best-logit/best-cls per cell in a tight loop.
                thread_local std::vector<float> best_logits;
                thread_local std::vector<int> best_classes;
                best_logits.assign(static_cast<std::size_t>(hw), -std::numeric_limits<float>::infinity());
                best_classes.assign(static_cast<std::size_t>(hw), 0);

                for (int c = 0; c < cls; ++c) {
                    const float* p = cls_base + static_cast<std::ptrdiff_t>(c) * hw;
                    for (int idx = 0; idx < hw; ++idx) {
                        const float v = p[idx];
                        if (v > best_logits[static_cast<std::size_t>(idx)]) {
                            best_logits[static_cast<std::size_t>(idx)] = v;
                            best_classes[static_cast<std::size_t>(idx)] = c;
                        }
                    }
                }

                for (int idx = 0; idx < hw; ++idx) {
                    const float best_logit = best_logits[static_cast<std::size_t>(idx)];
                    if (best_logit < logit_thr) continue;

                    const int h = idx / tv.feat_w;
                    const int w = idx - h * tv.feat_w;

                    Candidate cand{};
                    cand.tv = &tv;
                    cand.stride = stride;
                    cand.h = h;
                    cand.w = w;
                    cand.class_id = best_classes[static_cast<std::size_t>(idx)];
                    cand.cls_logit = best_logit;
                    push_topk(&heap, cand);
                }
            }
        }

        std::vector<Detection> dets;
        dets.reserve(heap.size());

        for (const auto& cand : heap) {
            if (cand.tv == nullptr) continue;
            const auto& tv = *cand.tv;
            const int stride = cand.stride;
            const int idx = cand.h * tv.feat_w + cand.w;

            float ltrb[4];
            if (tv.layout == Layout::kNHWC) {
                const float* cell = tv.ptr + static_cast<std::ptrdiff_t>(idx) * tv.channels;
                for (int k = 0; k < 4; ++k) {
                    const float* p = cell + k * reg;
                    float alpha = p[0];
                    for (int i = 1; i < reg; ++i) alpha = std::max(alpha, p[i]);
                    float expsum = 0.0F;
                    float exwsum = 0.0F;
                    for (int i = 0; i < reg; ++i) {
                        const float e = std::exp(p[i] - alpha);
                        expsum += e;
                        exwsum += static_cast<float>(i) * e;
                    }
                    const float dis = (expsum <= 0.0F) ? 0.0F : (exwsum / expsum);
                    ltrb[k] = dis * static_cast<float>(stride);
                }
            } else {
                const int hw = tv.feat_h * tv.feat_w;
                for (int k = 0; k < 4; ++k) {
                    const int base = k * reg;
                    float alpha = tv.ptr[(base + 0) * hw + idx];
                    for (int i = 1; i < reg; ++i) {
                        alpha = std::max(alpha, tv.ptr[(base + i) * hw + idx]);
                    }
                    float expsum = 0.0F;
                    float exwsum = 0.0F;
                    for (int i = 0; i < reg; ++i) {
                        const float raw = tv.ptr[(base + i) * hw + idx];
                        const float e = std::exp(raw - alpha);
                        expsum += e;
                        exwsum += static_cast<float>(i) * e;
                    }
                    const float dis = (expsum <= 0.0F) ? 0.0F : (exwsum / expsum);
                    ltrb[k] = dis * static_cast<float>(stride);
                }
            }

            const float cx = (static_cast<float>(cand.w) + 0.5F) * static_cast<float>(stride);
            const float cy = (static_cast<float>(cand.h) + 0.5F) * static_cast<float>(stride);

            Detection det{};
            det.x0 = cx - ltrb[0];
            det.y0 = cy - ltrb[1];
            det.x1 = cx + ltrb[2];
            det.y1 = cy + ltrb[3];
            det.score = Sigmoid(cand.cls_logit);
            det.class_id = cand.class_id;
            dets.push_back(det);
        }

        Nms(&dets, opt_.nms_threshold, static_cast<std::size_t>(opt_.max_det), opt_.class_agnostic_nms);
        *out = std::move(dets);
        return true;
    }

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

    Nms(&dets, opt_.nms_threshold, static_cast<std::size_t>(opt_.max_det), opt_.class_agnostic_nms);
    *out = std::move(dets);
    return true;
}

bool AxModelYoloV8Split::Init(const YoloDetOptions& opt, std::string* error) {
    opt_ = opt;
    return AxModelBase::Init(opt_.base, error);
}

bool AxModelYoloV8Split::ValidateModel(std::string* error) {
    if (opt_.strides.empty()) {
        if (error) *error = "strides is empty";
        return false;
    }
    if (opt_.yolov8_reg_max <= 1) {
        if (error) *error = "invalid yolov8_reg_max";
        return false;
    }
    if (opt_.num_classes <= 0) {
        if (error) *error = "invalid num_classes";
        return false;
    }
    return true;
}

bool AxModelYoloV8Split::Postprocess(const std::vector<TensorView>& outputs,
                                     const LetterboxInfo& /*lb*/,
                                     std::uint32_t /*src_w*/,
                                     std::uint32_t /*src_h*/,
                                     std::vector<Detection>* out,
                                     std::string* error) {
    if (!out) return false;
    out->clear();

    const std::size_t nstrides = opt_.strides.size();
    const std::size_t expected_outputs = nstrides * 2U;
    if (outputs.size() != expected_outputs) {
        if (error) *error = "unexpected output tensor count: got " + std::to_string(outputs.size()) +
                            " expect " + std::to_string(expected_outputs);
        return false;
    }

    const int cls = opt_.num_classes;
    const int reg = opt_.yolov8_reg_max;
    const int reg_ch = 4 * reg;
    const float conf = opt_.conf_threshold;
    const float logit_thr = Logit(conf);
    const std::size_t pre_topk = (opt_.pre_nms_topk > 0) ? static_cast<std::size_t>(opt_.pre_nms_topk) : 0U;

    // Assign output tensors to (stride -> {cls, reg}) pairs by shape and channel count.
    struct Pair {
        FeatureView tv_cls{};
        FeatureView tv_reg{};
        bool has_cls{false};
        bool has_reg{false};
    };
    std::vector<Pair> pairs(nstrides);

    const auto in = input_spec();
    struct ExpFeat {
        int h{0};
        int w{0};
    };
    std::vector<ExpFeat> expected_feat(nstrides);
    for (std::size_t i = 0; i < nstrides; ++i) {
        const int stride = opt_.strides[i];
        expected_feat[i].h = stride > 0 ? static_cast<int>(in.height / static_cast<std::uint32_t>(stride)) : 0;
        expected_feat[i].w = stride > 0 ? static_cast<int>(in.width / static_cast<std::uint32_t>(stride)) : 0;
    }

    auto find_stride_index = [&](int feat_h, int feat_w) -> int {
        for (std::size_t i = 0; i < nstrides; ++i) {
            if (expected_feat[i].h == feat_h && expected_feat[i].w == feat_w) {
                return static_cast<int>(i);
            }
        }
        return -1;
    };

    for (std::size_t oi = 0; oi < outputs.size(); ++oi) {
        FeatureView tv{};
        if (MakeFeatureView(outputs[oi], cls, &tv) && tv.channels == cls) {
            const int si = find_stride_index(tv.feat_h, tv.feat_w);
            if (si < 0) {
                if (error) *error = "unmatched cls output shape at index " + std::to_string(oi);
                return false;
            }
            if (pairs[static_cast<std::size_t>(si)].has_cls) {
                if (error) *error = "duplicate cls output for stride index " + std::to_string(si);
                return false;
            }
            pairs[static_cast<std::size_t>(si)].tv_cls = tv;
            pairs[static_cast<std::size_t>(si)].has_cls = true;
            continue;
        }
        if (MakeFeatureView(outputs[oi], reg_ch, &tv) && tv.channels == reg_ch) {
            const int si = find_stride_index(tv.feat_h, tv.feat_w);
            if (si < 0) {
                if (error) *error = "unmatched reg output shape at index " + std::to_string(oi);
                return false;
            }
            if (pairs[static_cast<std::size_t>(si)].has_reg) {
                if (error) *error = "duplicate reg output for stride index " + std::to_string(si);
                return false;
            }
            pairs[static_cast<std::size_t>(si)].tv_reg = tv;
            pairs[static_cast<std::size_t>(si)].has_reg = true;
            continue;
        }

        if (error) *error = "unexpected yolov8-split output channels at index " + std::to_string(oi);
        return false;
    }

    for (std::size_t i = 0; i < nstrides; ++i) {
        if (!pairs[i].has_cls || !pairs[i].has_reg) {
            if (error) *error = "missing cls/reg output for stride index " + std::to_string(i);
            return false;
        }
        if (pairs[i].tv_cls.feat_h != pairs[i].tv_reg.feat_h || pairs[i].tv_cls.feat_w != pairs[i].tv_reg.feat_w) {
            if (error) *error = "cls/reg feature map size mismatch at stride index " + std::to_string(i);
            return false;
        }
    }

    // Fast path: select top-K candidates by cls logit, then decode DFL only for them.
    if (pre_topk > 0) {
        struct Candidate {
            const FeatureView* tv_cls{nullptr};
            const FeatureView* tv_reg{nullptr};
            int stride{0};
            int h{0};
            int w{0};
            int class_id{0};
            float cls_logit{-std::numeric_limits<float>::infinity()};
        };

        struct CandidateLogitGreater {
            bool operator()(const Candidate& a, const Candidate& b) const noexcept { return a.cls_logit > b.cls_logit; }
        };

        auto push_topk = [&](std::vector<Candidate>* heap, const Candidate& c) {
            if (heap == nullptr || pre_topk == 0) return;
            if (heap->size() < pre_topk) {
                heap->push_back(c);
                std::push_heap(heap->begin(), heap->end(), CandidateLogitGreater{});
                return;
            }
            if (heap->empty()) return;
            if (c.cls_logit <= heap->front().cls_logit) return;
            std::pop_heap(heap->begin(), heap->end(), CandidateLogitGreater{});
            heap->back() = c;
            std::push_heap(heap->begin(), heap->end(), CandidateLogitGreater{});
        };

        std::vector<Candidate> heap;
        heap.reserve(pre_topk);

        for (std::size_t si = 0; si < nstrides; ++si) {
            const int stride = opt_.strides[si];
            const auto& tv_cls = pairs[si].tv_cls;
            const auto& tv_reg = pairs[si].tv_reg;

            if (tv_cls.layout == Layout::kNHWC) {
                // Hot path: argmax over the class channels for every anchor (feat_h*feat_w*cls
                // reads per stride). Class logits are contiguous in NHWC, so walk them through a
                // hoisted raw pointer instead of calling At() per element (which recomputes the
                // index and re-branches on layout each time) -- this lets the compiler vectorize
                // the reduction and is the difference between ~30 ms and a few ms here.
                const int chans = tv_cls.channels;
                for (int h = 0; h < tv_cls.feat_h; ++h) {
                    for (int w = 0; w < tv_cls.feat_w; ++w) {
                        const float* cls_ptr =
                            tv_cls.ptr + static_cast<std::ptrdiff_t>(h * tv_cls.feat_w + w) * chans;
                        float best_logit = cls_ptr[0];
                        int best_cls = 0;
                        for (int c = 1; c < cls; ++c) {
                            if (cls_ptr[c] > best_logit) {
                                best_logit = cls_ptr[c];
                                best_cls = c;
                            }
                        }
                        if (best_logit < logit_thr) continue;

                        Candidate cand{};
                        cand.tv_cls = &tv_cls;
                        cand.tv_reg = &tv_reg;
                        cand.stride = stride;
                        cand.h = h;
                        cand.w = w;
                        cand.class_id = best_cls;
                        cand.cls_logit = best_logit;
                        push_topk(&heap, cand);
                    }
                }
            } else {
                const int hw = tv_cls.feat_h * tv_cls.feat_w;
                thread_local std::vector<float> best_logits;
                thread_local std::vector<int> best_classes;
                best_logits.assign(static_cast<std::size_t>(hw), -std::numeric_limits<float>::infinity());
                best_classes.assign(static_cast<std::size_t>(hw), 0);

                for (int c = 0; c < cls; ++c) {
                    const float* p = tv_cls.ptr + static_cast<std::ptrdiff_t>(c) * hw;
                    for (int idx = 0; idx < hw; ++idx) {
                        const float v = p[idx];
                        if (v > best_logits[static_cast<std::size_t>(idx)]) {
                            best_logits[static_cast<std::size_t>(idx)] = v;
                            best_classes[static_cast<std::size_t>(idx)] = c;
                        }
                    }
                }

                for (int idx = 0; idx < hw; ++idx) {
                    const float best_logit = best_logits[static_cast<std::size_t>(idx)];
                    if (best_logit < logit_thr) continue;

                    const int h = idx / tv_cls.feat_w;
                    const int w = idx - h * tv_cls.feat_w;

                    Candidate cand{};
                    cand.tv_cls = &tv_cls;
                    cand.tv_reg = &tv_reg;
                    cand.stride = stride;
                    cand.h = h;
                    cand.w = w;
                    cand.class_id = best_classes[static_cast<std::size_t>(idx)];
                    cand.cls_logit = best_logit;
                    push_topk(&heap, cand);
                }
            }
        }

        std::vector<Detection> dets;
        dets.reserve(heap.size());
        for (const auto& cand : heap) {
            if (cand.tv_cls == nullptr || cand.tv_reg == nullptr) continue;
            const auto& tv_reg = *cand.tv_reg;
            const int idx = cand.h * tv_reg.feat_w + cand.w;

            float ltrb[4];
            if (tv_reg.layout == Layout::kNHWC) {
                const float* cell = tv_reg.ptr + static_cast<std::ptrdiff_t>(idx) * tv_reg.channels;
                for (int k = 0; k < 4; ++k) {
                    const float* p = cell + k * reg;
                    float alpha = p[0];
                    for (int i = 1; i < reg; ++i) alpha = std::max(alpha, p[i]);
                    float expsum = 0.0F;
                    float exwsum = 0.0F;
                    for (int i = 0; i < reg; ++i) {
                        const float e = std::exp(p[i] - alpha);
                        expsum += e;
                        exwsum += static_cast<float>(i) * e;
                    }
                    const float dis = (expsum <= 0.0F) ? 0.0F : (exwsum / expsum);
                    ltrb[k] = dis * static_cast<float>(cand.stride);
                }
            } else {
                const int hw = tv_reg.feat_h * tv_reg.feat_w;
                for (int k = 0; k < 4; ++k) {
                    const int base = k * reg;
                    float alpha = tv_reg.ptr[(base + 0) * hw + idx];
                    for (int i = 1; i < reg; ++i) {
                        alpha = std::max(alpha, tv_reg.ptr[(base + i) * hw + idx]);
                    }
                    float expsum = 0.0F;
                    float exwsum = 0.0F;
                    for (int i = 0; i < reg; ++i) {
                        const float raw = tv_reg.ptr[(base + i) * hw + idx];
                        const float e = std::exp(raw - alpha);
                        expsum += e;
                        exwsum += static_cast<float>(i) * e;
                    }
                    const float dis = (expsum <= 0.0F) ? 0.0F : (exwsum / expsum);
                    ltrb[k] = dis * static_cast<float>(cand.stride);
                }
            }

            const float cx = (static_cast<float>(cand.w) + 0.5F) * static_cast<float>(cand.stride);
            const float cy = (static_cast<float>(cand.h) + 0.5F) * static_cast<float>(cand.stride);

            Detection det{};
            det.x0 = cx - ltrb[0];
            det.y0 = cy - ltrb[1];
            det.x1 = cx + ltrb[2];
            det.y1 = cy + ltrb[3];
            det.score = Sigmoid(cand.cls_logit);
            det.class_id = cand.class_id;
            dets.push_back(det);
        }

        Nms(&dets, opt_.nms_threshold, static_cast<std::size_t>(opt_.max_det), opt_.class_agnostic_nms);
        *out = std::move(dets);
        return true;
    }

    // Fallback: decode all cells.
    std::vector<Detection> dets;
    dets.reserve(1024);
    for (std::size_t si = 0; si < nstrides; ++si) {
        const int stride = opt_.strides[si];
        const auto& tv_cls = pairs[si].tv_cls;
        const auto& tv_reg = pairs[si].tv_reg;
        const int feat_h = tv_cls.feat_h;
        const int feat_w = tv_cls.feat_w;
        const int chans = tv_cls.channels;
        for (int h = 0; h < feat_h; ++h) {
            for (int w = 0; w < feat_w; ++w) {
                int best_cls = 0;
                float best_logit = -std::numeric_limits<float>::infinity();
                if (tv_cls.layout == Layout::kNHWC) {
                    // Contiguous argmax via hoisted raw pointer (vectorizable), not per-element At().
                    const float* cls_ptr =
                        tv_cls.ptr + static_cast<std::ptrdiff_t>(h * feat_w + w) * chans;
                    best_logit = cls_ptr[0];
                    best_cls = 0;
                    for (int c = 1; c < cls; ++c) {
                        if (cls_ptr[c] > best_logit) {
                            best_logit = cls_ptr[c];
                            best_cls = c;
                        }
                    }
                } else {
                    for (int c = 0; c < cls; ++c) {
                        const float v = At(tv_cls, h, w, c);
                        if (v > best_logit) {
                            best_logit = v;
                            best_cls = c;
                        }
                    }
                }
                if (best_logit < logit_thr) continue;
                const float score = Sigmoid(best_logit);
                if (score < conf) continue;

                const int idx = h * feat_w + w;
                float ltrb[4];
                if (tv_reg.layout == Layout::kNHWC) {
                    const float* cell = tv_reg.ptr + static_cast<std::ptrdiff_t>(idx) * tv_reg.channels;
                    for (int k = 0; k < 4; ++k) {
                        const float* p = cell + k * reg;
                        float alpha = p[0];
                        for (int i = 1; i < reg; ++i) alpha = std::max(alpha, p[i]);
                        float expsum = 0.0F;
                        float exwsum = 0.0F;
                        for (int i = 0; i < reg; ++i) {
                            const float e = std::exp(p[i] - alpha);
                            expsum += e;
                            exwsum += static_cast<float>(i) * e;
                        }
                        const float dis = (expsum <= 0.0F) ? 0.0F : (exwsum / expsum);
                        ltrb[k] = dis * static_cast<float>(stride);
                    }
                } else {
                    const int hw = tv_reg.feat_h * tv_reg.feat_w;
                    for (int k = 0; k < 4; ++k) {
                        const int base = k * reg;
                        float alpha = tv_reg.ptr[(base + 0) * hw + idx];
                        for (int i = 1; i < reg; ++i) {
                            alpha = std::max(alpha, tv_reg.ptr[(base + i) * hw + idx]);
                        }
                        float expsum = 0.0F;
                        float exwsum = 0.0F;
                        for (int i = 0; i < reg; ++i) {
                            const float raw = tv_reg.ptr[(base + i) * hw + idx];
                            const float e = std::exp(raw - alpha);
                            expsum += e;
                            exwsum += static_cast<float>(i) * e;
                        }
                        const float dis = (expsum <= 0.0F) ? 0.0F : (exwsum / expsum);
                        ltrb[k] = dis * static_cast<float>(stride);
                    }
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
                dets.push_back(det);
            }
        }
    }

    Nms(&dets, opt_.nms_threshold, static_cast<std::size_t>(opt_.max_det), opt_.class_agnostic_nms);
    *out = std::move(dets);
    return true;
}

bool AxModelYolo26::Init(const YoloDetOptions& opt, std::string* error) {
    opt_ = opt;
    return AxModelBase::Init(opt_.base, error);
}

bool AxModelYolo26::ValidateModel(std::string* error) {
    if (opt_.strides.empty()) {
        if (error) *error = "strides is empty";
        return false;
    }
    if (opt_.num_classes <= 0) {
        if (error) *error = "invalid num_classes";
        return false;
    }
    return true;
}

// YOLO26 head: anchor-free, DFL-free, NMS-free. Two tensors per stride:
//   - cls: channels = num_classes
//   - box: channels = 4  (raw l,t,r,b distances in grid units; no DFL/softmax)
// Decode per cell: score = sigmoid(max class logit); box = (cx +/- d*stride).
// A light NMS is still applied as a safety net (the one-to-one head is NMS-free,
// so it is effectively a no-op; set nms_threshold >= 1.0 to disable entirely).
bool AxModelYolo26::Postprocess(const std::vector<TensorView>& outputs,
                                const LetterboxInfo& /*lb*/,
                                std::uint32_t /*src_w*/,
                                std::uint32_t /*src_h*/,
                                std::vector<Detection>* out,
                                std::string* error) {
    if (!out) return false;
    out->clear();

    const std::size_t nstrides = opt_.strides.size();
    const std::size_t expected_outputs = nstrides * 2U;
    if (outputs.size() != expected_outputs) {
        if (error) *error = "unexpected output tensor count: got " + std::to_string(outputs.size()) +
                            " expect " + std::to_string(expected_outputs);
        return false;
    }

    const int cls = opt_.num_classes;
    const int box_ch = 4;
    const float conf = opt_.conf_threshold;
    const float logit_thr = Logit(conf);

    struct Pair {
        FeatureView tv_cls{};
        FeatureView tv_box{};
        bool has_cls{false};
        bool has_box{false};
    };
    std::vector<Pair> pairs(nstrides);

    const auto in = input_spec();
    std::vector<std::pair<int, int>> expected_feat(nstrides);
    for (std::size_t i = 0; i < nstrides; ++i) {
        const int stride = opt_.strides[i];
        expected_feat[i] = {stride > 0 ? static_cast<int>(in.height / static_cast<std::uint32_t>(stride)) : 0,
                            stride > 0 ? static_cast<int>(in.width / static_cast<std::uint32_t>(stride)) : 0};
    }
    auto find_stride_index = [&](int feat_h, int feat_w) -> int {
        for (std::size_t i = 0; i < nstrides; ++i) {
            if (expected_feat[i].first == feat_h && expected_feat[i].second == feat_w) return static_cast<int>(i);
        }
        return -1;
    };

    // Match cls (channels==num_classes) vs box (channels==4) tensors to strides by feature-map size.
    // When num_classes==4 the two are ambiguous by channel count, so fall back to declaration order
    // (even index = box, odd = cls per stride), which matches the AXERA export order.
    const bool ambiguous = (cls == box_ch);
    for (std::size_t oi = 0; oi < outputs.size(); ++oi) {
        FeatureView tv{};
        if (!ambiguous && MakeFeatureView(outputs[oi], cls, &tv) && tv.channels == cls) {
            const int si = find_stride_index(tv.feat_h, tv.feat_w);
            if (si < 0) { if (error) *error = "unmatched cls output at index " + std::to_string(oi); return false; }
            pairs[static_cast<std::size_t>(si)].tv_cls = tv;
            pairs[static_cast<std::size_t>(si)].has_cls = true;
            continue;
        }
        if (MakeFeatureView(outputs[oi], box_ch, &tv) && tv.channels == box_ch) {
            const int si = find_stride_index(tv.feat_h, tv.feat_w);
            if (si < 0) { if (error) *error = "unmatched box output at index " + std::to_string(oi); return false; }
            const bool as_cls = ambiguous && (oi % 2U == 1U);  // odd -> cls in the ambiguous nc==4 case
            if (as_cls) {
                pairs[static_cast<std::size_t>(si)].tv_cls = tv;
                pairs[static_cast<std::size_t>(si)].has_cls = true;
            } else {
                pairs[static_cast<std::size_t>(si)].tv_box = tv;
                pairs[static_cast<std::size_t>(si)].has_box = true;
            }
            continue;
        }
        if (error) *error = "unexpected yolo26 output channels at index " + std::to_string(oi);
        return false;
    }
    for (std::size_t i = 0; i < nstrides; ++i) {
        if (!pairs[i].has_cls || !pairs[i].has_box) {
            if (error) *error = "missing cls/box output for stride index " + std::to_string(i);
            return false;
        }
    }

    std::vector<Detection> dets;
    dets.reserve(256);
    for (std::size_t si = 0; si < nstrides; ++si) {
        const int stride = opt_.strides[si];
        const FeatureView& tvc = pairs[si].tv_cls;
        const FeatureView& tvb = pairs[si].tv_box;
        const int fh = tvc.feat_h;
        const int fw = tvc.feat_w;
        const bool cls_nhwc = (tvc.layout == Layout::kNHWC);
        const std::ptrdiff_t cstep = cls_nhwc ? 1 : static_cast<std::ptrdiff_t>(fh) * fw;
        const bool box_nhwc = (tvb.layout == Layout::kNHWC);
        const std::ptrdiff_t bstep = box_nhwc ? 1 : static_cast<std::ptrdiff_t>(tvb.feat_h) * tvb.feat_w;
        for (int h = 0; h < fh; ++h) {
            for (int w = 0; w < fw; ++w) {
                const int idx = h * fw + w;
                const float* cls0 = cls_nhwc ? tvc.ptr + static_cast<std::ptrdiff_t>(idx) * tvc.channels : tvc.ptr + idx;
                int best_cls = 0;
                float best_logit = cls0[0];
                for (int c = 1; c < cls; ++c) {
                    const float v = cls0[static_cast<std::ptrdiff_t>(c) * cstep];
                    if (v > best_logit) { best_logit = v; best_cls = c; }
                }
                if (best_logit < logit_thr) continue;
                const float score = Sigmoid(best_logit);
                if (score < conf) continue;

                const float* b0 = box_nhwc ? tvb.ptr + static_cast<std::ptrdiff_t>(idx) * tvb.channels : tvb.ptr + idx;
                const float dl = b0[0];
                const float dt = b0[static_cast<std::ptrdiff_t>(1) * bstep];
                const float dr = b0[static_cast<std::ptrdiff_t>(2) * bstep];
                const float db = b0[static_cast<std::ptrdiff_t>(3) * bstep];

                const float cx = (static_cast<float>(w) + 0.5F) * static_cast<float>(stride);
                const float cy = (static_cast<float>(h) + 0.5F) * static_cast<float>(stride);

                Detection det{};
                det.x0 = cx - dl * static_cast<float>(stride);
                det.y0 = cy - dt * static_cast<float>(stride);
                det.x1 = cx + dr * static_cast<float>(stride);
                det.y1 = cy + db * static_cast<float>(stride);
                det.score = score;
                det.class_id = best_cls;
                dets.push_back(det);
            }
        }
    }

    if (opt_.pre_nms_topk > 0) {
        LimitTopK(&dets, static_cast<std::size_t>(opt_.pre_nms_topk));
    }
    Nms(&dets, opt_.nms_threshold, static_cast<std::size_t>(opt_.max_det), opt_.class_agnostic_nms);
    *out = std::move(dets);
    return true;
}

}  // namespace axpipeline::npu
