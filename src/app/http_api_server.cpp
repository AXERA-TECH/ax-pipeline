#include "app/http_api_server.hpp"

#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <chrono>
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>
#include <thread>
#include <dirent.h>
#include <dlfcn.h>
#include <unistd.h>
#include <utility>

#include "json.hpp"

// Reuse the single-header HTTP server already vendored by rtsp-sdk.
#include "httplib.h"

#if defined(AXPIPELINE_APP_PLATFORM_AXCL)
#include "axcl.h"       // axclrtSetDevice
#include "axcl_sys.h"   // AXCL_SYS_MemQueryStatus
#endif

namespace axpipeline::app {
extern const unsigned char kWebuiHtml[];
extern const unsigned long kWebuiHtmlLen;
}

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

// ---------- 插件发现:扫 plugins/*/libax_plugin_*.so,取可选的 config schema ----------
json ScanPlugins() {
    json arr = json::array();
    DIR* root = opendir("plugins");
    if (!root) return arr;
    dirent* e;
    while ((e = readdir(root)) != nullptr) {
        if (e->d_name[0] == '.') continue;
        const std::string sub = std::string("plugins/") + e->d_name;
        DIR* d = opendir(sub.c_str());
        if (!d) continue;
        dirent* f;
        while ((f = readdir(d)) != nullptr) {
            const std::string fn = f->d_name;
            if (fn.rfind("libax_plugin_", 0) != 0 || fn.size() < 3 ||
                fn.substr(fn.size() - 3) != ".so") continue;
            const std::string path = sub + "/" + fn;
            json item = {{"path", path},
                         {"name", fn.substr(13, fn.size() - 16)}};  // libax_plugin_<name>.so
            void* dl = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
            if (dl) {
                using SchemaFn = const char* (*)(void);
                if (auto fp = reinterpret_cast<SchemaFn>(dlsym(dl, "ax_plugin_get_config_schema"))) {
                    try {
                        item["schema"] = json::parse(fp());
                    } catch (...) {}
                }
                dlclose(dl);
            }
            arr.push_back(std::move(item));
        }
        closedir(d);
    }
    closedir(root);
    return arr;
}

// ---------- 系统资源采集(CPU / DDR / CMM) ----------
bool ReadFileStr(const char* p, std::string* out) {
    std::ifstream f(p);
    if (!f) return false;
    std::stringstream ss; ss << f.rdbuf();
    *out = ss.str();
    return true;
}
// /proc/self/stat 的 utime+stime 与 /proc/stat 的总 jiffies;两次采样差分出百分比
void CpuSample(std::uint64_t* proc_j, std::uint64_t* total_j, std::uint64_t* idle_j) {
    *proc_j = *total_j = *idle_j = 0;
    std::string st;
    if (ReadFileStr("/proc/self/stat", &st)) {
        const auto rp = st.rfind(')');
        std::istringstream is(st.substr(rp + 2));
        std::string tok; std::uint64_t ut = 0, stime = 0;
        for (int i = 1; is >> tok; ++i) {          // 字段(3) 起
            if (i == 12) ut = std::stoull(tok);    // utime = 全行第14 = ')'后第12
            if (i == 13) { stime = std::stoull(tok); break; }
        }
        *proc_j = ut + stime;
    }
    if (ReadFileStr("/proc/stat", &st)) {
        std::istringstream is(st);
        std::string cpu; is >> cpu;
        std::uint64_t v, i = 0, sum = 0;
        for (int k = 0; is >> v && k < 8; ++k) { sum += v; if (k == 3) i = v; }
        *total_j = sum; *idle_j = i;
    }
}
json SystemStatsJson(PipelineService* service) {
    json j;
    // CPU
    static std::mutex mu;
    static std::uint64_t lp = 0, lt = 0, li = 0;
    std::uint64_t p, t, i;
    CpuSample(&p, &t, &i);
    double proc_pct = 0, sys_pct = 0;
    {
        std::lock_guard<std::mutex> lk(mu);
        if (lt != 0 && t > lt) {
            const double dt = double(t - lt);
            const long nc = sysconf(_SC_NPROCESSORS_ONLN);
            proc_pct = 100.0 * double(p - lp) / dt * double(nc > 0 ? nc : 1);
            sys_pct  = 100.0 * double((t - lt) - (i - li)) / dt;
        }
        lp = p; lt = t; li = i;
    }
    j["cpu"] = {{"proc_pct", proc_pct}, {"sys_pct", sys_pct}};
    // DDR
    std::string ms; long rss_kb = 0, tot_kb = 0, avail_kb = 0;
    if (ReadFileStr("/proc/self/status", &ms)) {
        if (auto pos = ms.find("VmRSS:"); pos != std::string::npos) rss_kb = std::atol(ms.c_str() + pos + 6);
    }
    if (ReadFileStr("/proc/meminfo", &ms)) {
        if (auto pos = ms.find("MemTotal:"); pos != std::string::npos) tot_kb = std::atol(ms.c_str() + pos + 9);
        if (auto pos = ms.find("MemAvailable:"); pos != std::string::npos) avail_kb = std::atol(ms.c_str() + pos + 13);
    }
    j["mem"] = {{"rss_mb", rss_kb / 1024.0}, {"total_mb", tot_kb / 1024.0}, {"avail_mb", avail_kb / 1024.0}};
    // CMM
#if defined(AXPIPELINE_APP_PLATFORM_AXCL)
    {
        int dev = 0;
        if (service) {
            const auto snaps = service->ListSnapshots();
            if (!snaps.empty() && snaps.front().device_id >= 0) dev = snaps.front().device_id;
        }
        // http 线程默认没有 axcl context;device_id 是索引,须经 GetDeviceList 转 runtime id,
        // 先 SetDevice 再 CreateContext(create 即绑定本线程)。
        static thread_local axclrtContext s_ctx = nullptr;
        static thread_local int cur_dev = -1;
        if (cur_dev != dev) {
            axclrtDeviceList lst{};
            if (axclrtGetDeviceList(&lst) == 0 && dev < (int)lst.num) {
                const int rid = lst.devices[dev];
                if (s_ctx) { axclrtDestroyContext(s_ctx); s_ctx = nullptr; }
                if (axclrtSetDevice(rid) == 0 && axclrtCreateContext(&s_ctx, rid) == 0)
                    cur_dev = dev;
            }
        }
        AX_CMM_STATUS_T st{};
        if (AXCL_SYS_MemQueryStatus(&st) == 0 && st.TotalSize > 0) {
            // AX_SYS 惯例单位 KB
            j["cmm"] = {{"total_mb", st.TotalSize / 1024.0},
                        {"used_mb", (st.TotalSize - st.RemainSize) / 1024.0},
                        {"block_cnt", st.BlockCnt}, {"device_id", dev}};
        }
    }
#else
    {
        // 板端:/proc/ax_proc/mem_cmm_info 提供 "total size:xxxKB ... remain=xxxKB"
        std::string cm;
        if (ReadFileStr("/proc/ax_proc/mem_cmm_info", &cm)) {
            long tot = 0, rem = 0;
            if (auto pos = cm.find("total size="); pos != std::string::npos) tot = std::atol(cm.c_str() + pos + 11);
            if (auto pos = cm.find("remain="); pos != std::string::npos) rem = std::atol(cm.c_str() + pos + 7);
            if (tot > 0)
                j["cmm"] = {{"total_mb", tot / 1024.0}, {"used_mb", (tot - rem) / 1024.0}, {"device_id", 0}};
        }
    }
#endif
    static const auto t0 = std::chrono::steady_clock::now();
    j["uptime_s"] = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
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

    // 内嵌控制台:GET / 直接吐编进二进制的页面;--http_webroot 指定目录时磁盘优先(方便改UI)
    svr.Get("/", [this](const httplib::Request&, httplib::Response& res) {
        if (!impl_->opt.webroot.empty()) {
            std::ifstream f(impl_->opt.webroot + "/index.html", std::ios::binary);
            if (f) {
                std::stringstream ss; ss << f.rdbuf();
                res.set_content(ss.str(), "text/html; charset=utf-8");
                return;
            }
        }
        res.set_content(reinterpret_cast<const char*>(kWebuiHtml), kWebuiHtmlLen,
                        "text/html; charset=utf-8");
    });

    svr.Get("/api/v1/system", [this](const httplib::Request&, httplib::Response& res) {
        ReplyJson(res, 200, SystemStatsJson(service_));
    });

    // 插件清单 + 各自的配置 schema(首次扫描后缓存;控制台"新建 Pipeline"用它生成表单)
    svr.Get("/api/v1/plugins", [](const httplib::Request&, httplib::Response& res) {
        static std::mutex mu;
        static json cache;
        std::lock_guard<std::mutex> lk(mu);
        if (cache.is_null()) cache = ScanPlugins();
        ReplyJson(res, 200, json{{"ok", true}, {"plugins", cache}});
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
        npu["kalman_smooth"] = cfg.npu.kalman_smooth;
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
        if (cfg.device_id < 0 && impl_->opt.default_device_id >= 0) {
            cfg.device_id = impl_->opt.default_device_id;
            cfg.sdk.device_id = impl_->opt.default_device_id;
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
        if (cfg.device_id < 0 && impl_->opt.default_device_id >= 0) {
            cfg.device_id = impl_->opt.default_device_id;
            cfg.sdk.device_id = impl_->opt.default_device_id;
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
        if (npuj.contains("kalman_smooth")) {
            if (!npuj["kalman_smooth"].is_boolean()) {
                ReplyError(res, 400, "npu.kalman_smooth must be boolean");
                return;
            }
            current.npu.kalman_smooth = npuj["kalman_smooth"].get<bool>();
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

    // MJPEG 直播:multipart 连续推带检测框的 JPEG,客户端断开即停止编码(零常驻开销)。
    // 列表页用 4s 快照,点开详情用本端点高分辨率直播。
    svr.Get("/api/v1/pipelines/:name/stream.mjpeg", [&](const httplib::Request& req, httplib::Response& res) {
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
        const std::string name = name_it->second;
        PreviewOptions popt{};
        popt.quality = 75;
        popt.max_width = 1280;
        popt.max_height = 1280;
        popt.with_boxes = true;
        int fps = 10;
        if (req.has_param("fps")) { int v = 0; if (ParseInt(req.get_param_value("fps"), &v) && v >= 1 && v <= 30) fps = v; }
        if (req.has_param("quality")) { int q = 0; if (ParseInt(req.get_param_value("quality"), &q)) popt.quality = q; }
        if (req.has_param("max_w")) { std::uint32_t v = 0; if (ParseU32(req.get_param_value("max_w"), &v)) { popt.max_width = v; popt.max_height = v; } }
        if (req.has_param("with_boxes")) { bool b = true; if (ParseBool(req.get_param_value("with_boxes"), &b)) popt.with_boxes = b; }
        auto* service = service_;
        const int interval_ms = 1000 / fps;
        res.set_chunked_content_provider(
            "multipart/x-mixed-replace; boundary=axframe",
            [service, name, popt, interval_ms](std::size_t, httplib::DataSink& sink) {
                std::vector<std::uint8_t> jpg;
                std::string err;
                while (true) {
                    jpg.clear();
                    if (!service->GetPreviewJpeg(name, popt, &jpg, &err) || jpg.empty())
                        return false;  // pipeline 没了/无帧 -> 结束流
                    std::string head = "--axframe\r\nContent-Type: image/jpeg\r\nContent-Length: " +
                                       std::to_string(jpg.size()) + "\r\n\r\n";
                    if (!sink.write(head.data(), head.size())) return false;   // 客户端断开
                    if (!sink.write(reinterpret_cast<const char*>(jpg.data()), jpg.size())) return false;
                    if (!sink.write("\r\n", 2)) return false;
                    std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms));
                }
            });
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
