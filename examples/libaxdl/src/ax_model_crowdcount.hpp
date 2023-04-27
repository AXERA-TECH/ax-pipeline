#include "../include/ax_model_base.hpp"
#include "utilities/object_register.hpp"
#include "../../utilities/ringbuffer.hpp"
#include "vector"

class ax_model_crowdcount : public ax_model_single_base_t
{
protected:
    char info[128];

    int width_anchor = 0, height_anchor = 0;
    std::vector<float> all_anchor_points;

    SimpleRingBuffer<std::vector<axdl_point_t>> mSimpleRingBuffer;
    int post_process(axdl_image_t *pstFrame, axdl_bbox_t *crop_resize_box, axdl_results_t *results) override;
    void draw_custom(cv::Mat &image, axdl_results_t *results, float fontscale, int thickness, int offset_x, int offset_y) override;
};
REGISTER(MT_DET_CROWD_COUNT, ax_model_crowdcount)