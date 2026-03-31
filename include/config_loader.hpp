#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "json.hpp"

#include "common/ax_image.h"
#include "common/ax_image_processor.h"
#include "common/ax_system.h"
#include "pipeline/ax_pipeline.h"

namespace axpipeline {

class ConfigLoader {
public:
    struct PipelineCfg {
        std::string name;
        std::int32_t device_id{-1};
        axvsdk::pipeline::PipelineConfig sdk{};
        // Max NPU processing FPS:
        // - > 0: limit NPU processing rate (best-effort).
        // - 0 or -1: no limit (disable limiter).
        double npu_max_fps{0.0};
        struct NpuCfg {
            bool enable{false};
            bool enable_osd{true};
            // Enable ByteTrack object tracking on top of detector outputs.
            // When enabled, OSD will be drawn from tracked boxes (stable IDs, per-ID colors).
            bool enable_tracking{true};
            // Track buffer in frames (roughly: how long a lost track is kept before removal).
            std::int32_t track_buffer{30};
            // Plugin .so path.
            std::string ax_plugin_path;
            // Plugin isolation mode:
            // - "inproc": dlopen and run in current process (fastest, but plugin crash kills pipeline)
            // - "process": run plugin in a subprocess (crash-isolated; may add copies/overhead)
            std::string ax_plugin_isolation{"inproc"};
            // Plugin init JSON string (the value of npu.ax_plugin_init_info dumped to a string).
            std::string ax_plugin_init_json;
        } npu{};
        std::uint32_t log_every_n_frames{30};
    };

    struct AppCfg {
        axvsdk::common::SystemOptions system{};
        std::vector<PipelineCfg> pipelines;
    };

    static bool LoadFromFile(const std::string& path, AppCfg* out, std::string* error) {
        if (out == nullptr) {
            if (error) *error = "config output is null";
            return false;
        }

        std::string file_err;
        const auto text = ReadFileToString(path, &file_err);
        if (text.empty()) {
            if (error) *error = file_err.empty() ? "empty config" : file_err;
            return false;
        }

        json j;
        try {
            j = json::parse(text);
        } catch (const std::exception& e) {
            if (error) *error = std::string("json parse failed: ") + e.what();
            return false;
        }
        if (!j.is_object()) {
            if (error) *error = "config root must be object";
            return false;
        }

        AppCfg cfg{};
        const std::filesystem::path base_dir = std::filesystem::path(path).parent_path();
        if (j.contains("system")) {
            const auto& s = j["system"];
            if (!s.is_object()) return false;
            if (!GetOptI32(s, "device_id", &cfg.system.device_id)) return false;
            if (!GetOptBool(s, "enable_vdec", &cfg.system.enable_vdec)) return false;
            if (!GetOptBool(s, "enable_venc", &cfg.system.enable_venc)) return false;
            if (!GetOptBool(s, "enable_ivps", &cfg.system.enable_ivps)) return false;
        }

        if (!j.contains("pipelines") || !j["pipelines"].is_array() || j["pipelines"].empty()) {
            if (error) *error = "missing pipelines[]";
            return false;
        }

        const auto& arr = j["pipelines"];
        cfg.pipelines.clear();
        cfg.pipelines.reserve(arr.size());
        for (std::size_t i = 0; i < arr.size(); ++i) {
            PipelineCfg p{};
            if (!ParsePipeline(arr[i], i, &p)) {
                if (error) *error = "invalid pipelines[" + std::to_string(i) + "]";
                return false;
            }
            ResolvePathsRelativeToBase(base_dir, &p);
            cfg.pipelines.push_back(std::move(p));
        }

        *out = std::move(cfg);
        return true;
    }

private:
    using json = nlohmann::json;

    static void ResolvePathsRelativeToBase(const std::filesystem::path& base_dir, PipelineCfg* cfg) {
        if (cfg == nullptr) {
            return;
        }

        auto resolve = [&](std::string* p) {
            if (p == nullptr || p->empty()) return;
            std::filesystem::path pp(*p);
            if (pp.is_relative() && !base_dir.empty()) {
                *p = (base_dir / pp).lexically_normal().string();
            }
        };

        resolve(&cfg->npu.ax_plugin_path);
        if (!cfg->npu.ax_plugin_init_json.empty() && !base_dir.empty()) {
            try {
                auto j = json::parse(cfg->npu.ax_plugin_init_json);
                if (j.is_object() && j.contains("model_path") && j["model_path"].is_string()) {
                    std::string mp = j["model_path"].get<std::string>();
                    std::filesystem::path pp(mp);
                    if (pp.is_relative()) {
                        j["model_path"] = (base_dir / pp).lexically_normal().string();
                        cfg->npu.ax_plugin_init_json = j.dump();
                    }
                }
            } catch (...) {
            }
        }

        // Input URI: resolve only for local-file inputs.
        if (cfg->sdk.input.uri.rfind("rtsp://", 0) != 0 && cfg->sdk.input.uri.rfind("rtsps://", 0) != 0) {
            resolve(&cfg->sdk.input.uri);
        }

        // Output URIs: resolve file targets; keep RTSP URIs unchanged.
        for (auto& u : cfg->sdk.outputs) {
            for (auto& uri : u.uris) {
                if (uri.rfind("rtsp://", 0) == 0 || uri.rfind("rtsps://", 0) == 0) continue;
                resolve(&uri);
            }
        }
    }

    static std::string ReadFileToString(const std::string& path, std::string* error) {
        std::ifstream file(path, std::ios::binary);
        if (!file) {
            if (error) *error = "failed to open file: " + path;
            return {};
        }
        std::ostringstream oss;
        oss << file.rdbuf();
        return oss.str();
    }

    static bool GetOptBool(const json& j, const char* key, bool* out) {
        if (!j.contains(key)) return true;
        if (!j[key].is_boolean()) return false;
        *out = j[key].get<bool>();
        return true;
    }

    static bool GetOptI32(const json& j, const char* key, std::int32_t* out) {
        if (!j.contains(key)) return true;
        if (!j[key].is_number_integer()) return false;
        *out = j[key].get<std::int32_t>();
        return true;
    }

    static bool GetOptU32(const json& j, const char* key, std::uint32_t* out) {
        if (!j.contains(key)) return true;
        if (!j[key].is_number_unsigned() && !j[key].is_number_integer()) return false;
        const auto v = j[key].get<std::int64_t>();
        if (v < 0) return false;
        *out = static_cast<std::uint32_t>(v);
        return true;
    }

    static bool GetOptSizeT(const json& j, const char* key, std::size_t* out) {
        if (!j.contains(key)) return true;
        if (!j[key].is_number_unsigned() && !j[key].is_number_integer()) return false;
        const auto v = j[key].get<std::int64_t>();
        if (v < 0) return false;
        *out = static_cast<std::size_t>(v);
        return true;
    }

    static bool GetOptDouble(const json& j, const char* key, double* out) {
        if (!j.contains(key)) return true;
        if (!j[key].is_number()) return false;
        *out = j[key].get<double>();
        return true;
    }

    static bool GetOptString(const json& j, const char* key, std::string* out) {
        if (!j.contains(key)) return true;
        if (!j[key].is_string()) return false;
        *out = j[key].get<std::string>();
        return true;
    }

    static axvsdk::common::PixelFormat ParsePixelFormat(const std::string& s) {
        if (s == "nv12" || s == "NV12") return axvsdk::common::PixelFormat::kNv12;
        if (s == "rgb" || s == "RGB" || s == "rgb24" || s == "RGB24") return axvsdk::common::PixelFormat::kRgb24;
        if (s == "bgr" || s == "BGR" || s == "bgr24" || s == "BGR24") return axvsdk::common::PixelFormat::kBgr24;
        return axvsdk::common::PixelFormat::kUnknown;
    }

    static axvsdk::codec::VideoCodecType ParseVideoCodec(const std::string& s) {
        if (s == "h264" || s == "H264") return axvsdk::codec::VideoCodecType::kH264;
        if (s == "h265" || s == "H265" || s == "hevc" || s == "HEVC") return axvsdk::codec::VideoCodecType::kH265;
        return axvsdk::codec::VideoCodecType::kUnknown;
    }

    static axvsdk::codec::QueueOverflowPolicy ParseOverflowPolicy(const std::string& s) {
        if (s == "drop_newest") return axvsdk::codec::QueueOverflowPolicy::kDropNewest;
        if (s == "block") return axvsdk::codec::QueueOverflowPolicy::kBlock;
        return axvsdk::codec::QueueOverflowPolicy::kDropOldest;
    }

    static axvsdk::common::ResizeMode ParseResizeMode(const std::string& s) {
        if (s == "keep_aspect" || s == "keep_aspect_ratio") return axvsdk::common::ResizeMode::kKeepAspectRatio;
        return axvsdk::common::ResizeMode::kStretch;
    }

    static axvsdk::common::ResizeAlign ParseResizeAlign(const std::string& s) {
        if (s == "start") return axvsdk::common::ResizeAlign::kStart;
        if (s == "end") return axvsdk::common::ResizeAlign::kEnd;
        return axvsdk::common::ResizeAlign::kCenter;
    }

    static bool ParseResizeOptions(const json& j, axvsdk::common::ResizeOptions* out) {
        if (!out || !j.is_object()) return false;
        std::string mode;
        if (!GetOptString(j, "mode", &mode)) return false;
        if (!mode.empty()) out->mode = ParseResizeMode(mode);

        std::string h;
        if (!GetOptString(j, "horizontal_align", &h)) return false;
        if (!h.empty()) out->horizontal_align = ParseResizeAlign(h);

        std::string v;
        if (!GetOptString(j, "vertical_align", &v)) return false;
        if (!v.empty()) out->vertical_align = ParseResizeAlign(v);

        return GetOptU32(j, "background_color", &out->background_color);
    }

    static bool ParsePipeline(const json& j, std::size_t index, PipelineCfg* out) {
        if (!out || !j.is_object()) return false;

        if (!GetOptString(j, "name", &out->name)) return false;
        if (out->name.empty()) out->name = "pipeline_" + std::to_string(index);

        if (!GetOptI32(j, "device_id", &out->device_id)) return false;

        std::string uri;
        if (!GetOptString(j, "uri", &uri) || uri.empty()) return false;

        bool realtime = true;
        bool loop = false;
        if (!GetOptBool(j, "realtime_playback", &realtime)) return false;
        if (!GetOptBool(j, "loop_playback", &loop)) return false;

        if (!GetOptDouble(j, "npu_max_fps", &out->npu_max_fps)) return false;

        if (!GetOptU32(j, "log_every_n_frames", &out->log_every_n_frames)) return false;

        if (j.contains("npu")) {
            const auto& n = j["npu"];
            if (!n.is_object()) return false;
            if (!GetOptBool(n, "enable", &out->npu.enable)) return false;
            if (!GetOptBool(n, "enable_osd", &out->npu.enable_osd)) return false;
            if (!GetOptBool(n, "enable_tracking", &out->npu.enable_tracking)) return false;
            if (!GetOptI32(n, "track_buffer", &out->npu.track_buffer)) return false;
            if (!GetOptString(n, "ax_plugin_path", &out->npu.ax_plugin_path)) return false;
            if (!GetOptString(n, "ax_plugin_isolation", &out->npu.ax_plugin_isolation)) return false;
            if (n.contains("ax_plugin_init_info")) {
                const auto& init = n["ax_plugin_init_info"];
                if (!init.is_object()) return false;
                out->npu.ax_plugin_init_json = init.dump();
            }
        }

        out->sdk.device_id = out->device_id;
        out->sdk.input.uri = uri;
        out->sdk.input.realtime_playback = realtime;
        out->sdk.input.loop_playback = loop;

        // Default: follow decoded frame (usually NV12) and original resolution.
        out->sdk.frame_output.output_image = {};
        out->sdk.frame_output.resize = {};

        if (j.contains("frame_output")) {
            const auto& fo = j["frame_output"];
            if (!fo.is_object()) return false;
            std::string fmt;
            if (!GetOptString(fo, "format", &fmt)) return false;
            if (!fmt.empty()) {
                const auto pf = ParsePixelFormat(fmt);
                if (pf == axvsdk::common::PixelFormat::kUnknown) return false;
                out->sdk.frame_output.output_image.format = pf;
            }
            std::uint32_t w = out->sdk.frame_output.output_image.width;
            std::uint32_t h = out->sdk.frame_output.output_image.height;
            if (!GetOptU32(fo, "width", &w) || !GetOptU32(fo, "height", &h)) return false;
            out->sdk.frame_output.output_image.width = w;
            out->sdk.frame_output.output_image.height = h;
            if (fo.contains("resize")) {
                if (!ParseResizeOptions(fo["resize"], &out->sdk.frame_output.resize)) return false;
            }
        }

        // frame_output is always honored. When NPU input differs from the decoder source space,
        // ax-pipeline should map detections back to source coordinates before OSD/tracking.

        if (!j.contains("outputs") || !j["outputs"].is_array() || j["outputs"].empty()) return false;
        out->sdk.outputs.clear();
        for (const auto& o : j["outputs"]) {
            if (!o.is_object()) return false;
            axvsdk::pipeline::PipelineOutputConfig oc{};

            std::string codec;
            if (!GetOptString(o, "codec", &codec)) return false;
            if (!codec.empty()) {
                oc.codec = ParseVideoCodec(codec);
                if (oc.codec == axvsdk::codec::VideoCodecType::kUnknown) return false;
            }

            if (!GetOptU32(o, "width", &oc.width) ||
                !GetOptU32(o, "height", &oc.height) ||
                !GetOptDouble(o, "frame_rate", &oc.frame_rate) ||
                !GetOptU32(o, "bitrate_kbps", &oc.bitrate_kbps) ||
                !GetOptU32(o, "gop", &oc.gop) ||
                !GetOptSizeT(o, "input_queue_depth", &oc.input_queue_depth)) {
                return false;
            }

            std::string overflow;
            if (!GetOptString(o, "overflow_policy", &overflow)) return false;
            if (!overflow.empty()) oc.overflow_policy = ParseOverflowPolicy(overflow);

            if (o.contains("resize")) {
                if (!ParseResizeOptions(o["resize"], &oc.resize)) return false;
            }

            if (o.contains("uris")) {
                if (!o["uris"].is_array()) return false;
                for (const auto& u : o["uris"]) {
                    if (!u.is_string()) return false;
                    oc.uris.push_back(u.get<std::string>());
                }
            }

            out->sdk.outputs.push_back(std::move(oc));
        }

        return true;
    }
};

}  // namespace axpipeline
