#include "npu/yolo_postprocess.hpp"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

namespace {

int g_fail = 0;

void ExpectTrue(bool v, const char* msg) {
    if (!v) {
        std::cerr << "EXPECT_TRUE failed: " << msg << "\n";
        g_fail++;
    }
}

void ExpectNear(float a, float b, float eps, const char* msg) {
    if (std::fabs(a - b) > eps) {
        std::cerr << "EXPECT_NEAR failed: " << msg << " a=" << a << " b=" << b << "\n";
        g_fail++;
    }
}

axpipeline::npu::HostTensor MakeFp32Tensor(const std::string& name, std::vector<int64_t> shape, std::vector<float> data) {
    axpipeline::npu::HostTensor t{};
    t.desc.name = name;
    t.desc.dtype = axpipeline::npu::DataType::kFp32;
    t.desc.shape = std::move(shape);
    t.desc.byte_size = data.size() * sizeof(float);
    t.bytes.resize(t.desc.byte_size);
    std::memcpy(t.bytes.data(), data.data(), t.bytes.size());
    return t;
}

}  // namespace

int main() {
    using namespace axpipeline::npu;

    // ---- letterbox mapping sanity ----
    {
        const auto lb = ComputeLetterbox(1920, 1080, 640, 640,
                                         axvsdk::common::ResizeAlign::kCenter,
                                         axvsdk::common::ResizeAlign::kCenter);
        // scale = min(640/1920, 640/1080) = 0.3333..., resized = 640x360, pad_y = 140
        ExpectNear(lb.scale, 640.0F / 1920.0F, 1e-4F, "lb.scale");
        ExpectNear(lb.pad_x, 0.0F, 1e-3F, "lb.pad_x");
        ExpectNear(lb.pad_y, (640.0F - 360.0F) * 0.5F, 1e-3F, "lb.pad_y");

        std::vector<Detection> dets;
        Detection d{};
        d.x0 = 0;
        d.y0 = lb.pad_y;
        d.x1 = 640;
        d.y1 = lb.pad_y + 360;
        dets.push_back(d);
        UndoLetterbox(lb, 1920, 1080, &dets);
        ExpectNear(dets[0].x0, 0.0F, 1.0F, "undo.x0");
        ExpectNear(dets[0].y0, 0.0F, 1.0F, "undo.y0");
        ExpectNear(dets[0].x1, 1919.0F, 2.0F, "undo.x1");
        ExpectNear(dets[0].y1, 1079.0F, 2.0F, "undo.y1");
    }

    // ---- yolov5 decode minimal (1x1 grid, only one anchor hot) ----
    {
        YoloPostprocessOptions opt{};
        opt.model_type = YoloModelType::kYolov5;
        opt.num_classes = 1;
        opt.conf_threshold = 0.10F;
        opt.nms_threshold = 0.50F;
        opt.yolov5_anchors = {
            10, 10, 10, 10, 10, 10,
            10, 10, 10, 10, 10, 10,
            10, 10, 10, 10, 10, 10,
        };

        // 3 outputs required; make stride8/16 empty-ish, stride32 has one positive.
        // For each cell: 3 anchors, each (cls+5)=6 floats. Total = 18 floats.
        std::vector<float> out32(18, 0.0F);
        // Anchor 0 at cell(0,0): dx/dy/dw/dh
        out32[0] = 0.0F;   // dx sigmoid=0.5 => cx ~ (0.5*2-0.5+0)*32 = 16
        out32[1] = 0.0F;   // cy ~ 16
        out32[2] = 0.0F;   // dw sigmoid=0.5 => bw ~ 0.25*4*10=10
        out32[3] = 0.0F;   // bh ~ 10
        out32[4] = 5.0F;   // obj
        out32[5] = 5.0F;   // cls

        HostTensor t8 = MakeFp32Tensor("o8", {1, 1, 1, 18}, std::vector<float>(18, -10.0F));
        HostTensor t16 = MakeFp32Tensor("o16", {1, 1, 1, 18}, std::vector<float>(18, -10.0F));
        HostTensor t32 = MakeFp32Tensor("o32", {1, 1, 1, 18}, out32);
        // Default layout in tests is NHWC.
        t8.desc.layout = TensorLayout::kNHWC;
        t16.desc.layout = TensorLayout::kNHWC;
        t32.desc.layout = TensorLayout::kNHWC;

        // Make model input size match 1x1 feature map for stride32.
        const LetterboxInfo lb = ComputeLetterbox(32, 32, 32, 32,
                                                  axvsdk::common::ResizeAlign::kCenter,
                                                  axvsdk::common::ResizeAlign::kCenter);
        std::vector<Detection> dets;
        std::string err;
        const bool ok = YoloPostprocess({t8, t16, t32}, opt, lb, 32, 32, &dets, &err);
        ExpectTrue(ok, "yolov5 postprocess ok");
        ExpectTrue(!dets.empty(), "yolov5 has dets");
        if (!dets.empty()) {
            // Box should be roughly centered at (16,16) in model space, mapped back to same (no letterbox change).
            ExpectNear(dets[0].x0, 11.0F, 2.0F, "yolov5.x0");
            ExpectNear(dets[0].y0, 11.0F, 2.0F, "yolov5.y0");
            ExpectNear(dets[0].x1, 21.0F, 2.0F, "yolov5.x1");
            ExpectNear(dets[0].y1, 21.0F, 2.0F, "yolov5.y1");
        }
    }

    // ---- yolov8 native decode minimal (1x1 grid, DFL bins peaked at 0) ----
    {
        YoloPostprocessOptions opt{};
        opt.model_type = YoloModelType::kYolov8;
        opt.num_classes = 1;
        opt.conf_threshold = 0.10F;
        opt.nms_threshold = 0.50F;

        // step = cls + 4*reg_max(16) = 65 floats per location.
        std::vector<float> out32(65, -10.0F);
        // make each reg distribution peak at bin0.
        for (int k = 0; k < 4; ++k) out32[k * 16 + 0] = 10.0F;
        // class logit high.
        out32[4 * 16 + 0] = 10.0F;

        HostTensor t8 = MakeFp32Tensor("o8", {1, 1, 1, 65}, std::vector<float>(65, -10.0F));
        HostTensor t16 = MakeFp32Tensor("o16", {1, 1, 1, 65}, std::vector<float>(65, -10.0F));
        HostTensor t32 = MakeFp32Tensor("o32", {1, 1, 1, 65}, out32);
        t8.desc.layout = TensorLayout::kNHWC;
        t16.desc.layout = TensorLayout::kNHWC;
        t32.desc.layout = TensorLayout::kNHWC;

        const LetterboxInfo lb = ComputeLetterbox(32, 32, 32, 32,
                                                  axvsdk::common::ResizeAlign::kCenter,
                                                  axvsdk::common::ResizeAlign::kCenter);
        std::vector<Detection> dets;
        std::string err;
        const bool ok = YoloPostprocess({t8, t16, t32}, opt, lb, 32, 32, &dets, &err);
        ExpectTrue(ok, "yolov8 postprocess ok");
        ExpectTrue(!dets.empty(), "yolov8 has dets");
        if (!dets.empty()) {
            ExpectNear(dets[0].x0, 16.0F, 2.0F, "yolov8.x0");
            ExpectNear(dets[0].y0, 16.0F, 2.0F, "yolov8.y0");
            ExpectNear(dets[0].x1, 16.0F, 2.0F, "yolov8.x1");
            ExpectNear(dets[0].y1, 16.0F, 2.0F, "yolov8.y1");
        }
    }

    if (g_fail) {
        std::cerr << "FAILED: " << g_fail << " checks\n";
        return 1;
    }
    std::cout << "OK\n";
    return 0;
}
