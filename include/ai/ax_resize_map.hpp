#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

#include "ai/ax_detection.hpp"
#include "common/ax_image_processor.h"

namespace axpipeline::ai {

struct ResizeMap {
    bool identity{true};
    axvsdk::common::ResizeMode mode{axvsdk::common::ResizeMode::kStretch};
    // For keep_aspect:
    float scale{1.0F};
    float pad_x{0.0F};
    float pad_y{0.0F};
    // For stretch:
    float scale_x{1.0F};
    float scale_y{1.0F};
};

inline ResizeMap ComputeInferToSourceMap(std::uint32_t source_w,
                                        std::uint32_t source_h,
                                        std::uint32_t infer_w,
                                        std::uint32_t infer_h,
                                        const axvsdk::common::ResizeOptions& resize) {
    ResizeMap m{};
    if (source_w == 0 || source_h == 0 || infer_w == 0 || infer_h == 0) {
        return m;
    }

    if (source_w == infer_w && source_h == infer_h) {
        m.identity = true;
        m.mode = resize.mode;
        return m;
    }

    m.identity = false;
    m.mode = resize.mode;

    if (resize.mode == axvsdk::common::ResizeMode::kKeepAspectRatio) {
        const float sx = static_cast<float>(infer_w) / static_cast<float>(source_w);
        const float sy = static_cast<float>(infer_h) / static_cast<float>(source_h);
        const float scale = std::min(sx, sy);
        m.scale = scale > 0.0F ? scale : 1.0F;

        const std::uint32_t new_w = static_cast<std::uint32_t>(std::lround(static_cast<float>(source_w) * m.scale));
        const std::uint32_t new_h = static_cast<std::uint32_t>(std::lround(static_cast<float>(source_h) * m.scale));
        const std::uint32_t pad_w = (infer_w > new_w) ? (infer_w - new_w) : 0U;
        const std::uint32_t pad_h = (infer_h > new_h) ? (infer_h - new_h) : 0U;

        const std::uint32_t pad_x0 = [&]() -> std::uint32_t {
            switch (resize.horizontal_align) {
            case axvsdk::common::ResizeAlign::kStart:
                return 0U;
            case axvsdk::common::ResizeAlign::kEnd:
                return pad_w;
            case axvsdk::common::ResizeAlign::kCenter:
            default:
                return pad_w / 2U;
            }
        }();
        const std::uint32_t pad_y0 = [&]() -> std::uint32_t {
            switch (resize.vertical_align) {
            case axvsdk::common::ResizeAlign::kStart:
                return 0U;
            case axvsdk::common::ResizeAlign::kEnd:
                return pad_h;
            case axvsdk::common::ResizeAlign::kCenter:
            default:
                return pad_h / 2U;
            }
        }();

        m.pad_x = static_cast<float>(pad_x0);
        m.pad_y = static_cast<float>(pad_y0);
        return m;
    }

    // Stretch: inverse scale from infer -> source.
    m.scale_x = static_cast<float>(source_w) / static_cast<float>(infer_w);
    m.scale_y = static_cast<float>(source_h) / static_cast<float>(infer_h);
    return m;
}

inline void MapDetectionsInferToSource(const ResizeMap& map,
                                      std::uint32_t source_w,
                                      std::uint32_t source_h,
                                      std::vector<Detection>* dets) {
    if (!dets || dets->empty()) return;
    if (map.identity) return;

    const auto clamp_x = [&](float x) -> float {
        return std::max(0.0F, std::min(x, static_cast<float>(source_w)));
    };
    const auto clamp_y = [&](float y) -> float {
        return std::max(0.0F, std::min(y, static_cast<float>(source_h)));
    };

    for (auto& d : *dets) {
        if (map.mode == axvsdk::common::ResizeMode::kKeepAspectRatio) {
            const float inv = (map.scale > 0.0F) ? (1.0F / map.scale) : 1.0F;
            d.x0 = (d.x0 - map.pad_x) * inv;
            d.y0 = (d.y0 - map.pad_y) * inv;
            d.x1 = (d.x1 - map.pad_x) * inv;
            d.y1 = (d.y1 - map.pad_y) * inv;
        } else {
            d.x0 = d.x0 * map.scale_x;
            d.y0 = d.y0 * map.scale_y;
            d.x1 = d.x1 * map.scale_x;
            d.y1 = d.y1 * map.scale_y;
        }

        if (d.x1 < d.x0) std::swap(d.x0, d.x1);
        if (d.y1 < d.y0) std::swap(d.y0, d.y1);

        d.x0 = clamp_x(d.x0);
        d.y0 = clamp_y(d.y0);
        d.x1 = clamp_x(d.x1);
        d.y1 = clamp_y(d.y1);
    }
}

}  // namespace axpipeline::ai

