#include "sample_run_joint_post_process_hrnet_pose.h"
#include "pose.hpp"

void sample_run_joint_post_process_hrnet_pose(sample_run_joint_attr *pJointAttr, sample_run_joint_object *pObj)
{
    static const int HRNET_JOINTS = 17;
    auto ptr = (float *)pJointAttr->pOutputs[0].pVirAddr;
    pose::ai_body_parts_s ai_point_result;
    pose::post_process(ptr, ai_point_result, HRNET_JOINTS, pJointAttr->algo_height, pJointAttr->algo_width);

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

    pObj->bHasPoseLmk = 1;

    for (size_t i = 0; i < HRNET_JOINTS; i++)
    {
        pObj->pose_landmark[i].x = ai_point_result.keypoints[i].x;
        pObj->pose_landmark[i].y = ai_point_result.keypoints[i].y;
        pObj->pose_landmark[i].x = (pObj->pose_landmark[i].x - tmp_w) * ratio_x + pObj->bbox.x;
        pObj->pose_landmark[i].y = (pObj->pose_landmark[i].y - tmp_h) * ratio_y + pObj->bbox.y;
    }

}