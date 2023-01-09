#include "../include/ax_model_base.hpp"
#include "utilities/object_register.hpp"
#include "../../utilities/ringbuffer.hpp"
#include "opencv2/opencv.hpp"

class ax_model_ml_sub : public ax_model_single_base_t
{
protected:
    cv::Mat affine_trans_mat;
    cv::Mat affine_trans_mat_inv;
    SimpleRingBuffer<std::vector<sample_run_joint_point>> mSimpleRingBuffer;
};

class ax_model_pose_hrnet_sub : public ax_model_ml_sub
{
protected:
    int preprocess(const void *srcFrame, ax_joint_runner_box *crop_resize_box, sample_run_joint_results *results) override;
    int post_process(const void *pstFrame, ax_joint_runner_box *crop_resize_box, sample_run_joint_results *results) override;
};

class ax_model_pose_axppl_sub : public ax_model_pose_hrnet_sub
{
protected:
    int post_process(const void *pstFrame, ax_joint_runner_box *crop_resize_box, sample_run_joint_results *results) override;
};

class ax_model_pose_hrnet_animal_sub : public ax_model_ml_sub
{
protected:
    // int preprocess(const void *srcFrame, ax_joint_runner_box *crop_resize_box, sample_run_joint_results *results) override;
    int post_process(const void *pstFrame, ax_joint_runner_box *crop_resize_box, sample_run_joint_results *results) override;
};

class ax_model_pose_hand_sub : public ax_model_ml_sub
{
protected:
    int preprocess(const void *srcFrame, ax_joint_runner_box *crop_resize_box, sample_run_joint_results *results) override;
    int post_process(const void *pstFrame, ax_joint_runner_box *crop_resize_box, sample_run_joint_results *results) override;
};

class ax_model_face_feat_extactor_sub : public ax_model_ml_sub
{
protected:
    SimpleRingBuffer<std::vector<float>> mSimpleRingBuffer_FaceFeat;
    void _normalize(float *feature, int feature_len);
    int preprocess(const void *srcFrame, ax_joint_runner_box *crop_resize_box, sample_run_joint_results *results) override;
    int post_process(const void *pstFrame, ax_joint_runner_box *crop_resize_box, sample_run_joint_results *results) override;
};

class ax_model_license_plate_recognition_sub : public ax_model_ml_sub
{
protected:
    float argmax_data[21];
    float argmax_idx[21];
    int preprocess(const void *srcFrame, ax_joint_runner_box *crop_resize_box, sample_run_joint_results *results) override;
    int post_process(const void *pstFrame, ax_joint_runner_box *crop_resize_box, sample_run_joint_results *results) override;
};

// REGISTER(MT_MLM_HUMAN_POSE_HRNET, ax_model_hrnet)