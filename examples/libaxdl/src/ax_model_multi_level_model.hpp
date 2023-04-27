#include "../include/ax_model_base.hpp"
#include "utilities/object_register.hpp"
#include "vector"

class ax_model_human_pose_axppl : public ax_model_multi_base_t
{
protected:
    void draw_custom(cv::Mat &image, axdl_results_t *results, float fontscale, int thickness, int offset_x, int offset_y) override;

public:
    int inference(axdl_image_t *pstFrame, axdl_bbox_t *crop_resize_box, axdl_results_t *results) override;
};
REGISTER(MT_MLM_HUMAN_POSE_AXPPL, ax_model_human_pose_axppl)

class ax_model_human_pose_hrnet : public ax_model_human_pose_axppl
{
};
REGISTER(MT_MLM_HUMAN_POSE_HRNET, ax_model_human_pose_hrnet)

class ax_model_animal_pose_hrnet : public ax_model_human_pose_axppl
{
protected:
    void draw_custom(cv::Mat &image, axdl_results_t *results, float fontscale, int thickness, int offset_x, int offset_y) override;
};
REGISTER(MT_MLM_ANIMAL_POSE_HRNET, ax_model_animal_pose_hrnet)

class ax_model_hand_pose : public ax_model_multi_base_t
{
protected:
    void draw_custom(cv::Mat &image, axdl_results_t *results, float fontscale, int thickness, int offset_x, int offset_y) override;

    axdl_image_t pstFrame_RGB = {0};

public:
    void deinit() override;
    int inference(axdl_image_t *pstFrame, axdl_bbox_t *crop_resize_box, axdl_results_t *results) override;
};
REGISTER(MT_MLM_HAND_POSE, ax_model_hand_pose)

class ax_model_face_recognition : public ax_model_multi_base_t
{
protected:
    bool b_face_database_init = false;
    double _calcSimilar(float *feature1, float *feature2, int feature_len)
    {
        double sim = 0.0;
        for (int i = 0; i < feature_len; i++)
            sim += feature1[i] * feature2[i];
        sim = sim < 0 ? 0 : sim > 1 ? 1
                                    : sim;
        return sim;
    }

public:
    int inference(axdl_image_t *pstFrame, axdl_bbox_t *crop_resize_box, axdl_results_t *results) override;
};
REGISTER(MT_MLM_FACE_RECOGNITION, ax_model_face_recognition)

class ax_model_vehicle_license_recognition : public ax_model_multi_base_t
{
public:
    int inference(axdl_image_t *pstFrame, axdl_bbox_t *crop_resize_box, axdl_results_t *results) override;
};
REGISTER(MT_MLM_VEHICLE_LICENSE_RECOGNITION, ax_model_vehicle_license_recognition)