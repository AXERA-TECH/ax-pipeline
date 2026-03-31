#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "common/ax_image.h"
#include "common/ax_image_processor.h"

class ax_runner_base;

namespace axpipeline::npu {

enum class BackendType {
    kAuto = 0,
    kAxMsp,  // AX650 / AX620E-family (AX630C etc.)
    kAxcl,   // AXCL runtime
};

struct ModelInputSpec {
    std::uint32_t width{0};
    std::uint32_t height{0};
    axvsdk::common::PixelFormat format{axvsdk::common::PixelFormat::kUnknown};
};

struct Detection {
    float x0{0};
    float y0{0};
    float x1{0};
    float y1{0};
    float score{0};
    int class_id{-1};
};

struct LetterboxInfo {
    float scale{1.0F};
    float pad_x{0.0F};
    float pad_y{0.0F};
    std::uint32_t dst_w{0};
    std::uint32_t dst_h{0};
};

struct ModelInitOptions {
    BackendType backend{BackendType::kAuto};
    int device_id{-1};
    std::string model_path;

    // Preprocess options for resizing source -> model input.
    axvsdk::common::ResizeMode resize_mode{axvsdk::common::ResizeMode::kKeepAspectRatio};
    axvsdk::common::ResizeAlign h_align{axvsdk::common::ResizeAlign::kCenter};
    axvsdk::common::ResizeAlign v_align{axvsdk::common::ResizeAlign::kCenter};
    std::uint32_t background_color{0};  // 0xRRGGBB
};

struct RunTimings {
    std::uint64_t preprocess_us{};
    std::uint64_t input_sync_us{};
    std::uint64_t infer_us{};
    std::uint64_t output_sync_us{};
    std::uint64_t postprocess_us{};
    std::uint64_t total_us{};
};

// Base class: handles model loading, preprocess and runner execution.
// Derived classes implement model-specific postprocess and return detections in source coordinates.
class AxModelBase {
public:
    struct TensorView {
        const float* data{nullptr};
        std::vector<unsigned int> shape;  // model-provided dims
        std::string name;
        std::size_t bytes{0};
    };

    virtual ~AxModelBase() noexcept;

    bool Init(const ModelInitOptions& opt, std::string* error);
    void Deinit();

    const ModelInputSpec& input_spec() const noexcept { return input_; }
    const ModelInitOptions& options() const noexcept { return opt_; }

    bool Infer(const axvsdk::common::AxImage& frame,
               std::vector<Detection>* out,
               std::string* error,
               RunTimings* timings = nullptr);

protected:
    virtual bool Postprocess(const std::vector<TensorView>& outputs,
                             const LetterboxInfo& lb,
                             std::uint32_t src_w,
                             std::uint32_t src_h,
                             std::vector<Detection>* out,
                             std::string* error) = 0;

    // Hook for derived classes to validate expected output tensor count/layout early.
    virtual bool ValidateModel(std::string* /*error*/) { return true; }

private:
    bool EnsureImageProcessor(std::string* error);
    bool PrepareInput(const axvsdk::common::AxImage& frame, std::string* error, LetterboxInfo* lb);
    bool RunRunner(std::string* error, std::vector<TensorView>* outputs);

    static LetterboxInfo ComputeLetterbox(std::uint32_t src_w,
                                          std::uint32_t src_h,
                                          std::uint32_t dst_w,
                                          std::uint32_t dst_h,
                                          axvsdk::common::ResizeAlign h_align,
                                          axvsdk::common::ResizeAlign v_align);

    static void UndoLetterbox(const LetterboxInfo& lb,
                              std::uint32_t src_w,
                              std::uint32_t src_h,
                              std::vector<Detection>* dets);

    ModelInitOptions opt_{};
    ModelInputSpec input_{};
    std::vector<char> model_bytes_{};

    std::unique_ptr<axvsdk::common::ImageProcessor> imgproc_{};

    // Opaque runner pointer (implemented in .cpp to keep headers clean).
    struct RunnerHolder {
        BackendType backend{BackendType::kAuto};
        std::shared_ptr<ax_runner_base> runner{};
    };
    std::unique_ptr<RunnerHolder> runner_;

    // For AXCL: device scratch (tight-packed) used for hardware preprocess before D2H to runner input staging.
    axvsdk::common::AxImage::Ptr scratch_{};
};

}  // namespace axpipeline::npu
