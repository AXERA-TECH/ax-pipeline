#pragma once

#include <cstdint>

namespace axpipeline::ai {

struct Detection {
    float x0{0};
    float y0{0};
    float x1{0};
    float y1{0};
    float score{0};
    int class_id{-1};
    // -1 means "not available" (most detectors don't output track id).
    std::int64_t track_id{-1};
};

}  // namespace axpipeline::ai
