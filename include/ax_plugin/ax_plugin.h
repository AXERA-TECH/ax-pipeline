#pragma once

#include <stddef.h>
#include <stdint.h>

#if defined(_WIN32)
#  if defined(AX_PLUGIN_BUILD_DLL)
#    define AX_PLUGIN_API __declspec(dllexport)
#  else
#    define AX_PLUGIN_API __declspec(dllimport)
#  endif
#else
#  define AX_PLUGIN_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define AX_PLUGIN_API_VERSION 2

typedef enum ax_plugin_pixel_format_e {
    AX_PLUGIN_PIXEL_FORMAT_UNKNOWN = 0,
    AX_PLUGIN_PIXEL_FORMAT_NV12 = 1,
    AX_PLUGIN_PIXEL_FORMAT_RGB24 = 2,
    AX_PLUGIN_PIXEL_FORMAT_BGR24 = 3,
} ax_plugin_pixel_format_e;

typedef enum ax_plugin_memory_type_e {
    AX_PLUGIN_MEMORY_TYPE_UNKNOWN = 0,
    AX_PLUGIN_MEMORY_TYPE_HOST = 1,
    AX_PLUGIN_MEMORY_TYPE_CMM = 2,
    AX_PLUGIN_MEMORY_TYPE_POOL = 3,
    // AXCL device memory: pointers are device addresses, not directly CPU-readable.
    AX_PLUGIN_MEMORY_TYPE_AXCL_DEVICE = 4,
} ax_plugin_memory_type_e;

typedef struct ax_plugin_image_view_t {
    ax_plugin_pixel_format_e format;
    uint32_t width;
    uint32_t height;
    size_t plane_count;
    size_t strides[3];           // bytes per row
    uint64_t physical_addrs[3];  // 0 if unknown
    void* virtual_addrs[3];      // host ptr or device ptr (AXCL)
    uint32_t block_ids[3];       // AX pool block id if available, otherwise 0xFFFFFFFF
    ax_plugin_memory_type_e memory_type;
} ax_plugin_image_view_t;

typedef struct ax_plugin_det_t {
    float x0;
    float y0;
    float x1;
    float y1;
    float score;
    int32_t class_id;
    // Stable object id from tracker. -1 means "not available".
    int64_t track_id;
} ax_plugin_det_t;

typedef struct ax_plugin_det_result_t {
    const ax_plugin_det_t* dets;
    size_t det_count;
} ax_plugin_det_result_t;

typedef void* ax_plugin_handle_t;

AX_PLUGIN_API int ax_plugin_get_api_version(void);

// 可选导出:返回插件配置 schema(静态 JSON 字符串,UTF-8),供控制台等宿主动态生成配置界面:
//   {
//     "label":   "人类可读名称",
//     "defaults": { ...完整的 ax_plugin_init_info 默认值... },
//     "fields": [ {"key":"model_path","label":"模型路径","type":"string","required":true},
//                 {"key":"vlm.url","label":"VLM服务","type":"string"}, ... ]   // 重点字段(嵌套用点路径)
//   }
// 旧插件可不实现;宿主用 dlsym 探测,取不到则视为无 schema。
AX_PLUGIN_API const char* ax_plugin_get_config_schema(void);

// init_json: a JSON string for plugin-specific initialization options.
// device_id: used for AXCL multi-card selection (pass -1 for default).
AX_PLUGIN_API int ax_plugin_init(const char* init_json, int32_t device_id, ax_plugin_handle_t* out_handle);
AX_PLUGIN_API void ax_plugin_deinit(ax_plugin_handle_t handle);

AX_PLUGIN_API int ax_plugin_infer(ax_plugin_handle_t handle,
                                 const ax_plugin_image_view_t* image,
                                 ax_plugin_det_result_t* out_result);

// Release resources associated with out_result (if any).
AX_PLUGIN_API void ax_plugin_release_result(ax_plugin_handle_t handle, ax_plugin_det_result_t* result);

#ifdef __cplusplus
}  // extern "C"
#endif
