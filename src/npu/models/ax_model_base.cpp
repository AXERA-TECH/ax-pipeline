#include "npu/models/ax_model_base.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "npu/runner/ax_model_runner.hpp"

// Runner implementations (kept in src to avoid polluting public headers).
#if defined(AXPIPELINE_HAVE_AXCL)
#include "../runner/axcl/ax_model_runner_axcl.hpp"
#include "../runner/axcl/axcl_manager.h"
#endif

#if defined(AXPIPELINE_HAVE_MSP)
#include "../runner/ax650/ax_model_runner_ax650.hpp"
#endif

namespace axpipeline::npu {

namespace {

std::uint64_t NowUs() {
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now().time_since_epoch())
            .count());
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

std::size_t BytesPerPixel(axvsdk::common::PixelFormat fmt, std::size_t plane) noexcept {
    switch (fmt) {
        case axvsdk::common::PixelFormat::kBgr24:
        case axvsdk::common::PixelFormat::kRgb24:
            return (plane == 0) ? 3U : 0U;
        case axvsdk::common::PixelFormat::kNv12:
            return 1U;
        case axvsdk::common::PixelFormat::kUnknown:
        default:
            return 0U;
    }
}

BackendType NormalizeBackend(BackendType backend) noexcept {
    if (backend != BackendType::kAuto) return backend;
#if defined(AXPIPELINE_DEFAULT_BACKEND_AXCL)
    return BackendType::kAxcl;
#else
    return BackendType::kAxMsp;
#endif
}

}  // namespace

AxModelBase::~AxModelBase() noexcept = default;

LetterboxInfo AxModelBase::ComputeLetterbox(std::uint32_t src_w,
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

void AxModelBase::UndoLetterbox(const LetterboxInfo& lb,
                                std::uint32_t src_w,
                                std::uint32_t src_h,
                                std::vector<Detection>* dets) {
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

bool AxModelBase::EnsureImageProcessor(std::string* error) {
    if (imgproc_) return true;
    imgproc_ = axvsdk::common::CreateImageProcessor();
    if (!imgproc_) {
        if (error) *error = "CreateImageProcessor failed";
        return false;
    }
    return true;
}

bool AxModelBase::Init(const ModelInitOptions& opt, std::string* error) {
    Deinit();
    opt_ = opt;
    opt_.backend = NormalizeBackend(opt_.backend);

    if (opt_.model_path.empty()) {
        if (error) *error = "model_path is empty";
        return false;
    }
    if (!ReadFileToBuffer(opt_.model_path, &model_bytes_, error)) {
        return false;
    }

    runner_ = std::make_unique<RunnerHolder>();
    runner_->backend = opt_.backend;

    if (runner_->backend == BackendType::kAxcl) {
#if defined(AXPIPELINE_HAVE_AXCL)
        runner_->runner = std::make_shared<ax_runner_axcl>();
#else
        if (error) *error = "AXCL backend not enabled at build time";
        return false;
#endif
    } else {
#if defined(AXPIPELINE_HAVE_MSP)
        runner_->runner = std::make_shared<ax_runner_ax650>();
#else
        if (error) *error = "MSP backend not enabled at build time";
        return false;
#endif
    }

    if (!runner_->runner) {
        if (error) *error = "create runner failed";
        return false;
    }

#if defined(AXPIPELINE_HAVE_AXCL)
    if (runner_->backend == BackendType::kAxcl) {
        // Ensure AXCL worker for this device is running before the runner allocates any buffers.
        if (opt_.device_id < 0) {
            opt_.device_id = 0;
        }
        if (!axcl_Dev_IsInit(opt_.device_id)) {
            if (axcl_Dev_Init(opt_.device_id) != 0) {
                if (error) *error = "axcl_Dev_Init failed for device " + std::to_string(opt_.device_id);
                return false;
            }
        }
    }
#endif

    const int ret = runner_->runner->init(model_bytes_.data(),
                                          static_cast<unsigned int>(model_bytes_.size()),
                                          opt_.device_id);
    if (ret != 0) {
        if (error) *error = "runner init failed: ret=" + std::to_string(ret);
        return false;
    }

    // Infer model input spec from runner input tensor.
    if (runner_->runner->get_num_inputs() < 1) {
        if (error) *error = "model has no inputs";
        return false;
    }
    const auto& in = runner_->runner->get_input(0);
    if (in.vShape.size() < 3) {
        if (error) *error = "invalid input shape";
        return false;
    }

    const std::uint32_t w = static_cast<std::uint32_t>(in.vShape[2]);
    const std::uint32_t h_shape = static_cast<std::uint32_t>(in.vShape[1]);
    if (w == 0 || h_shape == 0 || in.nSize <= 0) {
        if (error) *error = "invalid input meta";
        return false;
    }

    axvsdk::common::PixelFormat fmt = axvsdk::common::PixelFormat::kUnknown;
    std::uint32_t h = h_shape;

    const std::uint64_t bytes = static_cast<std::uint64_t>(in.nSize);
    const std::uint64_t wh = static_cast<std::uint64_t>(w) * static_cast<std::uint64_t>(h_shape);
    if (bytes == wh * 3ULL) {
        fmt = axvsdk::common::PixelFormat::kBgr24;
        h = h_shape;
    } else if (bytes == wh) {
        // Some toolchains expose NV12 as a single-channel buffer with height = H*3/2.
        fmt = axvsdk::common::PixelFormat::kNv12;
        h = static_cast<std::uint32_t>(static_cast<std::uint64_t>(h_shape) * 2ULL / 3ULL);
    } else if (bytes == (wh * 3ULL / 2ULL)) {
        fmt = axvsdk::common::PixelFormat::kNv12;
        h = h_shape;
    } else if (in.vShape.size() >= 4 && in.vShape[3] == 3) {
        fmt = axvsdk::common::PixelFormat::kBgr24;
        h = h_shape;
    } else {
        // Default for current supported detection models.
        fmt = axvsdk::common::PixelFormat::kBgr24;
        h = h_shape;
    }

    input_.width = w;
    input_.height = h;
    input_.format = fmt;

#if defined(AXPIPELINE_HAVE_AXCL)
    if (runner_->backend == BackendType::kAxcl) {
        // Allocate tight-packed device scratch for AXCL hardware preprocess.
        axvsdk::common::ImageDescriptor desc{};
        desc.format = input_.format;
        desc.width = input_.width;
        desc.height = input_.height;
        if (desc.format == axvsdk::common::PixelFormat::kNv12) {
            desc.strides[0] = desc.width;
            desc.strides[1] = desc.width;
        } else {
            desc.strides[0] = static_cast<std::size_t>(desc.width) * 3U;
        }
        axvsdk::common::ImageAllocationOptions alloc{};
        alloc.memory_type = axvsdk::common::MemoryType::kCmm;
        alloc.cache_mode = axvsdk::common::CacheMode::kNonCached;
        alloc.alignment = 0x1000;
        alloc.token = "AxModelScratch";
        scratch_ = axvsdk::common::AxImage::Create(desc, alloc);
        if (!scratch_) {
            if (error) *error = "allocate AXCL scratch image failed";
            return false;
        }
    }
#endif

    if (!ValidateModel(error)) {
        return false;
    }

    return true;
}

void AxModelBase::Deinit() {
    if (runner_ && runner_->runner) {
        runner_->runner->deinit();
    }
    runner_.reset();
    model_bytes_.clear();
    input_ = {};
    scratch_.reset();
    imgproc_.reset();
}

bool AxModelBase::PrepareInput(const axvsdk::common::AxImage& frame, std::string* error, LetterboxInfo* lb) {
    if (lb == nullptr) return false;
    *lb = ComputeLetterbox(frame.width(), frame.height(),
                           input_.width, input_.height,
                           opt_.h_align, opt_.v_align);

    if (!runner_ || !runner_->runner) {
        if (error) *error = "runner not initialized";
        return false;
    }

    // Fast-path: input frame is already in model input shape/format.
    // For AXCL, runner input staging is host memory, so we can memcpy without IVPS.
    if (frame.width() == input_.width && frame.height() == input_.height && frame.format() == input_.format) {
        auto& r = *runner_->runner;
        const auto& in = r.get_input(0);
        if (runner_->backend == BackendType::kAxcl) {
#if defined(AXPIPELINE_HAVE_AXCL)
            if (in.pVirAddr == nullptr || in.nSize <= 0) {
                if (error) *error = "invalid runner input buffer";
                return false;
            }
            const auto* src0 = frame.plane_data(0);
            if (src0 == nullptr) {
                if (error) *error = "input plane0 not accessible on host";
                return false;
            }

            if (input_.format == axvsdk::common::PixelFormat::kBgr24 ||
                input_.format == axvsdk::common::PixelFormat::kRgb24) {
                const std::size_t row_bytes = static_cast<std::size_t>(input_.width) * 3U;
                const std::size_t src_stride = frame.stride(0);
                auto* dst = static_cast<std::uint8_t*>(in.pVirAddr);
                if (src_stride == row_bytes) {
                    std::memcpy(dst, src0, row_bytes * input_.height);
                } else {
                    for (std::uint32_t y = 0; y < input_.height; ++y) {
                        std::memcpy(dst + y * row_bytes, src0 + y * src_stride, row_bytes);
                    }
                }
                return true;
            }
            if (input_.format == axvsdk::common::PixelFormat::kNv12) {
                const auto* src1 = frame.plane_data(1);
                if (src1 == nullptr) {
                    if (error) *error = "input plane1 not accessible on host";
                    return false;
                }
                const std::size_t y_row = static_cast<std::size_t>(input_.width);
                const std::size_t y_stride = frame.stride(0);
                const std::size_t uv_stride = frame.stride(1);
                auto* dst = static_cast<std::uint8_t*>(in.pVirAddr);
                for (std::uint32_t y = 0; y < input_.height; ++y) {
                    std::memcpy(dst + y * y_row, src0 + y * y_stride, y_row);
                }
                const std::size_t y_bytes = y_row * input_.height;
                for (std::uint32_t y = 0; y < input_.height / 2U; ++y) {
                    std::memcpy(dst + y_bytes + y * y_row, src1 + y * uv_stride, y_row);
                }
                return true;
            }
#endif
        }
    }

    if (!EnsureImageProcessor(error)) return false;

    axvsdk::common::ImageProcessRequest req{};
    req.output_image.format = input_.format;
    req.output_image.width = input_.width;
    req.output_image.height = input_.height;
    req.resize.mode = opt_.resize_mode;
    req.resize.horizontal_align = opt_.h_align;
    req.resize.vertical_align = opt_.v_align;
    req.resize.background_color = opt_.background_color;

    auto& r = *runner_->runner;
    const auto& in = r.get_input(0);

    if (runner_->backend == BackendType::kAxcl) {
#if defined(AXPIPELINE_HAVE_AXCL)
        if (!scratch_) {
            if (error) *error = "AXCL scratch not allocated";
            return false;
        }
        if (!imgproc_->Process(frame, req, *scratch_)) {
            if (error) *error = "preprocess failed";
            return false;
        }

        // D2H: pack scratch bytes into runner input staging buffer.
        // Note: runner's input staging is host memory; device address is not host-accessible.
        if (in.pVirAddr == nullptr || in.nSize <= 0) {
            if (error) *error = "invalid runner input buffer";
            return false;
        }

        const auto phy = scratch_->physical_address(0);
        const auto vir = scratch_->virtual_address(0);
        const void* src = (phy != 0) ? reinterpret_cast<const void*>(static_cast<std::uintptr_t>(phy)) : vir;
        if (src == nullptr) {
            if (error) *error = "invalid AXCL scratch address";
            return false;
        }

        const auto cpy = axcl_Memcpy(in.pVirAddr, src, static_cast<std::size_t>(in.nSize),
                                     AXCL_MEMCPY_DEVICE_TO_HOST, opt_.device_id);
        if (cpy != 0) {
            if (error) *error = "axcl_Memcpy(D2H input) failed: ret=" + std::to_string(cpy);
            return false;
        }
        return true;
#else
        (void)frame;
        (void)req;
        if (error) *error = "AXCL backend not enabled at build time";
        return false;
#endif
    }

    // MSP: preprocess directly into runner input CMM buffer.
    if (in.pVirAddr == nullptr || in.phyAddr == 0 || in.nSize <= 0) {
        if (error) *error = "invalid runner input buffer";
        return false;
    }

    axvsdk::common::ImageDescriptor desc{};
    desc.format = input_.format;
    desc.width = input_.width;
    desc.height = input_.height;
    if (desc.format == axvsdk::common::PixelFormat::kNv12) {
        desc.strides[0] = desc.width;
        desc.strides[1] = desc.width;
    } else {
        desc.strides[0] = static_cast<std::size_t>(desc.width) * 3U;
    }

    std::array<axvsdk::common::ExternalImagePlane, axvsdk::common::kMaxImagePlanes> planes{};
    planes[0].virtual_address = in.pVirAddr;
    planes[0].physical_address = static_cast<std::uint64_t>(in.phyAddr);
    if (desc.format == axvsdk::common::PixelFormat::kNv12) {
        const std::size_t y_bytes = desc.strides[0] * desc.height;
        planes[1].virtual_address = static_cast<std::uint8_t*>(in.pVirAddr) + y_bytes;
        planes[1].physical_address = static_cast<std::uint64_t>(in.phyAddr) + y_bytes;
    }

    auto dst = axvsdk::common::AxImage::WrapExternal(desc, planes);
    if (!dst) {
        if (error) *error = "wrap runner input as AxImage failed";
        return false;
    }
    if (!imgproc_->Process(frame, req, *dst)) {
        if (error) *error = "preprocess failed";
        return false;
    }
    return true;
}

bool AxModelBase::RunRunner(std::string* error, std::vector<TensorView>* outputs) {
    if (outputs == nullptr) return false;
    outputs->clear();

    if (!runner_ || !runner_->runner) {
        if (error) *error = "runner not initialized";
        return false;
    }
    auto& r = *runner_->runner;

    // AXCL runner does sync in inference(); MSP runner needs explicit cache invalidation for outputs.
    const int ret = r.inference();
    if (ret != 0) {
        if (error) *error = "runner inference failed: ret=" + std::to_string(ret);
        return false;
    }

    const int nout = r.get_num_outputs();
    outputs->reserve(static_cast<std::size_t>(nout));
    for (int i = 0; i < nout; ++i) {
        if (runner_->backend != BackendType::kAxcl) {
            (void)r.sync_output(i);
        }
        const auto& o = r.get_output(i);
        TensorView tv{};
        tv.data = reinterpret_cast<const float*>(o.pVirAddr);
        tv.shape = o.vShape;
        tv.name = o.sName;
        tv.bytes = static_cast<std::size_t>(o.nSize);
        outputs->push_back(std::move(tv));
    }
    return true;
}

bool AxModelBase::Infer(const axvsdk::common::AxImage& frame,
                        std::vector<Detection>* out,
                        std::string* error,
                        RunTimings* timings) {
    if (out == nullptr) return false;
    out->clear();
    if (!runner_ || !runner_->runner) {
        if (error) *error = "model not initialized";
        return false;
    }

    RunTimings tm{};
    const std::uint64_t t_total0 = NowUs();

    LetterboxInfo lb{};
    const std::uint64_t t_pre0 = NowUs();
    if (!PrepareInput(frame, error, &lb)) return false;
    tm.preprocess_us = NowUs() - t_pre0;

    // For AXCL: input D2H above; H2D is done inside inference() (auto sync).
    // For MSP: input already in CMM; no sync needed.

    std::vector<TensorView> outputs;
    const std::uint64_t t_inf0 = NowUs();
    if (!RunRunner(error, &outputs)) return false;
    tm.infer_us = NowUs() - t_inf0;

    const std::uint64_t t_post0 = NowUs();
    if (!Postprocess(outputs, lb, frame.width(), frame.height(), out, error)) return false;
    // Common mapping: model-space -> source-space.
    if (opt_.resize_mode == axvsdk::common::ResizeMode::kKeepAspectRatio) {
        UndoLetterbox(lb, frame.width(), frame.height(), out);
    }
    tm.postprocess_us = NowUs() - t_post0;

    tm.total_us = NowUs() - t_total0;
    if (timings) *timings = tm;
    return true;
}

}  // namespace axpipeline::npu
