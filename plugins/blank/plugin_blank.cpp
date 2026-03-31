#define AX_PLUGIN_BUILD_DLL 1

#include "ax_plugin/ax_plugin.h"

#include <cstdlib>

extern "C" {

int ax_plugin_get_api_version(void) {
    return AX_PLUGIN_API_VERSION;
}

int ax_plugin_init(const char* /*init_json*/, int32_t /*device_id*/, ax_plugin_handle_t* out_handle) {
    if (out_handle == nullptr) {
        return -1;
    }
    *out_handle = reinterpret_cast<ax_plugin_handle_t>(0x1);  // non-null sentinel
    return 0;
}

void ax_plugin_deinit(ax_plugin_handle_t /*handle*/) {}

int ax_plugin_infer(ax_plugin_handle_t /*handle*/,
                    const ax_plugin_image_view_t* /*image*/,
                    ax_plugin_det_result_t* out_result) {
    // Test hook: allow injecting a hard crash to verify subprocess isolation.
    // Only enabled when AX_PLUGIN_BLANK_CRASH=1 in the process environment.
    const char* crash = std::getenv("AX_PLUGIN_BLANK_CRASH");
    if (crash && crash[0] == '1') {
        std::abort();
    }

    if (out_result == nullptr) {
        return -1;
    }
    out_result->dets = nullptr;
    out_result->det_count = 0;
    return 0;
}

void ax_plugin_release_result(ax_plugin_handle_t /*handle*/, ax_plugin_det_result_t* result) {
    if (result == nullptr) {
        return;
    }
    result->dets = nullptr;
    result->det_count = 0;
}

}  // extern "C"
