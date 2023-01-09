#include "../include/ax_model_base.hpp"
#include "utilities/object_register.hpp"

#include "../../utilities/ringbuffer.hpp"
#include "opencv2/opencv.hpp"

class ax_model_pphumseg : public ax_model_single_base_t
{
protected:
    SimpleRingBuffer<cv::Mat> mSimpleRingBuffer;
    int post_process(const void *pstFrame, ax_joint_runner_box *crop_resize_box, sample_run_joint_results *results) override;
};
REGISTER(MT_SEG_PPHUMSEG, ax_model_pphumseg)
