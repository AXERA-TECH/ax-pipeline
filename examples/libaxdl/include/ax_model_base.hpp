#pragma once
#include "c_api.h"
#include "ax_model_runner.hpp"
#include "ax_osd_drawer.hpp"

#include "vector"
#include "string"
#include "memory"
#include "bytetrack.h"
#include "opencv2/opencv.hpp"

class ax_model_base
{
protected:
    // int MAX_FACE_BBOX_COUNT  = 64;
    // int MAX_MASK_OBJ_COUNT   = 8;
    int MAX_SUB_INFER_COUNT  = 3;
    int FACE_FEAT_LEN        = 512;
    // bool use_warp_preprocess = true; // 当crop resize不行时使用

    int HEIGHT_DET_BBOX_RESTORE = 1080, WIDTH_DET_BBOX_RESTORE = 1920;
    MODEL_TYPE_E m_model_type = MT_UNKNOWN;
#ifdef AXERA_TARGET_CHIP_AX620
    RUNNER_TYPE_E m_runner_type = RUNNER_AX620;
#elif defined(AXERA_TARGET_CHIP_AX650)
    RUNNER_TYPE_E m_runner_type = RUNNER_AX650;
#endif
    // detection
    float PROB_THRESHOLD = 0.4f;
    float NMS_THRESHOLD = 0.45f;
    int CLASS_NUM = 80;
    std::vector<float> ANCHORS = {12, 16, 19, 36, 40, 28,
                                  36, 75, 76, 55, 72, 146,
                                  142, 110, 192, 243, 459, 401};
    std::vector<int> STRIDES = {8, 16, 32};
    std::vector<std::string> CLASS_NAMES = {
        "person", "bicycle", "car", "motorcycle", "airplane", "bus", "train", "truck", "boat", "traffic light",
        "fire hydrant", "stop sign", "parking meter", "bench", "bird", "cat", "dog", "horse", "sheep", "cow",
        "elephant", "bear", "zebra", "giraffe", "backpack", "umbrella", "handbag", "tie", "suitcase", "frisbee",
        "skis", "snowboard", "sports ball", "kite", "baseball bat", "baseball glove", "skateboard", "surfboard",
        "tennis racket", "bottle", "wine glass", "cup", "fork", "knife", "spoon", "bowl", "banana", "apple",
        "sandwich", "orange", "broccoli", "carrot", "hot dog", "pizza", "donut", "cake", "chair", "couch",
        "potted plant", "bed", "dining table", "toilet", "tv", "laptop", "mouse", "remote", "keyboard", "cell phone",
        "microwave", "oven", "toaster", "sink", "refrigerator", "book", "clock", "vase", "scissors", "teddy bear",
        "hair drier", "toothbrush"};
    const std::vector<cv::Scalar> COCO_COLORS = {
        {128, 56, 0, 255}, {128, 226, 255, 0}, {128, 0, 94, 255}, {128, 0, 37, 255}, {128, 0, 255, 94},
        {128, 255, 226, 0}, {128, 0, 18, 255}, {128, 255, 151, 0}, {128, 170, 0, 255}, {128, 0, 255, 56},
        {128, 255, 0, 75}, {128, 0, 75, 255}, {128, 0, 255, 169}, {128, 255, 0, 207}, {128, 75, 255, 0},
        {128, 207, 0, 255}, {128, 37, 0, 255}, {128, 0, 207, 255}, {128, 94, 0, 255}, {128, 0, 255, 113},
        {128, 255, 18, 0}, {128, 255, 0, 56}, {128, 18, 0, 255}, {128, 0, 255, 226}, {128, 170, 255, 0},
        {128, 255, 0, 245}, {128, 151, 255, 0}, {128, 132, 255, 0}, {128, 75, 0, 255}, {128, 151, 0, 255},
        {128, 0, 151, 255}, {128, 132, 0, 255}, {128, 0, 255, 245}, {128, 255, 132, 0}, {128, 226, 0, 255},
        {128, 255, 37, 0}, {128, 207, 255, 0}, {128, 0, 255, 207}, {128, 94, 255, 0}, {128, 0, 226, 255},
        {128, 56, 255, 0}, {128, 255, 94, 0}, {128, 255, 113, 0}, {128, 0, 132, 255}, {128, 255, 0, 132},
        {128, 255, 170, 0}, {128, 255, 0, 188}, {128, 113, 255, 0}, {128, 245, 0, 255}, {128, 113, 0, 255},
        {128, 255, 188, 0}, {128, 0, 113, 255}, {128, 255, 0, 0}, {128, 0, 56, 255}, {128, 255, 0, 113},
        {128, 0, 255, 188}, {128, 255, 0, 94}, {128, 255, 0, 18}, {128, 18, 255, 0}, {128, 0, 255, 132},
        {128, 0, 188, 255}, {128, 0, 245, 255}, {128, 0, 169, 255}, {128, 37, 255, 0}, {128, 255, 0, 151},
        {128, 188, 0, 255}, {128, 0, 255, 37}, {128, 0, 255, 0}, {128, 255, 0, 170}, {128, 255, 0, 37},
        {128, 255, 75, 0}, {128, 0, 0, 255}, {128, 255, 207, 0}, {128, 255, 0, 226}, {128, 255, 245, 0},
        {128, 188, 255, 0}, {128, 0, 255, 18}, {128, 0, 255, 75}, {128, 0, 255, 151}, {128, 255, 56, 0}, {128, 245, 255, 0}};

    const std::vector<ax_osd_drawer::ax_abgr_t> COCO_COLORS_ARGB = {
        {128, 56, 0, 255}, {128, 226, 255, 0}, {128, 0, 94, 255}, {128, 0, 37, 255}, {128, 0, 255, 94},
        {128, 255, 226, 0}, {128, 0, 18, 255}, {128, 255, 151, 0}, {128, 170, 0, 255}, {128, 0, 255, 56},
        {128, 255, 0, 75}, {128, 0, 75, 255}, {128, 0, 255, 169}, {128, 255, 0, 207}, {128, 75, 255, 0},
        {128, 207, 0, 255}, {128, 37, 0, 255}, {128, 0, 207, 255}, {128, 94, 0, 255}, {128, 0, 255, 113},
        {128, 255, 18, 0}, {128, 255, 0, 56}, {128, 18, 0, 255}, {128, 0, 255, 226}, {128, 170, 255, 0},
        {128, 255, 0, 245}, {128, 151, 255, 0}, {128, 132, 255, 0}, {128, 75, 0, 255}, {128, 151, 0, 255},
        {128, 0, 151, 255}, {128, 132, 0, 255}, {128, 0, 255, 245}, {128, 255, 132, 0}, {128, 226, 0, 255},
        {128, 255, 37, 0}, {128, 207, 255, 0}, {128, 0, 255, 207}, {128, 94, 255, 0}, {128, 0, 226, 255},
        {128, 56, 255, 0}, {128, 255, 94, 0}, {128, 255, 113, 0}, {128, 0, 132, 255}, {128, 255, 0, 132},
        {128, 255, 170, 0}, {128, 255, 0, 188}, {128, 113, 255, 0}, {128, 245, 0, 255}, {128, 113, 0, 255},
        {128, 255, 188, 0}, {128, 0, 113, 255}, {128, 255, 0, 0}, {128, 0, 56, 255}, {128, 255, 0, 113},
        {128, 0, 255, 188}, {128, 255, 0, 94}, {128, 255, 0, 18}, {128, 18, 255, 0}, {128, 0, 255, 132},
        {128, 0, 188, 255}, {128, 0, 245, 255}, {128, 0, 169, 255}, {128, 37, 255, 0}, {128, 255, 0, 151},
        {128, 188, 0, 255}, {128, 0, 255, 37}, {128, 0, 255, 0}, {128, 255, 0, 170}, {128, 255, 0, 37},
        {128, 255, 75, 0}, {128, 0, 0, 255}, {128, 255, 207, 0}, {128, 255, 0, 226}, {128, 255, 245, 0},
        {128, 188, 255, 0}, {128, 0, 255, 18}, {128, 0, 255, 75}, {128, 0, 255, 151}, {128, 255, 56, 0}, {128, 245, 255, 0}};
    // face recognition
    struct ax_model_faceid
    {
        std::string name;
        std::string path;
        std::vector<float> feat;
    };
    float FACE_RECOGNITION_THRESHOLD = 0.4f;
    std::vector<ax_model_faceid> face_register_ids;

    bool b_track = false;
    bytetracker_t tracker = nullptr;
    bytetrack_object_t tracker_objs = {0};

    // multi level model
    std::vector<int> CLASS_IDS;

    // pipe chn
    bool b_draw_obj_name = true;
    std::map<int, ax_osd_drawer> m_drawers;

    int cur_idx = 0;

    char fps_info[128];
    void draw_bbox(cv::Mat &image, axdl_results_t *results, float fontscale, int thickness, int offset_x, int offset_y);
    void draw_fps(cv::Mat &image, axdl_results_t *results, float fontscale, int thickness, int offset_x, int offset_y);
    virtual void draw_custom(cv::Mat &image, axdl_results_t *results, float fontscale, int thickness, int offset_x, int offset_y)
    {
        draw_bbox(image, results, fontscale, thickness, offset_x, offset_y);
    }

    // 
    void draw_bbox(int chn, axdl_results_t *results, float fontscale, int thickness);
    void draw_fps(int chn, axdl_results_t *results, float fontscale, int thickness);
    virtual void draw_custom(int chn, axdl_results_t *results, float fontscale, int thickness)
    {
        draw_bbox(chn, results, fontscale, thickness);
    }

public:
    void draw_init(int chn,int chn_width,int chn_height,int max_num_rgn){ m_drawers[chn].init(max_num_rgn, chn_width, chn_height);}
    ax_osd_drawer& get_drawer(int chn){ return m_drawers[chn]; }
    int get_sub_infer_count()     { return MAX_SUB_INFER_COUNT; }
    int get_track_enable()  { return b_track ; }
    int get_face_feat_len()       { return FACE_FEAT_LEN      ; }

    int set_sub_infer_count(int val)     { MAX_SUB_INFER_COUNT = val; }
    int set_track_enable(bool val)  { b_track  = val; }
    int set_face_feat_len(int val)       { FACE_FEAT_LEN       = val; }

    // 当前处理的的对象在 results->mObjects 的索引（多级模型用）
    void set_current_index(int idx) { cur_idx = idx; }
    virtual void set_det_restore_resolution(int width, int height)
    {
        WIDTH_DET_BBOX_RESTORE = width;
        HEIGHT_DET_BBOX_RESTORE = height;
    }

    virtual void get_det_restore_resolution(int &width, int &height)
    {
        width = WIDTH_DET_BBOX_RESTORE;
        height = HEIGHT_DET_BBOX_RESTORE;
    }

    int get_model_type() { return m_model_type; }

    void set_nms_threshold(float nms_threshold) { NMS_THRESHOLD = nms_threshold; }

    float get_nms_threshold() { return NMS_THRESHOLD; }

    void set_det_threshold(float det_threshold) { PROB_THRESHOLD = det_threshold; }

    float get_det_threshold() { return PROB_THRESHOLD; }

    void set_num_class(int num_class) { CLASS_NUM = num_class; }

    int get_num_class() { return CLASS_NUM; }

    void set_anchors(std::vector<float> &anchors) { ANCHORS = anchors; }

    std::vector<float> get_anchors() { return ANCHORS; }

    void set_strides(std::vector<int> &strides) { STRIDES = strides; }

    std::vector<int> get_strides() { return STRIDES; }

    void set_class_names(std::vector<std::string> &class_namse) { CLASS_NAMES = class_namse; }

    std::vector<std::string> get_class_names() { return CLASS_NAMES; }

    void set_face_recognition_threshold(float face_recognition_threshold) { FACE_RECOGNITION_THRESHOLD = face_recognition_threshold; }

    float get_face_recognition_threshold() { return FACE_RECOGNITION_THRESHOLD; }

    void set_face_register_ids(std::vector<ax_model_faceid> &ids) { face_register_ids = ids; }

    std::vector<ax_model_faceid> get_face_register_ids() { return face_register_ids; }

    static int get_model_type(void *json_obj, std::string &strModelType);
    static int get_runner_type(void *json_obj, std::string &strRunnerType);

    static bool get_track_enable(void *json_obj);
    virtual void enable_track(int frame_rate = 30, int track_buffer = 30);
    virtual void disbale_track();

    virtual int init(void *json_obj) = 0;
    virtual void deinit() = 0;
    virtual axdl_color_space_e get_color_space() = 0;
    virtual int get_algo_width() = 0;
    virtual int get_algo_height() = 0;
    virtual int inference(axdl_image_t *pstFrame, axdl_bbox_t *crop_resize_box, axdl_results_t *results) = 0;
    virtual void draw_results(cv::Mat&canvas, axdl_results_t *results, float fontscale, int thickness, int offset_x, int offset_y)
    {
        draw_custom(canvas, results, fontscale, thickness, offset_x, offset_y);
        draw_fps(canvas, results, fontscale, thickness, offset_x, offset_y);
    }

    virtual void draw_results(int chn, axdl_results_t *results, float fontscale, int thickness)
    {
        m_drawers[chn].reset();
        draw_custom(chn, results, fontscale, thickness);
        draw_fps(chn, results, fontscale, thickness);
    }
};

class ax_model_single_base_t : public ax_model_base
{
protected:
    std::shared_ptr<ax_runner_base> m_runner;
    std::string MODEL_PATH;

    axdl_image_t dstFrame = {0};
    bool bMalloc = false;

    virtual int preprocess(axdl_image_t *srcFrame, axdl_bbox_t *crop_resize_box, axdl_results_t *results);
    virtual int post_process(axdl_image_t *srcFrame, axdl_bbox_t *crop_resize_box, axdl_results_t *results) = 0;

public:
    virtual int init(void *json_obj) override;
    virtual void deinit() override;

    axdl_color_space_e get_color_space() override { return m_runner->get_color_space(); }
    int get_algo_width() override { return m_runner->get_algo_width(); }
    int get_algo_height() override { return m_runner->get_algo_height(); }

    virtual int inference(axdl_image_t *pstFrame, axdl_bbox_t *crop_resize_box, axdl_results_t *results) override;
};

class ax_model_multi_base_t : public ax_model_base
{
protected:
    std::shared_ptr<ax_model_base> model_0;
    std::shared_ptr<ax_model_base> model_1;

public:
    void set_det_restore_resolution(int width, int height) override
    {
        model_0->set_det_restore_resolution(width, height);
    }

    void get_det_restore_resolution(int &width, int &height) override
    {
        model_0->get_det_restore_resolution(width, height);
    }

    virtual void enable_track(int frame_rate = 30, int track_buffer = 30) override;
    virtual void disbale_track() override;

    // virtual int init(char *json_file_path);
    virtual int init(void *json_obj) override;
    virtual void deinit() override;
    axdl_color_space_e get_color_space() override { return model_0->get_color_space(); }
    int get_algo_width() override { return model_0->get_algo_width(); }
    int get_algo_height() override { return model_0->get_algo_height(); }
};
