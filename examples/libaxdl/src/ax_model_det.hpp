#include "../include/ax_model_base.hpp"
#include "utilities/object_register.hpp"

#include "base/detection.hpp"
#include "base/yolo.hpp"
#include "../../utilities/ringbuffer.hpp"

class ax_model_yolov5 : public ax_model_single_base_t
{
protected:
    int post_process(const void *pstFrame, ax_joint_runner_box *crop_resize_box, sample_run_joint_results *results) override;
};
REGISTER(MT_DET_YOLOV5, ax_model_yolov5)

class ax_model_yolov5_seg : public ax_model_single_base_t
{
protected:
    int post_process(const void *pstFrame, ax_joint_runner_box *crop_resize_box, sample_run_joint_results *results) override;
};
REGISTER(MT_INSEG_YOLOV5_MASK, ax_model_yolov5_seg)

class ax_model_yolov5_face : public ax_model_single_base_t
{
protected:
    SimpleRingBuffer<std::vector<sample_run_joint_point>> mSimpleRingBuffer;
    int post_process(const void *pstFrame, ax_joint_runner_box *crop_resize_box, sample_run_joint_results *results) override;
};
REGISTER(MT_DET_YOLOV5_FACE, ax_model_yolov5_face)

class ax_model_yolov5_lisence_plate : public ax_model_single_base_t
{
protected:
    SimpleRingBuffer<std::vector<sample_run_joint_point>> mSimpleRingBuffer;
    int post_process(const void *pstFrame, ax_joint_runner_box *crop_resize_box, sample_run_joint_results *results) override;
};
REGISTER(MT_DET_YOLOV5_LICENSE_PLATE, ax_model_yolov5_lisence_plate)

class ax_model_yolov6 : public ax_model_single_base_t
{
protected:
    int post_process(const void *pstFrame, ax_joint_runner_box *crop_resize_box, sample_run_joint_results *results) override;
};
REGISTER(MT_DET_YOLOV6, ax_model_yolov6)

class ax_model_yolov7 : public ax_model_single_base_t
{
protected:
    int post_process(const void *pstFrame, ax_joint_runner_box *crop_resize_box, sample_run_joint_results *results) override;
};
REGISTER(MT_DET_YOLOV7, ax_model_yolov7)

class ax_model_yolov7_face : public ax_model_single_base_t
{
protected:
    SimpleRingBuffer<std::vector<sample_run_joint_point>> mSimpleRingBuffer;
    int post_process(const void *pstFrame, ax_joint_runner_box *crop_resize_box, sample_run_joint_results *results) override;
};
REGISTER(MT_DET_YOLOV7_FACE, ax_model_yolov7_face)

class ax_model_yolov7_plam_hand : public ax_model_single_base_t
{
protected:
    int post_process(const void *pstFrame, ax_joint_runner_box *crop_resize_box, sample_run_joint_results *results) override;
};
REGISTER(MT_DET_YOLOV7_PALM_HAND, ax_model_yolov7_plam_hand)

class ax_model_plam_hand : public ax_model_single_base_t
{
protected:
    int post_process(const void *pstFrame, ax_joint_runner_box *crop_resize_box, sample_run_joint_results *results) override;
};
REGISTER(MT_DET_PALM_HAND, ax_model_plam_hand)

class ax_model_yolox : public ax_model_single_base_t
{
protected:
    int post_process(const void *pstFrame, ax_joint_runner_box *crop_resize_box, sample_run_joint_results *results) override;
};
REGISTER(MT_DET_YOLOX, ax_model_yolox)

class ax_model_yoloxppl : public ax_model_single_base_t
{
protected:
    int post_process(const void *pstFrame, ax_joint_runner_box *crop_resize_box, sample_run_joint_results *results) override;
};
REGISTER(MT_DET_YOLOX_PPL, ax_model_yoloxppl)

class ax_model_yolopv2 : public ax_model_single_base_t
{
protected:
    SimpleRingBuffer<cv::Mat> mSimpleRingBuffer_seg;
    SimpleRingBuffer<cv::Mat> mSimpleRingBuffer_ll;
    int post_process(const void *pstFrame, ax_joint_runner_box *crop_resize_box, sample_run_joint_results *results) override;
};
REGISTER(MT_DET_YOLOPV2, ax_model_yolopv2)

class ax_model_yolo_fast_body : public ax_model_single_base_t
{
protected:
    yolo::YoloDetectionOutput yolo{};
    std::vector<yolo::TMat> yolo_inputs, yolo_outputs;
    std::vector<float> output_buf;

    bool bInit = false;
    int post_process(const void *pstFrame, ax_joint_runner_box *crop_resize_box, sample_run_joint_results *results) override;
};
REGISTER(MT_DET_YOLO_FASTBODY, ax_model_yolo_fast_body)

class ax_model_nanodet : public ax_model_single_base_t
{
protected:
    int post_process(const void *pstFrame, ax_joint_runner_box *crop_resize_box, sample_run_joint_results *results) override;
};
REGISTER(MT_DET_NANODET, ax_model_nanodet)
