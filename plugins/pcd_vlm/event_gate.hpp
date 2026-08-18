// event_gate.hpp —— 事件层
//
// 职责:把"每帧的检测+跟踪结果"过滤成稀疏的「事件」。一个事件 = 值得给 VLM 看一眼的时刻,
// 附带抓拍图(硬件 JPEG)与元数据。本层不联网、不认识 VLM,只做策略:
//   - 触发筛选:类别 / 置信度 / 框高 / 跟踪稳定度 / 贴边剔除
//   - 去重:每个 track_id 只触发一次(可配冷却复看)
//   - 选帧:候选里挑 框面积×置信度 最高的目标
//   - 节流:每路最小间隔 + ±20% 随机抖动(防与循环视频时长共振)
//   - 抓拍:整帧或抠 ROI(可外扩),ax-video-sdk 硬件裁剪/编码,支持 device 帧
//   - 内容去重:连续两次抓拍完全相同则丢弃
#pragma once

#include <algorithm>
#include <chrono>
#include <memory>
#include <cstdint>
#include <deque>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include "ax_plugin/ax_plugin.h"
#include "common/ax_image.h"
#include "common/ax_image_processor.h"
#include "codec/ax_jpeg_codec.h"

namespace pcdvlm {

struct TriggerCfg {
    std::vector<int> classes;       // 空 = 全部类别
    float min_score{0.4F};
    int   min_box_h{64};            // 像素,太小的目标看不清
    int   min_track_age{8};         // 连续跟踪 N 帧才算稳定目标
    bool  reject_border{true};      // 贴边(残缺)不触发
    bool  per_track_once{true};     // 每个 track_id 只触发一次
    float per_track_cooldown_s{0};  // >0 则每个 track 每 N 秒可复看一次
    std::string select_frame{"best"};  // best(框面积×置信度) | now(仅置信度)
};

struct GateCfg {
    TriggerCfg trig;
    float per_stream_interval_s{30.F};  // 每路最小触发间隔(主旋钮)
    std::string crop_mode{"frame"};     // frame=整帧 | roi=抠检测框
    float roi_expand{0.0F};             // roi 模式按框比例四周外扩
    int   jpeg_quality{80};
    bool  motion_replay{true};          // 事件附带 ±2s 共5帧轮播(网页里"会动");关=只有主帧
    std::string jitter_key;             // 抖动随机序列的种子盐(用路名,避免各路同步)
};

// 事件:一次值得 VLM 解析的抓拍
struct Event {
    std::int64_t track_id{-1};
    int   cls{-1};
    float score{0};
    int   box[4]{0, 0, 0, 0};
    double ts{0};              // 墙钟 epoch 秒(主帧时刻)
    std::string image_b64;     // 主帧 JPEG base64(VLM 用;不含 data: 前缀)
    std::vector<std::string> replay;  // 轮播帧(约 T-2s..T+2s 每秒1帧,含主帧;motion_replay 开时为5帧)
};

class EventGate {
public:
    void Init(const GateCfg& c) {
        cfg_ = c;
        for (char ch : cfg_.jitter_key) jseed_ = jseed_ * 131U + (std::uint32_t)ch;
        ip_ = axvsdk::common::CreateImageProcessor();
    }

    // 每帧调用。now 为单调秒(仅用于节流/寿命);返回 true 时 out 填好一个事件。
    bool OnFrame(const axvsdk::common::AxImage& frame,
                 const ax_plugin_det_t* dets, std::size_t n, double now, Event* out) {
        if (!ip_ || !out) return false;
        const int fw = (int)frame.width(), fh = (int)frame.height();

        // 轮播预缓存:每秒存一帧整帧 JPEG,只留最近 3 张(事件的"前2秒"只能来自这里——
        // 触发时刻之前的帧早已流走,必须持续缓存)
        if (cfg_.motion_replay && now - last_cache_ >= 1.0) {
            std::string jpg = axvsdk::codec::EncodeJpegToBase64(frame, {(std::uint32_t)cfg_.jpeg_quality});
            if (!jpg.empty()) {
                precache_.push_back(std::move(jpg));
                while (precache_.size() > 3) precache_.pop_front();
                last_cache_ = now;
            }
        }

        // 有攒帧中的事件:每隔1秒补一帧"事件后"画面,凑满即产出(延迟约2s,VLM 本来就要3s)
        if (pending_) {
            for (std::size_t i = 0; i < n; i++) Touch(dets[i].track_id, now);
            if (now >= pend_next_) {
                std::string jpg = axvsdk::codec::EncodeJpegToBase64(frame, {(std::uint32_t)cfg_.jpeg_quality});
                if (!jpg.empty()) {
                    pend_.replay.push_back(std::move(jpg));
                    pend_next_ = now + 1.0;
                }
                if ((int)pend_.replay.size() >= 5) {
                    *out = std::move(pend_);
                    pend_ = {};
                    pending_ = false;
                    return true;
                }
            }
            return false;
        }

        // 过期 track 状态清理(防长时间运行内存膨胀)
        for (auto it = tracks_.begin(); it != tracks_.end();)
            it = (now - it->second.last_seen > 10.0) ? tracks_.erase(it) : std::next(it);

        // 路级节流:未到点只更新跟踪寿命
        if (now - last_sent_ < cur_gap_) {
            for (std::size_t i = 0; i < n; i++) Touch(dets[i].track_id, now);
            return false;
        }

        // 选帧:通过触发条件的目标里,挑得分最高的
        const ax_plugin_det_t* best = nullptr;
        float best_score = -1;
        for (std::size_t i = 0; i < n; i++) {
            const auto& d = dets[i];
            auto& st = Touch(d.track_id, now);
            if (!Pass(fw, fh, d, st, now)) continue;
            const float s = (cfg_.trig.select_frame == "now") ? d.score : d.score * (d.y1 - d.y0);
            if (s > best_score) { best_score = s; best = &d; }
        }
        if (!best) return false;

        if (!Snapshot(frame, *best, &out->image_b64)) return false;

        // 内容去重:与上次抓拍完全相同(短视频循环的唯一触发窗口)则丢弃
        const std::size_t h = std::hash<std::string>{}(out->image_b64);
        if (h == last_img_hash_) return false;
        last_img_hash_ = h;

        Event ev;
        ev.image_b64 = std::move(out->image_b64);
        ev.track_id = best->track_id;
        ev.cls = best->class_id;
        ev.score = best->score;
        ev.box[0] = (int)best->x0; ev.box[1] = (int)best->y0;
        ev.box[2] = (int)best->x1; ev.box[3] = (int)best->y1;
        ev.ts = std::chrono::duration<double>(
            std::chrono::system_clock::now().time_since_epoch()).count();

        last_sent_ = now;
        tracks_[best->track_id].last_sent = now;
        tracks_[best->track_id].ever_sent = true;
        NextGap();

        if (!cfg_.motion_replay) {
            ev.replay.push_back(ev.image_b64);
            *out = std::move(ev);
            return true;
        }
        // 轮播模式:前2帧取自预缓存,加上主帧;转入攒帧,等 T+1s / T+2s 再补2帧后产出
        for (std::size_t i = precache_.size() >= 2 ? precache_.size() - 2 : 0; i < precache_.size(); ++i)
            ev.replay.push_back(precache_[i]);
        ev.replay.push_back(ev.image_b64);
        while ((int)ev.replay.size() > 3) ev.replay.erase(ev.replay.begin());
        pend_ = std::move(ev);
        pend_next_ = now + 1.0;
        pending_ = true;
        return false;
    }

private:
    struct TrackState {
        double first_seen{0}, last_seen{0}, last_sent{0};
        int age{0};
        bool ever_sent{false};
    };

    TrackState& Touch(std::int64_t tid, double now) {
        auto& s = tracks_[tid];
        if (s.first_seen == 0) s.first_seen = now;
        s.age++;
        s.last_seen = now;
        return s;
    }

    bool Pass(int fw, int fh, const ax_plugin_det_t& d, const TrackState& st, double now) const {
        const auto& t = cfg_.trig;
        if (!t.classes.empty() &&
            std::find(t.classes.begin(), t.classes.end(), d.class_id) == t.classes.end()) return false;
        if (d.score < t.min_score) return false;
        if ((d.y1 - d.y0) < (float)t.min_box_h) return false;
        if (st.age < t.min_track_age) return false;
        if (t.reject_border) {
            const float m = 2.F;
            if (d.x0 <= m || d.y0 <= m || d.x1 >= fw - m || d.y1 >= fh - m) return false;
        }
        if (t.per_track_once && st.ever_sent && t.per_track_cooldown_s <= 0) return false;
        if (st.ever_sent && t.per_track_cooldown_s > 0 && now - st.last_sent < t.per_track_cooldown_s) return false;
        return true;
    }

    // 抓拍(硬件 JPEG,支持 device 帧):整帧,或抠 ROI(可外扩)
    bool Snapshot(const axvsdk::common::AxImage& frame, const ax_plugin_det_t& d, std::string* out) const {
        if (cfg_.crop_mode == "frame") {
            *out = axvsdk::codec::EncodeJpegToBase64(frame, {(std::uint32_t)cfg_.jpeg_quality});
            return !out->empty();
        }
        const float ex = std::max(0.0F, cfg_.roi_expand);
        const float bw = d.x1 - d.x0, bh = d.y1 - d.y0;
        int x = std::max(0, (int)(d.x0 - bw * ex)), y = std::max(0, (int)(d.y0 - bh * ex));
        int w = (int)(d.x1 + bw * ex) - x, h = (int)(d.y1 + bh * ex) - y;
        if (x + w > (int)frame.width())  w = (int)frame.width() - x;
        if (y + h > (int)frame.height()) h = (int)frame.height() - y;
        if (w <= 1 || h <= 1) return false;
        axvsdk::common::ImageProcessRequest req;
        req.enable_crop = true;
        req.crop = {x, y, (std::uint32_t)w, (std::uint32_t)h};
        req.output_image.format = axvsdk::common::PixelFormat::kRgb24;
        req.output_image.width = (std::uint32_t)w;
        req.output_image.height = (std::uint32_t)h;
        auto roi = ip_->Process(frame, req);
        if (!roi) return false;
        *out = axvsdk::codec::EncodeJpegToBase64(*roi, {(std::uint32_t)cfg_.jpeg_quality});
        return !out->empty();
    }

    // 下次间隔加 ±20% 抖动:避免与循环播放的视频时长整除共振(否则每次都抓到同一帧)
    void NextGap() {
        jseed_ = jseed_ * 1103515245U + 12345U;
        cur_gap_ = cfg_.per_stream_interval_s *
                   (0.8F + 0.4F * (float)((jseed_ >> 16) & 0x7FFF) / 32767.0F);
    }

    GateCfg cfg_;
    std::unique_ptr<axvsdk::common::ImageProcessor> ip_;
    std::unordered_map<std::int64_t, TrackState> tracks_;
    double last_sent_{0};
    double cur_gap_{0};        // 首个事件立即可发,其后带抖动
    std::uint32_t jseed_{20260818U};
    std::size_t last_img_hash_{0};
    // 轮播帧状态
    std::deque<std::string> precache_;  // 最近3秒、每秒1帧的整帧JPEG(b64)
    double last_cache_{0};
    bool pending_{false};               // 事件已触发,正在攒"事件后"帧
    Event pend_;
    double pend_next_{0};
};

}  // namespace pcdvlm
