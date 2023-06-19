#include "../include/ax_model_base.hpp"
#include "utilities/object_register.hpp"

#include "base/detection.hpp"
#include "base/yolo.hpp"
#include "../../utilities/ringbuffer.hpp"

class ax_model_yolov5 : public ax_model_single_base_t
{
protected:
    int post_process(axdl_image_t *pstFrame, axdl_bbox_t *crop_resize_box, axdl_results_t *results) override;
};
REGISTER(MT_DET_YOLOV5, ax_model_yolov5)

class ax_model_yolov5_seg : public ax_model_single_base_t
{
protected:
    SimpleRingBuffer<cv::Mat> mSimpleRingBuffer;
    int post_process(axdl_image_t *pstFrame, axdl_bbox_t *crop_resize_box, axdl_results_t *results) override;
    void draw_custom(cv::Mat &image, axdl_results_t *results, float fontscale, int thickness, int offset_x, int offset_y) override;
    void draw_custom(int chn, axdl_results_t *results, float fontscale, int thickness) override;
};
REGISTER(MT_INSEG_YOLOV5_MASK, ax_model_yolov5_seg)

class ax_model_yolov5_face : public ax_model_single_base_t
{
protected:
    SimpleRingBuffer<std::vector<axdl_point_t>> mSimpleRingBuffer;
    int post_process(axdl_image_t *pstFrame, axdl_bbox_t *crop_resize_box, axdl_results_t *results) override;
    void draw_custom(cv::Mat &image, axdl_results_t *results, float fontscale, int thickness, int offset_x, int offset_y) override;
    void draw_custom(int chn, axdl_results_t *results, float fontscale, int thickness) override;
};
REGISTER(MT_DET_YOLOV5_FACE, ax_model_yolov5_face)

class ax_model_yolov5_lisence_plate : public ax_model_single_base_t
{
protected:
    SimpleRingBuffer<std::vector<axdl_point_t>> mSimpleRingBuffer;
    int post_process(axdl_image_t *pstFrame, axdl_bbox_t *crop_resize_box, axdl_results_t *results) override;
};
REGISTER(MT_DET_YOLOV5_LICENSE_PLATE, ax_model_yolov5_lisence_plate)

class ax_model_yolov6 : public ax_model_single_base_t
{
protected:
    int post_process(axdl_image_t *pstFrame, axdl_bbox_t *crop_resize_box, axdl_results_t *results) override;
};
REGISTER(MT_DET_YOLOV6, ax_model_yolov6)

class ax_model_yolov7 : public ax_model_single_base_t
{
protected:
    int post_process(axdl_image_t *pstFrame, axdl_bbox_t *crop_resize_box, axdl_results_t *results) override;
};
REGISTER(MT_DET_YOLOV7, ax_model_yolov7)

class ax_model_yolov7_face : public ax_model_yolov5_face
{
protected:
    int post_process(axdl_image_t *pstFrame, axdl_bbox_t *crop_resize_box, axdl_results_t *results) override;
};
REGISTER(MT_DET_YOLOV7_FACE, ax_model_yolov7_face)

class ax_model_yolov7_plam_hand : public ax_model_single_base_t
{
protected:
    int post_process(axdl_image_t *pstFrame, axdl_bbox_t *crop_resize_box, axdl_results_t *results) override;
};
REGISTER(MT_DET_YOLOV7_PALM_HAND, ax_model_yolov7_plam_hand)

class ax_model_plam_hand : public ax_model_single_base_t
{
protected:
    int post_process(axdl_image_t *pstFrame, axdl_bbox_t *crop_resize_box, axdl_results_t *results) override;
};
REGISTER(MT_DET_PALM_HAND, ax_model_plam_hand)

class ax_model_yolox : public ax_model_single_base_t
{
protected:
    int post_process(axdl_image_t *pstFrame, axdl_bbox_t *crop_resize_box, axdl_results_t *results) override;
};
REGISTER(MT_DET_YOLOX, ax_model_yolox)

class ax_model_yoloxppl : public ax_model_single_base_t
{
protected:
    int post_process(axdl_image_t *pstFrame, axdl_bbox_t *crop_resize_box, axdl_results_t *results) override;
};
REGISTER(MT_DET_YOLOX_PPL, ax_model_yoloxppl)

class ax_model_yolopv2 : public ax_model_single_base_t
{
protected:
    cv::Mat base_canvas;
    SimpleRingBuffer<cv::Mat> mSimpleRingBuffer_seg;
    SimpleRingBuffer<cv::Mat> mSimpleRingBuffer_ll;
    int post_process(axdl_image_t *pstFrame, axdl_bbox_t *crop_resize_box, axdl_results_t *results) override;
    void draw_custom(cv::Mat &image, axdl_results_t *results, float fontscale, int thickness, int offset_x, int offset_y) override;
    void draw_custom(int chn, axdl_results_t *results, float fontscale, int thickness) override;
};
REGISTER(MT_DET_YOLOPV2, ax_model_yolopv2)

class ax_model_yolo_fast_body : public ax_model_single_base_t
{
protected:
    yolo::YoloDetectionOutput yolo{};
    std::vector<yolo::TMat> yolo_inputs, yolo_outputs;
    std::vector<float> output_buf;

    bool bInit = false;
    int post_process(axdl_image_t *pstFrame, axdl_bbox_t *crop_resize_box, axdl_results_t *results) override;
};
REGISTER(MT_DET_YOLO_FASTBODY, ax_model_yolo_fast_body)

class ax_model_nanodet : public ax_model_single_base_t
{
protected:
    int post_process(axdl_image_t *pstFrame, axdl_bbox_t *crop_resize_box, axdl_results_t *results) override;
};
REGISTER(MT_DET_NANODET, ax_model_nanodet)

class ax_model_scrfd : public ax_model_yolov5_face
{
protected:
    int post_process(axdl_image_t *pstFrame, axdl_bbox_t *crop_resize_box, axdl_results_t *results) override;
};
REGISTER(MT_DET_SCRFD, ax_model_scrfd)

class ax_model_yolov8 : public ax_model_yolov5
{
protected:
    int post_process(axdl_image_t *pstFrame, axdl_bbox_t *crop_resize_box, axdl_results_t *results) override;
};
REGISTER(MT_DET_YOLOV8, ax_model_yolov8)

class ax_model_yolov8_650 : public ax_model_yolov5
{
protected:
    int num_grid = 0;
    std::vector<std::vector<float>> grids;
    int post_process(axdl_image_t *pstFrame, axdl_bbox_t *crop_resize_box, axdl_results_t *results) override;
};
REGISTER(MT_DET_YOLOV8_650, ax_model_yolov8_650)

class ax_model_yolov8_seg : public ax_model_yolov5_seg
{
protected:
    int post_process(axdl_image_t *pstFrame, axdl_bbox_t *crop_resize_box, axdl_results_t *results) override;
};
REGISTER(MT_DET_YOLOV8_SEG, ax_model_yolov8_seg)

class ax_model_yolov8_pose : public ax_model_yolov5_face
{
protected:
    // int NUM_POINT = 17;
    int post_process(axdl_image_t *pstFrame, axdl_bbox_t *crop_resize_box, axdl_results_t *results) override;
    void draw_custom(cv::Mat &image, axdl_results_t *results, float fontscale, int thickness, int offset_x, int offset_y) override;
    void draw_custom(int chn, axdl_results_t *results, float fontscale, int thickness) override;
};
REGISTER(MT_DET_YOLOV8_POSE, ax_model_yolov8_pose)

class ax_model_yolov8_pose_650 : public ax_model_yolov8_pose
{
protected:
    std::vector<std::vector<float>> grids;
    // int NUM_POINT = 17;
    int post_process(axdl_image_t *pstFrame, axdl_bbox_t *crop_resize_box, axdl_results_t *results) override;
};
REGISTER(MT_DET_YOLOV8_POSE_650, ax_model_yolov8_pose_650)

class ax_model_yolonas : public ax_model_single_base_t
{
protected:
    int post_process(axdl_image_t *pstFrame, axdl_bbox_t *crop_resize_box, axdl_results_t *results) override;
};
REGISTER(MT_DET_YOLO_NAS, ax_model_yolonas)

class ax_model_ppyoloe : public ax_model_single_base_t
{
protected:
    int post_process(axdl_image_t *pstFrame, axdl_bbox_t *crop_resize_box, axdl_results_t *results) override;
};
REGISTER(MT_DET_PPYOLOE, ax_model_ppyoloe)

