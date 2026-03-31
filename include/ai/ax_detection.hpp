#pragma once

namespace axpipeline::ai {

struct Detection {
    float x0{0};
    float y0{0};
    float x1{0};
    float y1{0};
    float score{0};
    int class_id{-1};
};

}  // namespace axpipeline::ai

