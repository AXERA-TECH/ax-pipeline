#include "app/pipeline_service.hpp"

#include <utility>

namespace axpipeline::app {

PipelineService::~PipelineService() {
    StopAll();
}

std::shared_ptr<PipelineInstance> PipelineService::GetInstanceLocked(const std::string& name) const {
    const auto it = pipelines_.find(name);
    if (it == pipelines_.end()) return {};
    return it->second;
}

bool PipelineService::AddPipeline(const ConfigLoader::PipelineCfg& cfg, bool autostart, std::string* error) {
    std::shared_ptr<PipelineInstance> inst;
    {
        std::lock_guard<std::mutex> lock(mu_);
        if (cfg.name.empty()) {
            if (error) *error = "pipeline name is empty";
            return false;
        }
        if (pipelines_.find(cfg.name) != pipelines_.end()) {
            if (error) *error = "pipeline already exists: " + cfg.name;
            return false;
        }
        inst = std::make_shared<PipelineInstance>(cfg);
        pipelines_.emplace(cfg.name, inst);
    }

    // Open/start outside the global lock.
    if (autostart) {
        std::string start_err;
        if (!StartPipeline(cfg.name, &start_err)) {
            (void)RemovePipeline(cfg.name, nullptr);
            if (error) *error = start_err.empty() ? "start pipeline failed" : start_err;
            return false;
        }
    }
    return true;
}

bool PipelineService::RemovePipeline(const std::string& name, std::string* error) {
    std::shared_ptr<PipelineInstance> inst;
    {
        std::lock_guard<std::mutex> lock(mu_);
        auto it = pipelines_.find(name);
        if (it == pipelines_.end()) {
            if (error) *error = "pipeline not found: " + name;
            return false;
        }
        inst = it->second;
        pipelines_.erase(it);
    }
    if (inst) {
        inst->Stop();
        inst->Close();
    }
    return true;
}

bool PipelineService::StartPipeline(const std::string& name, std::string* error) {
    std::shared_ptr<PipelineInstance> inst;
    {
        std::lock_guard<std::mutex> lock(mu_);
        inst = GetInstanceLocked(name);
        if (!inst) {
            if (error) *error = "pipeline not found: " + name;
            return false;
        }
    }
    return inst->Start(error);
}

bool PipelineService::StopPipeline(const std::string& name, std::string* error) {
    (void)error;
    std::shared_ptr<PipelineInstance> inst;
    {
        std::lock_guard<std::mutex> lock(mu_);
        inst = GetInstanceLocked(name);
        if (!inst) {
            if (error) *error = "pipeline not found: " + name;
            return false;
        }
    }
    inst->Stop();
    return true;
}

bool PipelineService::ReplacePipeline(const ConfigLoader::PipelineCfg& cfg, bool autostart, std::string* error) {
    std::shared_ptr<PipelineInstance> inst;
    {
        std::lock_guard<std::mutex> lock(mu_);
        inst = GetInstanceLocked(cfg.name);
        if (!inst) {
            if (error) *error = "pipeline not found: " + cfg.name;
            return false;
        }
    }
    return inst->Reconfigure(cfg, autostart, error);
}

bool PipelineService::UpdatePipelineNpu(const std::string& name,
                                       const ConfigLoader::PipelineCfg::NpuCfg& npu,
                                       double npu_max_fps,
                                       bool autostart,
                                       std::string* error) {
    std::shared_ptr<PipelineInstance> inst;
    {
        std::lock_guard<std::mutex> lock(mu_);
        inst = GetInstanceLocked(name);
        if (!inst) {
            if (error) *error = "pipeline not found: " + name;
            return false;
        }
    }
    return inst->UpdateNpu(npu, npu_max_fps, autostart, error);
}

bool PipelineService::AddPipelineOutput(const std::string& name,
                                       const axvsdk::pipeline::PipelineOutputConfig& output,
                                       std::size_t* out_index,
                                       std::string* error) {
    std::shared_ptr<PipelineInstance> inst;
    {
        std::lock_guard<std::mutex> lock(mu_);
        inst = GetInstanceLocked(name);
        if (!inst) {
            if (error) *error = "pipeline not found: " + name;
            return false;
        }
    }
    return inst->AddOutput(output, out_index, error);
}

bool PipelineService::RemovePipelineOutput(const std::string& name, std::size_t index, std::string* error) {
    std::shared_ptr<PipelineInstance> inst;
    {
        std::lock_guard<std::mutex> lock(mu_);
        inst = GetInstanceLocked(name);
        if (!inst) {
            if (error) *error = "pipeline not found: " + name;
            return false;
        }
    }
    return inst->RemoveOutput(index, error);
}

std::vector<PipelineSnapshot> PipelineService::ListSnapshots() const {
    std::vector<std::shared_ptr<PipelineInstance>> list;
    {
        std::lock_guard<std::mutex> lock(mu_);
        list.reserve(pipelines_.size());
        for (const auto& kv : pipelines_) {
            list.push_back(kv.second);
        }
    }

    std::vector<PipelineSnapshot> out;
    out.reserve(list.size());
    for (const auto& p : list) {
        if (!p) continue;
        out.push_back(p->Snapshot());
    }
    return out;
}

bool PipelineService::GetPreviewJpeg(const std::string& name,
                                    const PreviewOptions& opt,
                                    std::vector<std::uint8_t>* out_jpeg,
                                    std::string* error) const {
    std::shared_ptr<PipelineInstance> inst;
    {
        std::lock_guard<std::mutex> lock(mu_);
        inst = GetInstanceLocked(name);
        if (!inst) {
            if (error) *error = "pipeline not found: " + name;
            return false;
        }
    }
    return inst->GetPreviewJpeg(opt, out_jpeg, error);
}

bool PipelineService::GetPipelineConfig(const std::string& name, ConfigLoader::PipelineCfg* out, std::string* error) const {
    if (out == nullptr) return false;
    std::shared_ptr<PipelineInstance> inst;
    {
        std::lock_guard<std::mutex> lock(mu_);
        inst = GetInstanceLocked(name);
        if (!inst) {
            if (error) *error = "pipeline not found: " + name;
            return false;
        }
    }
    *out = inst->config();
    return true;
}

void PipelineService::StopAll() noexcept {
    std::vector<std::shared_ptr<PipelineInstance>> insts;
    {
        std::lock_guard<std::mutex> lock(mu_);
        for (auto& kv : pipelines_) {
            insts.push_back(std::move(kv.second));
        }
        pipelines_.clear();
    }
    for (auto& p : insts) {
        if (!p) continue;
        p->Stop();
        p->Close();
    }
}

}  // namespace axpipeline::app
