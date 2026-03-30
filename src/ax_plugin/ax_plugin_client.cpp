#include "ax_plugin/ax_plugin_client.hpp"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "ax_plugin/ax_plugin_loader.hpp"
#include "ax_plugin/ax_plugin_process.hpp"

namespace axpipeline::plugin {

namespace {

class InprocClient final : public AxPluginClient {
public:
    bool Open(const std::string& so_path,
              const std::string& init_json,
              std::int32_t device_id,
              std::string* error) override {
        return impl_.Open(so_path, init_json, device_id, error);
    }

    void Close() noexcept override { impl_.Close(); }

    bool Infer(const axvsdk::common::AxImage& image,
               std::vector<axpipeline::ai::Detection>* out,
               std::string* error) override {
        return impl_.Infer(image, out, error);
    }

private:
    AxPlugin impl_{};
};

class ProcessClient final : public AxPluginClient {
public:
    bool Open(const std::string& so_path,
              const std::string& init_json,
              std::int32_t device_id,
              std::string* error) override {
        return impl_.Open(so_path, init_json, device_id, error);
    }

    void Close() noexcept override { impl_.Close(); }

    bool Infer(const axvsdk::common::AxImage& image,
               std::vector<axpipeline::ai::Detection>* out,
               std::string* error) override {
        return impl_.Infer(image, out, error);
    }

private:
    AxPluginProcess impl_{};
};

}  // namespace

std::unique_ptr<AxPluginClient> CreatePluginClient(AxPluginIsolationMode mode) {
    switch (mode) {
    case AxPluginIsolationMode::kSubprocess:
        return std::make_unique<ProcessClient>();
    case AxPluginIsolationMode::kInProcess:
    default:
        return std::make_unique<InprocClient>();
    }
}

}  // namespace axpipeline::plugin

