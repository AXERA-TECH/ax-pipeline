#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "ai/ax_detection.hpp"
#include "ax_plugin/ax_plugin.h"
#include "common/ax_image.h"

namespace axpipeline::plugin {

class AxPlugin {
public:
    AxPlugin() = default;
    ~AxPlugin();

    AxPlugin(const AxPlugin&) = delete;
    AxPlugin& operator=(const AxPlugin&) = delete;

    bool Open(const std::string& so_path,
              const std::string& init_json,
              std::int32_t device_id,
              std::string* error);
    void Close() noexcept;

    bool Infer(const axvsdk::common::AxImage& image,
               std::vector<axpipeline::ai::Detection>* out,
               std::string* error);

private:
    void* lib_{nullptr};
    ax_plugin_handle_t handle_{nullptr};

    int (*get_api_version_)() = nullptr;
    int (*init_)(const char*, std::int32_t, ax_plugin_handle_t*) = nullptr;
    void (*deinit_)(ax_plugin_handle_t) = nullptr;
    int (*infer_)(ax_plugin_handle_t, const ax_plugin_image_view_t*, ax_plugin_det_result_t*) = nullptr;
    void (*release_)(ax_plugin_handle_t, ax_plugin_det_result_t*) = nullptr;
};

}  // namespace axpipeline::plugin

