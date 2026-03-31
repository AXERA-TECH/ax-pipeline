#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "npu/models/ax_model_base.hpp"

namespace axpipeline::tracking {

struct TrackedObject {
    float x0{0};
    float y0{0};
    float x1{0};
    float y1{0};
    float score{0};
    int class_id{-1};
    std::int64_t track_id{0};
};

struct ByteTrackOptions {
    int frame_rate{30};
    int track_buffer{30};
    float min_score{0.0F};
};

class ByteTrack {
public:
    explicit ByteTrack(const ByteTrackOptions& opt);
    ~ByteTrack();

    ByteTrack(const ByteTrack&) = delete;
    ByteTrack& operator=(const ByteTrack&) = delete;

    std::vector<TrackedObject> Update(const std::vector<axpipeline::npu::Detection>& dets);

    // 0xRRGGBB (bright + deterministic).
    static std::uint32_t ColorForTrackId(std::uint64_t track_id) noexcept;

private:
    ByteTrackOptions opt_{};
    void* handle_{nullptr};  // bytetracker_t
};

}  // namespace axpipeline::tracking

