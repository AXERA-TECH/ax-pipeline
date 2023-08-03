#include "sample_run_joint_post_process_pose.h"
#include "base/pose.hpp"

void sample_run_joint_post_process_pose(sample_run_joint_models *pModels, sample_run_joint_object *pObj)
{
    sample_run_joint_attr *pJointAttr = &pModels->mMinor.JointAttr;

    float scale_letterbox;
    int resize_rows;
    int resize_cols;
    int letterbox_rows = pJointAttr->algo_height;
    int letterbox_cols = pJointAttr->algo_width;
    int src_rows = pObj->bbox.h;
    int src_cols = pObj->bbox.w;
    if ((letterbox_rows * 1.0 / src_rows) < (letterbox_cols * 1.0 / src_cols))
    {
        scale_letterbox = letterbox_rows * 1.0 / src_rows;
    }
    else
    {
        scale_letterbox = letterbox_cols * 1.0 / src_cols;
    }
    resize_cols = int(scale_letterbox * src_cols);
    resize_rows = int(scale_letterbox * src_rows);

    int tmp_h = (letterbox_rows - resize_rows) / 2;
    int tmp_w = (letterbox_cols - resize_cols) / 2;

    float ratio_x = (float)src_rows / resize_rows;
    float ratio_y = (float)src_cols / resize_cols;

    pose::ai_body_parts_s ai_point_result;
    pose::ai_hand_parts_s ai_hand_point_result;
    switch (pModels->ModelType_Main)
    {
    case MT_MLM_HUMAN_POSE_AXPPL:
    {
        auto ptr = (float *)pJointAttr->pOutputs[0].pVirAddr;
        auto ptr_index = (float *)pJointAttr->pOutputs[1].pVirAddr;
        pose::ppl_pose_post_process(ptr, ptr_index, ai_point_result, SAMPLE_RUN_JOINT_BODY_LMK_SIZE);
        pObj->nLandmark = SAMPLE_RUN_JOINT_BODY_LMK_SIZE;
        for (size_t i = 0; i < SAMPLE_RUN_JOINT_BODY_LMK_SIZE; i++)
        {
            pObj->landmark[i].x = ai_point_result.keypoints[i].x;
            pObj->landmark[i].y = ai_point_result.keypoints[i].y;
        }
    }
    break;
    case MT_MLM_HUMAN_POSE_HRNET:
    {
        auto ptr = (float *)pJointAttr->pOutputs[0].pVirAddr;
        pose::hrnet_post_process(ptr, ai_point_result, SAMPLE_RUN_JOINT_BODY_LMK_SIZE, pJointAttr->algo_height, pJointAttr->algo_width);
        pObj->nLandmark = SAMPLE_RUN_JOINT_BODY_LMK_SIZE;
        for (size_t i = 0; i < SAMPLE_RUN_JOINT_BODY_LMK_SIZE; i++)
        {
            pObj->landmark[i].x = ai_point_result.keypoints[i].x;
            pObj->landmark[i].y = ai_point_result.keypoints[i].y;
        }
    }
    break;
    case MT_MLM_ANIMAL_POSE_HRNET:
    {
        auto ptr = (float *)pJointAttr->pOutputs[0].pVirAddr;
        pose::hrnet_post_process(ptr, ai_point_result, SAMPLE_RUN_JOINT_ANIMAL_LMK_SIZE, pJointAttr->algo_height, pJointAttr->algo_width);
        pObj->nLandmark = SAMPLE_RUN_JOINT_ANIMAL_LMK_SIZE;
        for (size_t i = 0; i < SAMPLE_RUN_JOINT_ANIMAL_LMK_SIZE; i++)
        {
            pObj->landmark[i].x = (ai_point_result.keypoints[i].x - tmp_w) * ratio_x + pObj->bbox.x;
            pObj->landmark[i].y = (ai_point_result.keypoints[i].y - tmp_h) * ratio_y + pObj->bbox.y;
        }
    }
    break;
    case MT_MLM_HAND_POSE:
    {
        auto &info_point = pJointAttr->pOutputs[0];
        auto &info_score = pJointAttr->pOutputs[1];
        auto point_ptr = (float *)info_point.pVirAddr;
        auto score_ptr = (float *)info_score.pVirAddr;
        pose::post_process_hand(point_ptr, score_ptr, ai_hand_point_result, SAMPLE_RUN_JOINT_HAND_LMK_SIZE, pJointAttr->algo_height, pJointAttr->algo_width);
        pObj->nLandmark = SAMPLE_RUN_JOINT_HAND_LMK_SIZE;
        for (size_t i = 0; i < SAMPLE_RUN_JOINT_HAND_LMK_SIZE; i++)
        {
            pObj->landmark[i].x = ai_hand_point_result.keypoints[i].x;
            pObj->landmark[i].y = ai_hand_point_result.keypoints[i].y;
        }
    }
    break;
    default:
        break;
    }
}