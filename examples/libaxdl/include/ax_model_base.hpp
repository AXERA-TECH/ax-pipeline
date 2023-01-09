#pragma once
#include "sample_run_joint_post_process.h"
#include "ax_joint_runner.hpp"

#include "vector"
#include "string"
#include "memory"

#include "npu_cv_kit/ax_npu_imgproc.h"

struct ax_model_faceid
{
    std::string name;
    std::string path;
    std::vector<float> feat;
};

class ax_model_base_t
{
protected:
    int HEIGHT_DET_BBOX_RESTORE = 1080, WIDTH_DET_BBOX_RESTORE = 1920;
    SAMPLE_RUN_JOINT_MODEL_TYPE m_model_type = MT_UNKNOWN;
    // detection
    float PROB_THRESHOLD = 0.4f;
    float NMS_THRESHOLD = 0.45f;
    int CLASS_NUM = 80;
    std::vector<float> ANCHORS = {12, 16, 19, 36, 40, 28,
                                  36, 75, 76, 55, 72, 146,
                                  142, 110, 192, 243, 459, 401};
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

    // face recognition
    float FACE_RECOGNITION_THRESHOLD = 0.4f;
    std::vector<ax_model_faceid> face_register_ids;

    // multi level model
    // int NUM_CLASS_ID;
    std::vector<int> CLASS_IDS;

    int cur_idx = 0;
public:
    // 当前处理的的对象在 results->mObjects 的索引（多级模型用）
    int set_current_index(int idx) { cur_idx = idx; }
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

    void set_class_names(std::vector<std::string> &class_namse) { CLASS_NAMES = class_namse; }

    std::vector<std::string> get_class_names() { return CLASS_NAMES; }

    void set_face_recognition_threshold(float face_recognition_threshold) { FACE_RECOGNITION_THRESHOLD = face_recognition_threshold; }

    float get_face_recognition_threshold() { return FACE_RECOGNITION_THRESHOLD; }

    void set_face_register_ids(std::vector<ax_model_faceid> &ids) { face_register_ids = ids; }

    std::vector<ax_model_faceid> get_face_register_ids() { return face_register_ids; }

    static int get_model_type(void *json_obj, std::string &strModelType);

    virtual int init(void *json_obj) = 0;
    virtual void deinit() = 0;
    virtual int get_color_space() = 0;
    virtual int get_algo_width() = 0;
    virtual int get_algo_height() = 0;
    virtual int inference(const void *pstFrame, ax_joint_runner_box *crop_resize_box, sample_run_joint_results *results) = 0;
};

class ax_model_single_base_t : public ax_model_base_t
{
protected:
    ax_joint_runner m_runner;
    std::string m_model_path;

    AX_NPU_CV_Image dstFrame = {0};
    bool bMalloc = false;

    virtual int preprocess(const void *srcFrame, ax_joint_runner_box *crop_resize_box, sample_run_joint_results *results);
    virtual int post_process(const void *srcFrame, ax_joint_runner_box *crop_resize_box, sample_run_joint_results *results) = 0;

public:
    virtual int init(void *json_obj) override;
    virtual void deinit() override;

    int get_color_space() override { return m_runner.m_attr.algo_colorformat; }
    int get_algo_width() override { return m_runner.m_attr.algo_width; }
    int get_algo_height() override { return m_runner.m_attr.algo_height; }

    virtual int inference(const void *pstFrame, ax_joint_runner_box *crop_resize_box, sample_run_joint_results *results) override;
};

class ax_model_multi_base_t : public ax_model_base_t
{
protected:
    std::shared_ptr<ax_model_base_t> model_0;
    std::shared_ptr<ax_model_base_t> model_1;

public:
    void set_det_restore_resolution(int width, int height) override
    {
        model_0->set_det_restore_resolution(width, height);
    }

    void get_det_restore_resolution(int &width, int &height) override
    {
        model_0->get_det_restore_resolution(width, height);
    }
    // virtual int init(char *json_file_path);
    virtual int init(void *json_obj) override;
    virtual void deinit() override;
    int get_color_space() override { return model_0->get_color_space(); }
    int get_algo_width() override { return model_0->get_algo_width(); }
    int get_algo_height() override { return model_0->get_algo_height(); }
};
