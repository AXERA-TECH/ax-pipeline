#ifndef _SAMPLE_RUN_JOINT_POST_PROCESS_DETECTION_H_
#define _SAMPLE_RUN_JOINT_POST_PROCESS_DETECTION_H_

#include "sample_run_joint_post_process.h"

#ifdef __cplusplus
extern "C"
{
#endif

    int sample_parse_param_det(char *json_file_path);

    void sample_run_joint_post_process_detection(SAMPLE_RUN_JOINT_MODEL_TYPE modeltype, AX_U32 nOutputSize, AX_JOINT_IOMETA_T *pOutputsInfo, AX_JOINT_IO_BUFFER_T *pOutputs, sample_run_joint_results *pResults,
                                                 int SAMPLE_ALGO_WIDTH, int SAMPLE_ALGO_HEIGHT, int SAMPLE_MAJOR_STREAM_WIDTH, int SAMPLE_MAJOR_STREAM_HEIGHT);

#ifdef __cplusplus
}
#endif

#endif