// plugin_pcd_vlm.cpp —— 人车非检测 + VLM 语义描述 插件(demo,面向二次改造)
//
// 三层解耦,各层单独可换:
//   1) 检测/跟踪层  detector_proxy.hpp —— dlopen 现成检测插件(默认 pcd,可换 yolov5/v8 等),
//      检测框照常返回给 OSD/主链路;本插件不重复实现检测。
//   2) 事件层      event_gate.hpp    —— 把每帧检测结果过滤成稀疏「事件」(触发筛选/选帧/
//      节流抖动/去重/硬件JPEG抓拍)。想改业务策略(禁区、越线、停留…)只动这一层。
//   3) VLM 层      vlm_worker.hpp    —— 异步 worker:调 OpenAI 兼容 VLM 拿描述,上报 web
//      事件中心。想换 VLM 后端 / 上报到 MQTT、数据库等只动这一层。
//
// infer() 主链路零阻塞:检测转发 + 事件判定(纯内存)+ 队列投递,VLM/网络全在 worker 线程。
//
// init_json 结构见 README.md 与 pcd_vlm_rtsp.json。
#define AX_PLUGIN_BUILD_DLL 1

#include "ax_plugin/ax_plugin.h"

#include <array>
#include <chrono>
#include <cstddef>
#include <memory>
#include <string>

#include "json.hpp"
#include "common/ax_image.h"

#include "detector_proxy.hpp"
#include "event_gate.hpp"
#include "vlm_worker.hpp"

namespace {
using json = nlohmann::json;

struct Ctx {
    pcdvlm::DetectorProxy detector;
    pcdvlm::EventGate gate;
    pcdvlm::VlmWorker worker;
    bool vlm_enabled{false};
};

double MonoNow() {
    return std::chrono::duration<double>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

// 把插件 ABI 的 image view 包装成 AxImage(零拷贝;事件层抓拍用)
bool WrapImage(const ax_plugin_image_view_t& view, axvsdk::common::AxImage::Ptr* out) {
    if (!out) return false;
    *out = nullptr;
    if (view.width == 0 || view.height == 0 || view.plane_count == 0) return false;
    axvsdk::common::ImageDescriptor desc{};
    switch (view.format) {
    case AX_PLUGIN_PIXEL_FORMAT_NV12:  desc.format = axvsdk::common::PixelFormat::kNv12;  break;
    case AX_PLUGIN_PIXEL_FORMAT_RGB24: desc.format = axvsdk::common::PixelFormat::kRgb24; break;
    case AX_PLUGIN_PIXEL_FORMAT_BGR24: desc.format = axvsdk::common::PixelFormat::kBgr24; break;
    default: return false;
    }
    desc.width = view.width;
    desc.height = view.height;
    for (std::size_t i = 0; i < 3 && i < view.plane_count; ++i) desc.strides[i] = view.strides[i];
    std::array<axvsdk::common::ExternalImagePlane, axvsdk::common::kMaxImagePlanes> planes{};
    for (auto& p : planes) {
        p.virtual_address = nullptr; p.physical_address = 0;
        p.block_id = axvsdk::common::kInvalidPoolId;
    }
    for (std::size_t i = 0; i < view.plane_count && i < axvsdk::common::kMaxImagePlanes; ++i) {
        planes[i].virtual_address = view.virtual_addrs[i];
        planes[i].physical_address = view.physical_addrs[i];
        planes[i].block_id = view.block_ids[i];
    }
    auto img = axvsdk::common::AxImage::WrapExternal(desc, planes);
    if (!img) return false;
    *out = std::move(img);
    return true;
}

void ParseCfg(const json& j, const std::string& stream_name,
              pcdvlm::GateCfg* g, pcdvlm::VlmCfg* v, bool* enabled) {
    const json vj = (j.contains("vlm") && j["vlm"].is_object()) ? j["vlm"] : json::object();
    *enabled = vj.value("enable", true);

    // VLM 层
    v->stream_name   = stream_name;
    v->url           = vj.value("url", v->url);
    v->api_key       = vj.value("api_key", v->api_key);
    v->model         = vj.value("model", v->model);
    v->system_prompt = vj.value("system_prompt", v->system_prompt);
    v->prompt        = vj.value("prompt", v->prompt);
    v->max_tokens    = vj.value("max_tokens", v->max_tokens);
    v->temperature   = vj.value("temperature", v->temperature);
    v->report_url    = vj.value("report_url", v->report_url);

    // 事件层
    g->crop_mode     = vj.value("crop_mode", g->crop_mode);
    g->roi_expand    = vj.value("roi_expand", g->roi_expand);
    g->jpeg_quality  = vj.value("jpeg_quality", g->jpeg_quality);
    g->motion_replay = vj.value("motion_replay", g->motion_replay);
    g->jitter_key    = stream_name;
    if (vj.contains("trigger") && vj["trigger"].is_object()) {
        const auto& t = vj["trigger"]; auto& d = g->trig;
        if (t.contains("classes") && t["classes"].is_array())
            d.classes = t["classes"].get<std::vector<int>>();
        d.min_score            = t.value("min_score", d.min_score);
        d.min_box_h            = t.value("min_box_h", d.min_box_h);
        d.min_track_age        = t.value("min_track_age", d.min_track_age);
        d.reject_border        = t.value("reject_border", d.reject_border);
        d.per_track_once       = t.value("per_track_once", d.per_track_once);
        d.per_track_cooldown_s = t.value("per_track_cooldown_s", d.per_track_cooldown_s);
        d.select_frame         = t.value("select_frame", d.select_frame);
    }
    if (vj.contains("rate") && vj["rate"].is_object()) {
        const auto& r = vj["rate"];
        g->per_stream_interval_s = r.value("per_stream_interval_s", g->per_stream_interval_s);
        v->queue_size            = r.value("queue_size", v->queue_size);
        v->retry_once            = r.value("retry_once", v->retry_once);
    }
}
}  // namespace

extern "C" {

int ax_plugin_get_api_version(void) { return AX_PLUGIN_API_VERSION; }

int ax_plugin_init(const char* init_json, int32_t device_id, ax_plugin_handle_t* out_handle) {
    if (!out_handle) return -1;
    *out_handle = nullptr;

    json j;
    try { j = json::parse(init_json ? init_json : "{}"); }
    catch (...) { return -2; }

    auto ctx = std::make_unique<Ctx>();

    // 1) 检测/跟踪层:用「检测部分」的配置初始化内层插件(剔除本插件私有字段;
    //    强制开跟踪 —— 事件层的选帧/去重需要 track_id)
    const std::string so = j.value("pcd_plugin_path", std::string("libax_plugin_pcd.so"));
    json det = j;
    det.erase("vlm");
    det.erase("pcd_plugin_path");
    det.erase("stream_name");
    det["enable_tracking"] = true;
    const int r = ctx->detector.Open(so, det.dump(), device_id);
    if (r != 0) return r;

    // 2)+3) 事件层 / VLM 层
    const std::string stream = j.value("stream_name", std::string("CH00"));
    pcdvlm::GateCfg gcfg;
    pcdvlm::VlmCfg vcfg;
    ParseCfg(j, stream, &gcfg, &vcfg, &ctx->vlm_enabled);
    if (ctx->vlm_enabled) {
        ctx->gate.Init(gcfg);
        ctx->worker.Init(vcfg);
    }

    *out_handle = reinterpret_cast<ax_plugin_handle_t>(ctx.release());
    return 0;
}

int ax_plugin_infer(ax_plugin_handle_t handle, const ax_plugin_image_view_t* image,
                    ax_plugin_det_result_t* out_result) {
    auto* ctx = reinterpret_cast<Ctx*>(handle);
    if (!ctx || !image || !out_result) return -1;

    // 检测/跟踪(检测框照常给 OSD、主链路)
    const int r = ctx->detector.Infer(image, out_result);

    // 事件层判定 → VLM 层异步处理(主链路零阻塞)
    if (r == 0 && ctx->vlm_enabled && out_result->dets && out_result->det_count > 0) {
        axvsdk::common::AxImage::Ptr frame;
        if (WrapImage(*image, &frame) && frame) {
            pcdvlm::Event ev;
            if (ctx->gate.OnFrame(*frame, out_result->dets, out_result->det_count, MonoNow(), &ev)) {
                ctx->worker.Submit(std::move(ev));
            }
        }
    }
    return r;
}

void ax_plugin_release_result(ax_plugin_handle_t handle, ax_plugin_det_result_t* result) {
    auto* ctx = reinterpret_cast<Ctx*>(handle);
    if (ctx) ctx->detector.Release(result);
}

void ax_plugin_deinit(ax_plugin_handle_t handle) {
    auto* ctx = reinterpret_cast<Ctx*>(handle);
    if (!ctx) return;
    ctx->worker.Stop();
    ctx->detector.Close();
    delete ctx;
}

}  // extern "C"
