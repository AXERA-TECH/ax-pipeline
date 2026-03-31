#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "npu/models/ax_model_base.hpp"

namespace axpipeline::npu {

struct YoloDetOptions {
    ModelInitOptions base{};

    int num_classes{80};
    float conf_threshold{0.25F};
    float nms_threshold{0.45F};

    // Common YOLO strides, ordered to match output tensors.
    std::vector<int> strides{8, 16, 32};

    // YOLOv5 anchors in pixels, order is [stride8(3 pairs), stride16(3 pairs), stride32(3 pairs)].
    std::vector<float> yolov5_anchors = {
        10, 13, 16, 30, 33, 23,
        30, 61, 62, 45, 59, 119,
        116, 90, 156, 198, 373, 326,
    };

    // YOLOv8 native DFL reg_max.
    int yolov8_reg_max{16};
};

class AxModelYoloV5 final : public AxModelBase {
public:
    bool Init(const YoloDetOptions& opt, std::string* error);

private:
    bool ValidateModel(std::string* error) override;
    bool Postprocess(const std::vector<TensorView>& outputs,
                     const LetterboxInfo& lb,
                     std::uint32_t src_w,
                     std::uint32_t src_h,
                     std::vector<Detection>* out,
                     std::string* error) override;

    YoloDetOptions opt_{};
};

class AxModelYoloV8Native final : public AxModelBase {
public:
    bool Init(const YoloDetOptions& opt, std::string* error);

private:
    bool ValidateModel(std::string* error) override;
    bool Postprocess(const std::vector<TensorView>& outputs,
                     const LetterboxInfo& lb,
                     std::uint32_t src_w,
                     std::uint32_t src_h,
                     std::vector<Detection>* out,
                     std::string* error) override;

    YoloDetOptions opt_{};
};

}  // namespace axpipeline::npu

