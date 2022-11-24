#ifndef _SAMPLE_RUN_JOINT_POST_PROCESS_DETECTION_H_
#define _SAMPLE_RUN_JOINT_POST_PROCESS_DETECTION_H_

#include "sample_run_joint_post_process.h"

#ifdef __cplusplus
extern "C"
{
#endif

    int sample_parse_param_det(char *json_file_path);

    /// @brief
    /// @param json_obj nlohmann::json
    /// @return
    int sample_set_param_det(void *json_obj);

    void sample_run_joint_post_process_detection(SAMPLE_RUN_JOINT_MODEL_TYPE modeltype, sample_run_joint_attr *pJointAttr, sample_run_joint_results *pResults,
                                                 int SAMPLE_ALGO_WIDTH, int SAMPLE_ALGO_HEIGHT, int SAMPLE_MAJOR_STREAM_WIDTH, int SAMPLE_MAJOR_STREAM_HEIGHT);

    void sample_run_joint_post_process_yolov5_seg(SAMPLE_RUN_JOINT_MODEL_TYPE modeltype, sample_run_joint_attr *pJointAttr, sample_run_joint_results *pResults,
                                                  int SAMPLE_ALGO_WIDTH, int SAMPLE_ALGO_HEIGHT, int SAMPLE_MAJOR_STREAM_WIDTH, int SAMPLE_MAJOR_STREAM_HEIGHT);

    void sample_run_joint_post_process_det_single_func(sample_run_joint_results *pResults, sample_run_joint_models *pModels);

#ifdef __cplusplus
}
#endif

#endif