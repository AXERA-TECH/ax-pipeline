#include "ax_plugin/ax_plugin_loader.hpp"

#include <cstddef>
#include <cstdint>

#if defined(_WIN32)
#  include <windows.h>
#else
#  include <dlfcn.h>
#endif

namespace axpipeline::plugin {

namespace {

ax_plugin_pixel_format_e ToPluginFormat(axvsdk::common::PixelFormat fmt) noexcept {
    switch (fmt) {
    case axvsdk::common::PixelFormat::kNv12:
        return AX_PLUGIN_PIXEL_FORMAT_NV12;
    case axvsdk::common::PixelFormat::kRgb24:
        return AX_PLUGIN_PIXEL_FORMAT_RGB24;
    case axvsdk::common::PixelFormat::kBgr24:
        return AX_PLUGIN_PIXEL_FORMAT_BGR24;
    case axvsdk::common::PixelFormat::kUnknown:
    default:
        return AX_PLUGIN_PIXEL_FORMAT_UNKNOWN;
    }
}

ax_plugin_memory_type_e ToPluginMemType(axvsdk::common::MemoryType mem) noexcept {
    switch (mem) {
    case axvsdk::common::MemoryType::kCmm:
        return AX_PLUGIN_MEMORY_TYPE_CMM;
    case axvsdk::common::MemoryType::kPool:
        return AX_PLUGIN_MEMORY_TYPE_POOL;
    case axvsdk::common::MemoryType::kExternal:
        return AX_PLUGIN_MEMORY_TYPE_HOST;
    case axvsdk::common::MemoryType::kAuto:
    default:
        return AX_PLUGIN_MEMORY_TYPE_UNKNOWN;
    }
}

}  // namespace

AxPlugin::~AxPlugin() {
    Close();
}

bool AxPlugin::Open(const std::string& so_path,
                    const std::string& init_json,
                    std::int32_t device_id,
                    std::string* error) {
    Close();

    if (so_path.empty()) {
        if (error) *error = "plugin path is empty";
        return false;
    }

#if defined(_WIN32)
    HMODULE lib = LoadLibraryA(so_path.c_str());
    if (lib == nullptr) {
        if (error) *error = "LoadLibrary failed";
        return false;
    }
    lib_ = lib;
    auto load_sym = [&](const char* name) -> void* { return reinterpret_cast<void*>(GetProcAddress(lib, name)); };
#else
    void* lib = dlopen(so_path.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (lib == nullptr) {
        const char* dlerr = dlerror();  // dlerror() is stateful; call once.
        if (error) *error = std::string("dlopen failed: ") + (dlerr ? dlerr : "");
        return false;
    }
    lib_ = lib;
    auto load_sym = [&](const char* name) -> void* { return dlsym(lib, name); };
#endif

    get_api_version_ = reinterpret_cast<int (*)()>(load_sym("ax_plugin_get_api_version"));
    init_ = reinterpret_cast<int (*)(const char*, std::int32_t, ax_plugin_handle_t*)>(load_sym("ax_plugin_init"));
    deinit_ = reinterpret_cast<void (*)(ax_plugin_handle_t)>(load_sym("ax_plugin_deinit"));
    infer_ = reinterpret_cast<int (*)(ax_plugin_handle_t, const ax_plugin_image_view_t*, ax_plugin_det_result_t*)>(
        load_sym("ax_plugin_infer"));
    release_ = reinterpret_cast<void (*)(ax_plugin_handle_t, ax_plugin_det_result_t*)>(load_sym("ax_plugin_release_result"));

    if (!get_api_version_ || !init_ || !deinit_ || !infer_ || !release_) {
        if (error) *error = "plugin missing required symbols";
        Close();
        return false;
    }

    int api = 0;
    try {
        api = get_api_version_();
    } catch (const std::exception& e) {
        if (error) *error = std::string("plugin get_api_version threw: ") + e.what();
        Close();
        return false;
    } catch (...) {
        if (error) *error = "plugin get_api_version threw unknown exception";
        Close();
        return false;
    }
    if (api != AX_PLUGIN_API_VERSION) {
        if (error) *error = "plugin api version mismatch: got " + std::to_string(api) +
                            " expect " + std::to_string(AX_PLUGIN_API_VERSION);
        Close();
        return false;
    }

    ax_plugin_handle_t h = nullptr;
    int ret = 0;
    try {
        ret = init_(init_json.c_str(), device_id, &h);
    } catch (const std::exception& e) {
        if (error) *error = std::string("plugin init threw: ") + e.what();
        Close();
        return false;
    } catch (...) {
        if (error) *error = "plugin init threw unknown exception";
        Close();
        return false;
    }
    if (ret != 0 || h == nullptr) {
        if (error) *error = "plugin init failed: ret=" + std::to_string(ret);
        Close();
        return false;
    }

    handle_ = h;
    return true;
}

void AxPlugin::Close() noexcept {
    if (handle_ && deinit_) {
        try {
            deinit_(handle_);
        } catch (...) {
        }
    }
    handle_ = nullptr;

    get_api_version_ = nullptr;
    init_ = nullptr;
    deinit_ = nullptr;
    infer_ = nullptr;
    release_ = nullptr;

    if (lib_) {
#if defined(_WIN32)
        FreeLibrary(reinterpret_cast<HMODULE>(lib_));
#else
        dlclose(lib_);
#endif
        lib_ = nullptr;
    }
}

bool AxPlugin::Infer(const axvsdk::common::AxImage& image,
                     std::vector<axpipeline::ai::Detection>* out,
                     std::string* error) {
    if (out == nullptr) {
        if (error) *error = "output is null";
        return false;
    }
    out->clear();

    if (!handle_ || !infer_ || !release_) {
        if (error) *error = "plugin not opened";
        return false;
    }

    ax_plugin_image_view_t view{};
    view.format = ToPluginFormat(image.format());
    view.width = image.width();
    view.height = image.height();
    view.plane_count = image.plane_count();
    view.memory_type = ToPluginMemType(image.memory_type());

#if defined(AXPIPELINE_APP_PLATFORM_AXCL)
    // On AXCL builds, CMM/POOL are device-side memory. Treat them as AXCL device buffers.
    if (image.memory_type() != axvsdk::common::MemoryType::kExternal) {
        view.memory_type = AX_PLUGIN_MEMORY_TYPE_AXCL_DEVICE;
    }
#endif

    for (std::size_t i = 0; i < 3; ++i) {
        view.strides[i] = 0;
        view.physical_addrs[i] = 0;
        view.virtual_addrs[i] = nullptr;
        view.block_ids[i] = 0xFFFFFFFFU;
    }
    for (std::size_t i = 0; i < view.plane_count && i < 3; ++i) {
        view.strides[i] = image.stride(i);
        view.physical_addrs[i] = image.physical_address(i);
        view.virtual_addrs[i] = const_cast<void*>(image.virtual_address(i));
        view.block_ids[i] = image.block_id(i);
    }

    ax_plugin_det_result_t result{};
    int ret = 0;
    try {
        ret = infer_(handle_, &view, &result);
    } catch (const std::exception& e) {
        if (error) *error = std::string("plugin infer threw: ") + e.what();
        return false;
    } catch (...) {
        if (error) *error = "plugin infer threw unknown exception";
        return false;
    }
    if (ret != 0) {
        if (error) *error = "plugin infer failed: ret=" + std::to_string(ret);
        return false;
    }

    if (result.dets != nullptr && result.det_count > 0) {
        out->reserve(result.det_count);
        for (std::size_t i = 0; i < result.det_count; ++i) {
            const auto& d = result.dets[i];
            axpipeline::ai::Detection dd{};
            dd.x0 = d.x0;
            dd.y0 = d.y0;
            dd.x1 = d.x1;
            dd.y1 = d.y1;
            dd.score = d.score;
            dd.class_id = static_cast<int>(d.class_id);
            out->push_back(dd);
        }
    }

    try {
        release_(handle_, &result);
    } catch (...) {
        // Best-effort: result memory is owned by plugin; if release throws, treat as fatal for this call
        // but keep pipeline running.
        if (error) *error = "plugin release_result threw";
        return false;
    }
    return true;
}

}  // namespace axpipeline::plugin
