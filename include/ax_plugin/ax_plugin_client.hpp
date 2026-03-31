#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "ai/ax_detection.hpp"
#include "common/ax_image.h"

namespace axpipeline::plugin {

class AxPluginClient {
public:
    virtual ~AxPluginClient() = default;

    AxPluginClient(const AxPluginClient&) = delete;
    AxPluginClient& operator=(const AxPluginClient&) = delete;

    virtual bool Open(const std::string& so_path,
                      const std::string& init_json,
                      std::int32_t device_id,
                      std::string* error) = 0;
    virtual void Close() noexcept = 0;

    virtual bool Infer(const axvsdk::common::AxImage& image,
                       std::vector<axpipeline::ai::Detection>* out,
                       std::string* error) = 0;

protected:
    AxPluginClient() = default;
};

enum class AxPluginIsolationMode {
    kInProcess = 0,
    kSubprocess = 1,
};

std::unique_ptr<AxPluginClient> CreatePluginClient(AxPluginIsolationMode mode);

}  // namespace axpipeline::plugin

