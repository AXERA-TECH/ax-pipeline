#include "../include/ax_model_base.hpp"
#include "utilities/object_register.hpp"
#include "vector"

class ax_model_human_pose_axppl : public ax_model_multi_base_t
{
public:
    int inference(const void *pstFrame, ax_joint_runner_box *crop_resize_box, sample_run_joint_results *results) override;
};
REGISTER(MT_MLM_HUMAN_POSE_AXPPL, ax_model_human_pose_axppl)

class ax_model_human_pose_hrnet : public ax_model_human_pose_axppl
{
};
REGISTER(MT_MLM_HUMAN_POSE_HRNET, ax_model_human_pose_hrnet)

class ax_model_animal_pose_hrnet : public ax_model_multi_base_t
{
public:
    int inference(const void *pstFrame, ax_joint_runner_box *crop_resize_box, sample_run_joint_results *results) override;
};
REGISTER(MT_MLM_ANIMAL_POSE_HRNET, ax_model_animal_pose_hrnet)

class ax_model_hand_pose : public ax_model_multi_base_t
{
    AX_NPU_CV_Image pstFrame_RGB = {0};

public:
    void deinit() override;
    int inference(const void *pstFrame, ax_joint_runner_box *crop_resize_box, sample_run_joint_results *results) override;
};
REGISTER(MT_MLM_HAND_POSE, ax_model_hand_pose)

class ax_model_face_recognition : public ax_model_multi_base_t
{
protected:
    bool b_face_database_init = false;
    double _calcSimilar(float *feature1, float *feature2, int feature_len)
    {
        // assert(feature1.size() == feature2.size());
        double sim = 0.0;
        for (int i = 0; i < feature_len; i++)
            sim += feature1[i] * feature2[i];
        sim = sim < 0 ? 0 : sim > 1 ? 1
                                    : sim;
        return sim;
    }

public:
    int inference(const void *pstFrame, ax_joint_runner_box *crop_resize_box, sample_run_joint_results *results) override;
};
REGISTER(MT_MLM_FACE_RECOGNITION, ax_model_face_recognition)

class ax_model_vehicle_license_recognition : public ax_model_multi_base_t
{
public:
    int inference(const void *pstFrame, ax_joint_runner_box *crop_resize_box, sample_run_joint_results *results) override;
};
REGISTER(MT_MLM_VEHICLE_LICENSE_RECOGNITION, ax_model_vehicle_license_recognition)