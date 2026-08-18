// detector_proxy.hpp —— 检测/跟踪层
//
// 职责:dlopen 一个现成的检测插件(默认 libax_plugin_pcd.so,也可换 yolov5/yolov8 等),
// 把 init/infer/deinit/release 完整转发给它。检测与跟踪(ByteTrack)都发生在内层插件里,
// 本层不做任何算法,只负责"装饰器"的加载与生命周期。
#pragma once

#include <dlfcn.h>

#include <cstdint>
#include <string>

#include "ax_plugin/ax_plugin.h"

namespace pcdvlm {

class DetectorProxy {
public:
    ~DetectorProxy() { Close(); }

    // 返回 0 成功;-10 dlopen 失败;-11 符号缺失;其余为内层插件 init 的返回值。
    int Open(const std::string& so_path, const std::string& init_json, std::int32_t device_id) {
        Close();
        dl_ = dlopen(so_path.c_str(), RTLD_NOW | RTLD_LOCAL);
        if (!dl_) return -10;
        init_    = reinterpret_cast<InitFn>(dlsym(dl_, "ax_plugin_init"));
        infer_   = reinterpret_cast<InferFn>(dlsym(dl_, "ax_plugin_infer"));
        deinit_  = reinterpret_cast<DeinitFn>(dlsym(dl_, "ax_plugin_deinit"));
        release_ = reinterpret_cast<ReleaseFn>(dlsym(dl_, "ax_plugin_release_result"));
        if (!init_ || !infer_ || !deinit_) return -11;
        return init_(init_json.c_str(), device_id, &handle_);
    }

    int Infer(const ax_plugin_image_view_t* image, ax_plugin_det_result_t* out) {
        return infer_ ? infer_(handle_, image, out) : -1;
    }

    void Release(ax_plugin_det_result_t* result) {
        if (release_) release_(handle_, result);
    }

    void Close() {
        if (handle_ && deinit_) deinit_(handle_);
        handle_ = nullptr;
        if (dl_) dlclose(dl_);
        dl_ = nullptr;
        init_ = nullptr; infer_ = nullptr; deinit_ = nullptr; release_ = nullptr;
    }

private:
    using InitFn    = int (*)(const char*, std::int32_t, ax_plugin_handle_t*);
    using InferFn   = int (*)(ax_plugin_handle_t, const ax_plugin_image_view_t*, ax_plugin_det_result_t*);
    using DeinitFn  = void (*)(ax_plugin_handle_t);
    using ReleaseFn = void (*)(ax_plugin_handle_t, ax_plugin_det_result_t*);

    void* dl_{nullptr};
    ax_plugin_handle_t handle_{nullptr};
    InitFn init_{nullptr};
    InferFn infer_{nullptr};
    DeinitFn deinit_{nullptr};
    ReleaseFn release_{nullptr};
};

}  // namespace pcdvlm
