// vlm_worker.hpp —— VLM 层
//
// 职责:接收事件层产出的 Event,在独立 worker 线程里:
//   1) 调 OpenAI 兼容的 VLM 服务(ax-llm / vLLM / 云端 API)生成一句描述
//   2) 把「描述 + 抓拍 + 元数据」POST 给 web 事件中心 /ingest
// 队列有限(满即丢最旧),绝不阻塞检测主链路;VLM 失败可选重试一次。
// 本层不认识检测/跟踪,只消费 Event。
#pragma once

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <condition_variable>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <mutex>
#include <string>
#include <thread>

#include "json.hpp"
#include "event_gate.hpp"   // Event

namespace pcdvlm {

struct VlmCfg {
    std::string stream_name{"CH00"};
    // OpenAI 兼容端点
    std::string url{"http://127.0.0.1:8013"};
    std::string api_key;            // 云端/vLLM 需要时填;ax-llm 本地留空
    std::string model;              // 建议必填(ax-llm serve 校验模型名);留空回退 "vlm"
    std::string system_prompt{"you are a helpful assistant."};
    std::string prompt{"用简体中文描述这张画面。"};
    int   max_tokens{80};
    float temperature{0.7F};
    // web 事件中心(留空 = 只调 VLM 不上报)
    std::string report_url{"http://127.0.0.1:8900/ingest"};
    int  queue_size{4};
    bool retry_once{false};
};

// 极简 HTTP/1.0 POST(Connection: close)。仅支持 http://host[:port]/path。
inline bool HttpPostJson(const std::string& url, const std::string& body,
                         const std::string& api_key, std::string* resp, int timeout_s = 120) {
    if (url.rfind("http://", 0) != 0) return false;
    std::string rest = url.substr(7), host, path = "/";
    int port = 80;
    const std::size_t sl = rest.find('/');
    std::string hp = (sl == std::string::npos) ? rest : rest.substr(0, sl);
    if (sl != std::string::npos) path = rest.substr(sl);
    const std::size_t co = hp.find(':');
    if (co == std::string::npos) host = hp;
    else { host = hp.substr(0, co); port = std::atoi(hp.substr(co + 1).c_str()); }

    addrinfo hints{}; hints.ai_family = AF_INET; hints.ai_socktype = SOCK_STREAM;
    addrinfo* res = nullptr;
    if (getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &res) != 0 || !res) return false;
    const int fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (fd < 0) { freeaddrinfo(res); return false; }
    timeval tv{timeout_s, 0};
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    const bool ok = connect(fd, res->ai_addr, res->ai_addrlen) == 0;
    freeaddrinfo(res);
    if (!ok) { close(fd); return false; }

    std::string req = "POST " + path + " HTTP/1.0\r\nHost: " + host + "\r\n"
                      "Content-Type: application/json\r\n";
    if (!api_key.empty()) req += "Authorization: Bearer " + api_key + "\r\n";
    req += "Content-Length: " + std::to_string(body.size()) + "\r\nConnection: close\r\n\r\n" + body;
    std::size_t sent = 0;
    while (sent < req.size()) {
        const ssize_t n = send(fd, req.data() + sent, req.size() - sent, 0);
        if (n <= 0) { close(fd); return false; }
        sent += (std::size_t)n;
    }
    std::string all; char buf[8192]; ssize_t n;
    while ((n = recv(fd, buf, sizeof(buf), 0)) > 0) all.append(buf, (std::size_t)n);
    close(fd);
    const std::size_t he = all.find("\r\n\r\n");
    if (resp) *resp = (he == std::string::npos) ? all : all.substr(he + 4);
    return all.rfind("HTTP/1.", 0) == 0 && all.find(" 200 ") != std::string::npos;
}

class VlmWorker {
public:
    void Init(const VlmCfg& c) {
        cfg_ = c;
        run_ = true;
        th_ = std::thread([this] { Loop(); });
    }
    ~VlmWorker() { Stop(); }
    void Stop() {
        if (!run_.exchange(false)) return;
        cv_.notify_all();
        if (th_.joinable()) th_.join();
    }

    // 非阻塞投递;队列满丢最旧
    void Submit(Event&& e) {
        {
            std::lock_guard<std::mutex> lk(mu_);
            if ((int)q_.size() >= cfg_.queue_size) q_.pop_front();
            q_.push_back(std::move(e));
        }
        cv_.notify_one();
    }

private:
    void Loop() {
        while (run_) {
            Event e;
            {
                std::unique_lock<std::mutex> lk(mu_);
                cv_.wait(lk, [this] { return !run_ || !q_.empty(); });
                if (!run_) break;
                e = std::move(q_.front());
                q_.pop_front();
            }
            std::string desc;
            const auto t0 = std::chrono::steady_clock::now();
            if (CallVlm(e.image_b64, &desc)) {
                const int ms = (int)std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - t0).count();
                std::fprintf(stderr, "[pcdvlm %s] VLM ok(%dms): %.40s\n", cfg_.stream_name.c_str(), ms, desc.c_str());
                Report(e, desc, ms);
            } else {
                std::fprintf(stderr, "[pcdvlm %s] VLM call FAILED\n", cfg_.stream_name.c_str());
            }
        }
    }

    bool CallVlm(const std::string& jpg_b64, std::string* desc) const {
        using json = nlohmann::json;
        const std::string model = cfg_.model.empty() ? std::string("vlm") : cfg_.model;
        json body = {
            {"model", model}, {"max_tokens", cfg_.max_tokens},
            {"temperature", cfg_.temperature}, {"stream", false},
            {"messages", json::array({
                {{"role", "system"},
                 {"content", json::array({{{"type", "text"}, {"text", cfg_.system_prompt}}})}},
                {{"role", "user"},
                 {"content", json::array({
                     {{"type", "image_url"},
                      {"image_url", {{"url", "data:image/jpeg;base64," + jpg_b64}}}},
                     {{"type", "text"}, {"text", cfg_.prompt}}})}}})}};
        std::string resp;
        bool ok = HttpPostJson(cfg_.url + "/v1/chat/completions", body.dump(), cfg_.api_key, &resp);
        if (!ok && cfg_.retry_once)
            ok = HttpPostJson(cfg_.url + "/v1/chat/completions", body.dump(), cfg_.api_key, &resp);
        if (!ok) return false;
        try {
            auto j = json::parse(resp);
            auto& ct = j["choices"][0]["message"]["content"];
            *desc = ct.is_string() ? ct.get<std::string>() : ct.dump();
        } catch (...) { return false; }
        return true;
    }

    void Report(const Event& e, const std::string& desc, int latency_ms) const {
        if (cfg_.report_url.empty()) return;
        using json = nlohmann::json;
        json item = {
            {"stream", cfg_.stream_name}, {"track_id", e.track_id}, {"cls", e.cls},
            {"score", e.score}, {"box", {e.box[0], e.box[1], e.box[2], e.box[3]}},
            {"ts", e.ts}, {"desc", desc}, {"mode", "single"}, {"latency_ms", latency_ms}, {"image", e.image_b64}};
        if (e.replay.size() > 1) item["replay"] = e.replay;  // ±2s 轮播帧(网页详情页循环播放)
        std::string resp;
        HttpPostJson(cfg_.report_url, item.dump(), "", &resp, 15);
    }

    VlmCfg cfg_;
    std::deque<Event> q_;
    std::mutex mu_;
    std::condition_variable cv_;
    std::atomic<bool> run_{false};
    std::thread th_;
};

}  // namespace pcdvlm
