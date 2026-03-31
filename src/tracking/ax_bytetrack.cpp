#include "tracking/ax_bytetrack.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>

#include "bytetrack.h"

namespace axpipeline::tracking {

namespace {

struct Rgb {
    std::uint8_t r;
    std::uint8_t g;
    std::uint8_t b;
};

Rgb HsvToRgb(float h, float s, float v) {
    // h in [0,1)
    h = h - std::floor(h);
    s = std::max(0.0F, std::min(s, 1.0F));
    v = std::max(0.0F, std::min(v, 1.0F));

    const float c = v * s;
    const float hh = h * 6.0F;
    const float x = c * (1.0F - std::fabs(std::fmod(hh, 2.0F) - 1.0F));
    float r1 = 0, g1 = 0, b1 = 0;

    if (hh >= 0.0F && hh < 1.0F) {
        r1 = c; g1 = x; b1 = 0;
    } else if (hh < 2.0F) {
        r1 = x; g1 = c; b1 = 0;
    } else if (hh < 3.0F) {
        r1 = 0; g1 = c; b1 = x;
    } else if (hh < 4.0F) {
        r1 = 0; g1 = x; b1 = c;
    } else if (hh < 5.0F) {
        r1 = x; g1 = 0; b1 = c;
    } else {
        r1 = c; g1 = 0; b1 = x;
    }

    const float m = v - c;
    const auto r = static_cast<std::uint8_t>(std::lround((r1 + m) * 255.0F));
    const auto g = static_cast<std::uint8_t>(std::lround((g1 + m) * 255.0F));
    const auto b = static_cast<std::uint8_t>(std::lround((b1 + m) * 255.0F));
    return {r, g, b};
}

std::uint32_t Hash32(std::uint32_t x) noexcept {
    // Murmur3 finalizer.
    x ^= x >> 16U;
    x *= 0x7FEB352DU;
    x ^= x >> 15U;
    x *= 0x846CA68BU;
    x ^= x >> 16U;
    return x;
}

}  // namespace

ByteTrack::ByteTrack(const ByteTrackOptions& opt)
    : opt_(opt) {
    const int fps = opt_.frame_rate > 0 ? opt_.frame_rate : 30;
    const int buf = opt_.track_buffer > 0 ? opt_.track_buffer : 30;
    handle_ = bytetracker_create(fps, buf);
}

ByteTrack::~ByteTrack() {
    if (handle_ != nullptr) {
        bytetracker_t tmp = handle_;
        bytetracker_release(&tmp);
        handle_ = nullptr;
    }
}

std::vector<TrackedObject> ByteTrack::Update(const std::vector<axpipeline::ai::Detection>& dets) {
    std::vector<TrackedObject> out;
    if (handle_ == nullptr) {
        return out;
    }

    bytetrack_object_t objs{};
    objs.n_objects = 0;
    for (const auto& d : dets) {
        if (d.score < opt_.min_score) {
            continue;
        }
        if (objs.n_objects >= TRACK_OBJETCS_MAX_SIZE) {
            break;
        }

        auto& o = objs.objects[objs.n_objects++];
        o.label = d.class_id;
        o.prob = d.score;
        o.rect.x = d.x0;
        o.rect.y = d.y0;
        o.rect.width = std::max(0.0F, d.x1 - d.x0);
        o.rect.height = std::max(0.0F, d.y1 - d.y0);
        o.track_id = 0;
        o.user_data = nullptr;
    }

    bytetracker_track(handle_, &objs);

    out.reserve(static_cast<std::size_t>(objs.n_track_objects));
    for (int i = 0; i < objs.n_track_objects; ++i) {
        const auto& t = objs.track_objects[i];
        TrackedObject o{};
        o.class_id = t.label;
        o.score = t.prob;
        o.track_id = static_cast<std::int64_t>(t.track_id);
        o.x0 = t.rect.x;
        o.y0 = t.rect.y;
        o.x1 = t.rect.x + t.rect.width;
        o.y1 = t.rect.y + t.rect.height;
        out.push_back(o);
    }
    return out;
}

std::uint32_t ByteTrack::ColorForTrackId(std::uint64_t track_id) noexcept {
    // Deterministic: each track_id gets a stable, bright color.
    const std::uint32_t h = Hash32(static_cast<std::uint32_t>(track_id));
    const float hue = static_cast<float>(h % 360U) / 360.0F;
    const auto rgb = HsvToRgb(hue, 0.90F, 0.95F);
    return (static_cast<std::uint32_t>(rgb.r) << 16U) |
           (static_cast<std::uint32_t>(rgb.g) << 8U) |
           (static_cast<std::uint32_t>(rgb.b));
}

}  // namespace axpipeline::tracking
