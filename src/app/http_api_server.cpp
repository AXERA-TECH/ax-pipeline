#include "app/http_api_server.hpp"

#include <cctype>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <utility>

#include "json.hpp"

// Reuse the single-header HTTP server already vendored by rtsp-sdk.
#include "httplib.h"

namespace axpipeline::app {

namespace {

using json = nlohmann::json;

std::string Trim(const std::string& s) {
    std::size_t b = 0;
    while (b < s.size() && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
    std::size_t e = s.size();
    while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
    return s.substr(b, e - b);
}

bool ParseInt(const std::string& s, int* out) {
    if (!out) return false;
    try {
        *out = std::stoi(s);
        return true;
    } catch (...) {
        return false;
    }
}

bool ParseU32(const std::string& s, std::uint32_t* out) {
    if (!out) return false;
    try {
        const auto v = std::stoll(s);
        if (v < 0) return false;
        *out = static_cast<std::uint32_t>(v);
        return true;
    } catch (...) {
        return false;
    }
}

bool ParseBool(const std::string& s, bool* out) {
    if (!out) return false;
    const auto v = Trim(s);
    if (v == "1" || v == "true" || v == "TRUE" || v == "yes" || v == "YES") {
        *out = true;
        return true;
    }
    if (v == "0" || v == "false" || v == "FALSE" || v == "no" || v == "NO") {
        *out = false;
        return true;
    }
    return false;
}

void ReplyJson(httplib::Response& res, int status, const json& j) {
    res.status = status;
    res.set_header("Cache-Control", "no-store");
    res.set_content(j.dump(), "application/json; charset=utf-8");
}

void ReplyError(httplib::Response& res, int status, const std::string& msg) {
    ReplyJson(res, status, json{{"ok", false}, {"error", msg}});
}

bool AuthOk(const httplib::Request& req, const std::string& token) {
    if (token.empty()) return true;
    if (!req.has_header("Authorization")) return false;
    const std::string auth = req.get_header_value("Authorization");
    const std::string prefix = "Bearer ";
    if (auth.rfind(prefix, 0) != 0) return false;
    const std::string got = auth.substr(prefix.size());
    return got == token;
}

json SnapshotToJson(const PipelineSnapshot& s) {
    json j;
    j["name"] = s.name;
    j["device_id"] = s.device_id;
    j["running"] = s.running;
    j["decoded"] = s.stats.decoded_frames;
    j["submit_fail"] = s.stats.branch_submit_failures;
    j["npu_ok"] = s.npu_ok;
    j["npu_err"] = s.npu_err;
    json outs = json::array();
    for (std::size_t i = 0; i < s.stats.output_stats.size(); ++i) {
        const auto& o = s.stats.output_stats[i];
        outs.push_back(json{
            {"index", i},
            {"submitted", o.submitted_frames},
            {"dropped", o.dropped_frames},
            {"encoded_pkts", o.encoded_packets},
            {"key_pkts", o.key_packets},
            {"queue_depth", o.current_queue_depth},
            {"queue_capacity", o.queue_capacity},
        });
    }
    j["outputs"] = std::move(outs);
    return j;
}

}  // namespace

class HttpApiServer::Impl {
public:
    httplib::Server server;
    HttpServerOptions opt{};
    std::thread th;
};

HttpApiServer::HttpApiServer(PipelineService* service) : service_(service) {}

HttpApiServer::~HttpApiServer() {
    Stop();
}

bool HttpApiServer::Start(const HttpServerOptions& opt, std::string* error) {
    if (!service_) {
        if (error) *error = "pipeline service is null";
        return false;
    }
    if (opt.port <= 0) {
        if (error) *error = "http port must be > 0";
        return false;
    }
    if (running_.load()) return true;

    impl_ = std::make_unique<Impl>();
    impl_->opt = opt;

    auto& svr = impl_->server;
    svr.set_payload_max_length(2 * 1024 * 1024);  // 2MB JSON

    svr.Get("/api/v1/health", [&](const httplib::Request&, httplib::Response& res) {
        ReplyJson(res, 200, json{{"ok", true}});
    });

    svr.Get("/api/v1/pipelines", [&](const httplib::Request& req, httplib::Response& res) {
        if (!AuthOk(req, impl_->opt.bearer_token)) {
            ReplyError(res, 401, "unauthorized");
            return;
        }
        const auto list = service_->ListSnapshots();
        json arr = json::array();
        for (const auto& s : list) arr.push_back(SnapshotToJson(s));
        ReplyJson(res, 200, json{{"ok", true}, {"pipelines", std::move(arr)}});
    });

    svr.Get("/api/v1/pipelines/:name", [&](const httplib::Request& req, httplib::Response& res) {
        if (!AuthOk(req, impl_->opt.bearer_token)) {
            ReplyError(res, 401, "unauthorized");
            return;
        }
        const auto name_it = req.path_params.find("name");
        if (name_it == req.path_params.end()) {
            ReplyError(res, 400, "missing pipeline name");
            return;
        }
        ConfigLoader::PipelineCfg cfg{};
        std::string err;
        if (!service_->GetPipelineConfig(name_it->second, &cfg, &err)) {
            ReplyError(res, 404, err.empty() ? "not found" : err);
            return;
        }

        json out;
        out["name"] = cfg.name;
        out["device_id"] = cfg.device_id;
        out["uri"] = cfg.sdk.input.uri;
        out["realtime_playback"] = cfg.sdk.input.realtime_playback;
        out["loop_playback"] = cfg.sdk.input.loop_playback;
        out["npu_max_fps"] = cfg.npu_max_fps;
        out["log_every_n_frames"] = cfg.log_every_n_frames;

        json npu;
        npu["enable"] = cfg.npu.enable;
        npu["enable_osd"] = cfg.npu.enable_osd;
        npu["enable_tracking"] = cfg.npu.enable_tracking;
        npu["track_buffer"] = cfg.npu.track_buffer;
        npu["ax_plugin_path"] = cfg.npu.ax_plugin_path;
        npu["ax_plugin_isolation"] = cfg.npu.ax_plugin_isolation;
        if (!cfg.npu.ax_plugin_init_json.empty()) {
            try {
                npu["ax_plugin_init_info"] = json::parse(cfg.npu.ax_plugin_init_json);
            } catch (...) {
                npu["ax_plugin_init_info"] = json::object();
            }
        }
        out["npu"] = std::move(npu);

        json outputs = json::array();
        for (const auto& o : cfg.sdk.outputs) {
            json oo;
            oo["codec"] = (o.codec == axvsdk::codec::VideoCodecType::kH265) ? "h265" : "h264";
            oo["width"] = o.width;
            oo["height"] = o.height;
            oo["frame_rate"] = o.frame_rate;
            oo["bitrate_kbps"] = o.bitrate_kbps;
            oo["gop"] = o.gop;
            oo["input_queue_depth"] = o.input_queue_depth;
            oo["overflow_policy"] = (o.overflow_policy == axvsdk::codec::QueueOverflowPolicy::kDropNewest)
                                        ? "drop_newest"
                                    : (o.overflow_policy == axvsdk::codec::QueueOverflowPolicy::kBlock) ? "block"
                                                                                                      : "drop_oldest";
            json uris = json::array();
            for (const auto& u : o.uris) uris.push_back(u);
            oo["uris"] = std::move(uris);
            outputs.push_back(std::move(oo));
        }
        out["outputs"] = std::move(outputs);

        ReplyJson(res, 200, json{{"ok", true}, {"pipeline", std::move(out)}});
    });

    svr.Post("/api/v1/pipelines", [&](const httplib::Request& req, httplib::Response& res) {
        if (!AuthOk(req, impl_->opt.bearer_token)) {
            ReplyError(res, 401, "unauthorized");
            return;
        }
        json body;
        try {
            body = json::parse(req.body);
        } catch (...) {
            ReplyError(res, 400, "invalid json");
            return;
        }
        bool autostart = true;
        if (body.is_object() && body.contains("autostart") && body["autostart"].is_boolean()) {
            autostart = body["autostart"].get<bool>();
        }
        json pj = body;
        if (body.is_object() && body.contains("pipeline")) pj = body["pipeline"];
        ConfigLoader::PipelineCfg cfg{};
        std::string err;
        if (!ConfigLoader::LoadPipelineFromJson(pj, 0, &cfg, &err)) {
            ReplyError(res, 400, err.empty() ? "invalid pipeline config" : err);
            return;
        }
        if (cfg.name.empty()) {
            ReplyError(res, 400, "pipeline.name is required");
            return;
        }
        if (!service_->AddPipeline(cfg, autostart, &err)) {
            ReplyError(res, 400, err.empty() ? "add pipeline failed" : err);
            return;
        }
        ReplyJson(res, 200, json{{"ok", true}, {"name", cfg.name}});
    });

    svr.Delete("/api/v1/pipelines/:name", [&](const httplib::Request& req, httplib::Response& res) {
        if (!AuthOk(req, impl_->opt.bearer_token)) {
            ReplyError(res, 401, "unauthorized");
            return;
        }
        const auto name_it = req.path_params.find("name");
        if (name_it == req.path_params.end()) {
            ReplyError(res, 400, "missing pipeline name");
            return;
        }
        std::string err;
        if (!service_->RemovePipeline(name_it->second, &err)) {
            ReplyError(res, 404, err.empty() ? "remove failed" : err);
            return;
        }
        ReplyJson(res, 200, json{{"ok", true}});
    });

    svr.Post("/api/v1/pipelines/:name/start", [&](const httplib::Request& req, httplib::Response& res) {
        if (!AuthOk(req, impl_->opt.bearer_token)) {
            ReplyError(res, 401, "unauthorized");
            return;
        }
        const auto name_it = req.path_params.find("name");
        if (name_it == req.path_params.end()) {
            ReplyError(res, 400, "missing pipeline name");
            return;
        }
        std::string err;
        if (!service_->StartPipeline(name_it->second, &err)) {
            ReplyError(res, 400, err.empty() ? "start failed" : err);
            return;
        }
        ReplyJson(res, 200, json{{"ok", true}});
    });

    svr.Post("/api/v1/pipelines/:name/stop", [&](const httplib::Request& req, httplib::Response& res) {
        if (!AuthOk(req, impl_->opt.bearer_token)) {
            ReplyError(res, 401, "unauthorized");
            return;
        }
        const auto name_it = req.path_params.find("name");
        if (name_it == req.path_params.end()) {
            ReplyError(res, 400, "missing pipeline name");
            return;
        }
        std::string err;
        if (!service_->StopPipeline(name_it->second, &err)) {
            ReplyError(res, 400, err.empty() ? "stop failed" : err);
            return;
        }
        ReplyJson(res, 200, json{{"ok", true}});
    });

    svr.Put("/api/v1/pipelines/:name", [&](const httplib::Request& req, httplib::Response& res) {
        if (!AuthOk(req, impl_->opt.bearer_token)) {
            ReplyError(res, 401, "unauthorized");
            return;
        }
        const auto name_it = req.path_params.find("name");
        if (name_it == req.path_params.end()) {
            ReplyError(res, 400, "missing pipeline name");
            return;
        }

        json body;
        try {
            body = json::parse(req.body);
        } catch (...) {
            ReplyError(res, 400, "invalid json");
            return;
        }

        bool autostart = true;
        if (body.is_object() && body.contains("autostart") && body["autostart"].is_boolean()) {
            autostart = body["autostart"].get<bool>();
        }
        json pj = body;
        if (body.is_object() && body.contains("pipeline")) pj = body["pipeline"];

        if (pj.is_object() && (!pj.contains("name") || !pj["name"].is_string() || pj["name"].get<std::string>().empty())) {
            pj["name"] = name_it->second;
        }

        ConfigLoader::PipelineCfg cfg{};
        std::string err;
        if (!ConfigLoader::LoadPipelineFromJson(pj, 0, &cfg, &err)) {
            ReplyError(res, 400, err.empty() ? "invalid pipeline config" : err);
            return;
        }
        if (cfg.name != name_it->second) {
            ReplyError(res, 400, "pipeline.name mismatch with URL");
            return;
        }

        if (!service_->ReplacePipeline(cfg, autostart, &err)) {
            ReplyError(res, 400, err.empty() ? "replace failed" : err);
            return;
        }
        ReplyJson(res, 200, json{{"ok", true}});
    });

    svr.Put("/api/v1/pipelines/:name/npu", [&](const httplib::Request& req, httplib::Response& res) {
        if (!AuthOk(req, impl_->opt.bearer_token)) {
            ReplyError(res, 401, "unauthorized");
            return;
        }
        const auto name_it = req.path_params.find("name");
        if (name_it == req.path_params.end()) {
            ReplyError(res, 400, "missing pipeline name");
            return;
        }

        json body;
        try {
            body = json::parse(req.body);
        } catch (...) {
            ReplyError(res, 400, "invalid json");
            return;
        }

        bool autostart = true;
        if (body.is_object() && body.contains("autostart") && body["autostart"].is_boolean()) {
            autostart = body["autostart"].get<bool>();
        }

        ConfigLoader::PipelineCfg current{};
        std::string err;
        if (!service_->GetPipelineConfig(name_it->second, &current, &err)) {
            ReplyError(res, 404, err.empty() ? "not found" : err);
            return;
        }

        json npuj = body;
        if (body.is_object() && body.contains("npu")) npuj = body["npu"];
        if (!npuj.is_object()) {
            ReplyError(res, 400, "npu must be object");
            return;
        }

        // We accept the same fields as config file's npu{}.
        if (npuj.contains("enable")) {
            if (!npuj["enable"].is_boolean()) {
                ReplyError(res, 400, "npu.enable must be boolean");
                return;
            }
            current.npu.enable = npuj["enable"].get<bool>();
        }
        if (npuj.contains("enable_osd")) {
            if (!npuj["enable_osd"].is_boolean()) {
                ReplyError(res, 400, "npu.enable_osd must be boolean");
                return;
            }
            current.npu.enable_osd = npuj["enable_osd"].get<bool>();
        }
        if (npuj.contains("enable_tracking")) {
            if (!npuj["enable_tracking"].is_boolean()) {
                ReplyError(res, 400, "npu.enable_tracking must be boolean");
                return;
            }
            current.npu.enable_tracking = npuj["enable_tracking"].get<bool>();
        }
        if (npuj.contains("track_buffer")) {
            if (!npuj["track_buffer"].is_number_integer()) {
                ReplyError(res, 400, "npu.track_buffer must be integer");
                return;
            }
            current.npu.track_buffer = npuj["track_buffer"].get<std::int32_t>();
        }
        if (npuj.contains("ax_plugin_path")) {
            if (!npuj["ax_plugin_path"].is_string()) {
                ReplyError(res, 400, "npu.ax_plugin_path must be string");
                return;
            }
            current.npu.ax_plugin_path = npuj["ax_plugin_path"].get<std::string>();
        }
        if (npuj.contains("ax_plugin_isolation")) {
            if (!npuj["ax_plugin_isolation"].is_string()) {
                ReplyError(res, 400, "npu.ax_plugin_isolation must be string");
                return;
            }
            current.npu.ax_plugin_isolation = npuj["ax_plugin_isolation"].get<std::string>();
        }
        if (npuj.contains("ax_plugin_init_info")) {
            current.npu.ax_plugin_init_json = npuj["ax_plugin_init_info"].dump();
        }
        if (body.is_object() && body.contains("npu_max_fps") && body["npu_max_fps"].is_number()) {
            current.npu_max_fps = body["npu_max_fps"].get<double>();
        }

        if (!service_->UpdatePipelineNpu(name_it->second, current.npu, current.npu_max_fps, autostart, &err)) {
            ReplyError(res, 400, err.empty() ? "update npu failed" : err);
            return;
        }
        ReplyJson(res, 200, json{{"ok", true}});
    });

    svr.Post("/api/v1/pipelines/:name/outputs", [&](const httplib::Request& req, httplib::Response& res) {
        if (!AuthOk(req, impl_->opt.bearer_token)) {
            ReplyError(res, 401, "unauthorized");
            return;
        }
        const auto name_it = req.path_params.find("name");
        if (name_it == req.path_params.end()) {
            ReplyError(res, 400, "missing pipeline name");
            return;
        }

        json body;
        try {
            body = json::parse(req.body);
        } catch (...) {
            ReplyError(res, 400, "invalid json");
            return;
        }

        json oj = body;
        if (body.is_object() && body.contains("output")) oj = body["output"];

        axvsdk::pipeline::PipelineOutputConfig outcfg{};
        std::string err;
        if (!ConfigLoader::LoadOutputFromJson(oj, &outcfg, &err)) {
            ReplyError(res, 400, err.empty() ? "invalid output config" : err);
            return;
        }

        std::size_t idx = 0;
        if (!service_->AddPipelineOutput(name_it->second, outcfg, &idx, &err)) {
            ReplyError(res, 400, err.empty() ? "add output failed" : err);
            return;
        }
        ReplyJson(res, 200, json{{"ok", true}, {"index", idx}});
    });

    svr.Delete("/api/v1/pipelines/:name/outputs/:index", [&](const httplib::Request& req, httplib::Response& res) {
        if (!AuthOk(req, impl_->opt.bearer_token)) {
            ReplyError(res, 401, "unauthorized");
            return;
        }
        const auto name_it = req.path_params.find("name");
        const auto idx_it = req.path_params.find("index");
        if (name_it == req.path_params.end() || idx_it == req.path_params.end()) {
            ReplyError(res, 400, "missing pipeline name or index");
            return;
        }

        int idx = -1;
        if (!ParseInt(idx_it->second, &idx) || idx < 0) {
            ReplyError(res, 400, "invalid output index");
            return;
        }

        std::string err;
        if (!service_->RemovePipelineOutput(name_it->second, static_cast<std::size_t>(idx), &err)) {
            ReplyError(res, 400, err.empty() ? "remove output failed" : err);
            return;
        }
        ReplyJson(res, 200, json{{"ok", true}});
    });

    svr.Get("/api/v1/pipelines/:name/preview.jpg", [&](const httplib::Request& req, httplib::Response& res) {
        if (!AuthOk(req, impl_->opt.bearer_token)) {
            res.status = 401;
            res.set_content("unauthorized", "text/plain; charset=utf-8");
            return;
        }
        const auto name_it = req.path_params.find("name");
        if (name_it == req.path_params.end()) {
            res.status = 400;
            res.set_content("missing pipeline name", "text/plain; charset=utf-8");
            return;
        }

        PreviewOptions popt{};
        if (req.has_param("quality")) {
            int q = 0;
            if (ParseInt(req.get_param_value("quality"), &q)) popt.quality = q;
        }
        if (req.has_param("max_w")) {
            std::uint32_t v = 0;
            if (ParseU32(req.get_param_value("max_w"), &v)) popt.max_width = v;
        }
        if (req.has_param("max_h")) {
            std::uint32_t v = 0;
            if (ParseU32(req.get_param_value("max_h"), &v)) popt.max_height = v;
        }
        if (req.has_param("with_boxes")) {
            bool b = false;
            if (ParseBool(req.get_param_value("with_boxes"), &b)) popt.with_boxes = b;
        }

        std::vector<std::uint8_t> jpg;
        std::string err;
        if (!service_->GetPreviewJpeg(name_it->second, popt, &jpg, &err)) {
            res.status = 400;
            res.set_content(err.empty() ? "preview failed" : err, "text/plain; charset=utf-8");
            return;
        }
        res.status = 200;
        res.set_header("Cache-Control", "no-store");
        res.set_content(reinterpret_cast<const char*>(jpg.data()), jpg.size(), "image/jpeg");
    });

    impl_->th = std::thread([this]() {
        running_.store(true);
        const auto addr = impl_->opt.bind_addr.empty() ? std::string("127.0.0.1") : impl_->opt.bind_addr;
        const int port = impl_->opt.port;
        std::cerr << "[http] listening on " << addr << ":" << port << "\n";
        impl_->server.listen(addr, port);
        running_.store(false);
    });

    return true;
}

void HttpApiServer::Stop() noexcept {
    if (!impl_) return;
    impl_->server.stop();
    if (impl_->th.joinable()) {
        impl_->th.join();
    }
    impl_.reset();
    running_.store(false);
}

}  // namespace axpipeline::app
