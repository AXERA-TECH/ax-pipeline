#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "ai/ax_detection.hpp"
#include "common/ax_image.h"

namespace axpipeline::plugin {

// Run a plugin in a separate process to isolate crashes (SIGSEGV/abort) from the main pipeline.
// Note: this mode may require extra copies (e.g. AXCL device -> host) and can be slower than in-proc.
class AxPluginProcess {
public:
    AxPluginProcess() = default;
    ~AxPluginProcess();

    AxPluginProcess(const AxPluginProcess&) = delete;
    AxPluginProcess& operator=(const AxPluginProcess&) = delete;

    bool Open(const std::string& so_path,
              const std::string& init_json,
              std::int32_t device_id,
              std::string* error);
    void Close() noexcept;

    bool Infer(const axvsdk::common::AxImage& image,
               std::vector<axpipeline::ai::Detection>* out,
               std::string* error);

private:
    bool Spawn(std::string* error);
    bool SendInit(std::string* error);
    bool EnsureRunning(std::string* error);

    bool SendInferRequest(const axvsdk::common::AxImage& image,
                          std::vector<axpipeline::ai::Detection>* out,
                          std::string* error);

    void KillChild() noexcept;

    std::string so_path_{};
    std::string init_json_{};
    std::int32_t device_id_{-1};

    int fd_{-1};
    int child_pid_{-1};

    // Restart/backoff to avoid tight crash loops.
    std::uint64_t restart_failures_{0};
    std::uint64_t last_restart_ms_{0};
};

}  // namespace axpipeline::plugin

