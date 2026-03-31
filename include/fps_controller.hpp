#pragma once

#include <mutex>
#include <thread>
class FpsController {
public:
    explicit FpsController(double max_fps) : max_fps_(max_fps) {
        UpdateInterval();
    }

    void SetMaxFps(double max_fps) {
        std::lock_guard<std::mutex> lock(mu_);
        max_fps_ = max_fps;
        next_ = {};
        UpdateInterval();
    }

    void Throttle() {
        if (max_fps_ <= 0.0) {
            return;
        }

        const auto interval = interval_;
        if (interval <= std::chrono::microseconds(0)) {
            return;
        }

        std::chrono::steady_clock::time_point sleep_until{};
        {
            std::lock_guard<std::mutex> lock(mu_);
            const auto now = std::chrono::steady_clock::now();
            if (next_.time_since_epoch().count() == 0) {
                next_ = now;
            }
            sleep_until = next_;
            // Monotonic scheduling to enforce max FPS.
            next_ = (next_ > now ? next_ : now) + interval;
        }

        const auto now2 = std::chrono::steady_clock::now();
        if (sleep_until > now2) {
            std::this_thread::sleep_for(sleep_until - now2);
        }
    }

private:
    void UpdateInterval() {
        if (max_fps_ <= 0.0) {
            interval_ = std::chrono::microseconds(0);
            return;
        }

        const auto interval_us = static_cast<std::uint64_t>(1000000.0 / max_fps_);
        interval_ = std::chrono::microseconds(interval_us == 0 ? 1 : interval_us);
    }

    std::mutex mu_;
    double max_fps_{0.0};
    std::chrono::microseconds interval_{0};
    std::chrono::steady_clock::time_point next_{};
};