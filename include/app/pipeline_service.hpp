#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "app/pipeline_instance.hpp"
#include "config_loader.hpp"

namespace axpipeline::app {

class PipelineService {
public:
    PipelineService() = default;
    ~PipelineService();

    PipelineService(const PipelineService&) = delete;
    PipelineService& operator=(const PipelineService&) = delete;

    bool AddPipeline(const ConfigLoader::PipelineCfg& cfg, bool autostart, std::string* error);
    bool RemovePipeline(const std::string& name, std::string* error);
    bool StartPipeline(const std::string& name, std::string* error);
    bool StopPipeline(const std::string& name, std::string* error);
    bool ReplacePipeline(const ConfigLoader::PipelineCfg& cfg, bool autostart, std::string* error);
    bool UpdatePipelineNpu(const std::string& name,
                           const ConfigLoader::PipelineCfg::NpuCfg& npu,
                           double npu_max_fps,
                           bool autostart,
                           std::string* error);
    bool AddPipelineOutput(const std::string& name,
                           const axvsdk::pipeline::PipelineOutputConfig& output,
                           std::size_t* out_index,
                           std::string* error);
    bool RemovePipelineOutput(const std::string& name, std::size_t index, std::string* error);

    std::vector<PipelineSnapshot> ListSnapshots() const;
    bool GetPreviewJpeg(const std::string& name,
                        const PreviewOptions& opt,
                        std::vector<std::uint8_t>* out_jpeg,
                        std::string* error) const;
    bool GetPipelineConfig(const std::string& name, ConfigLoader::PipelineCfg* out, std::string* error) const;

    void StopAll() noexcept;

private:
    std::shared_ptr<PipelineInstance> GetInstanceLocked(const std::string& name) const;

    mutable std::mutex mu_;
    std::unordered_map<std::string, std::shared_ptr<PipelineInstance>> pipelines_;
};

}  // namespace axpipeline::app
