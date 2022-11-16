#ifndef _SAMPLE_RUN_JOINT_POST_PROCESS_H_
#define _SAMPLE_RUN_JOINT_POST_PROCESS_H_
#include "sample_run_joint.h"

#define SAMPLE_MAX_BBOX_COUNT 64
#define SAMPLE_OBJ_NAME_MAX_LEN 16

typedef struct _sample_run_joint_bbox
{
    float x, y, w, h;
} sample_run_joint_bbox;

typedef struct _sample_run_joint_point
{
    float x, y;
} sample_run_joint_point;

typedef struct _sample_run_joint_object
{
    sample_run_joint_bbox bbox;
    int label;
    float prob;
    char objname[SAMPLE_OBJ_NAME_MAX_LEN];
} sample_run_joint_object;

typedef struct _sample_run_joint_results
{
    int size;
    sample_run_joint_object objects[SAMPLE_MAX_BBOX_COUNT];
} sample_run_joint_results;

#ifdef __cplusplus
extern "C"
{
#endif
    /// @brief 通过 json 解析 yolov5 所需的参数，如果某些 key 置空，则不更新该参数
    /// @param json_file_path
    /// @return
    int sample_parse_param_yolov5(char *json_file_path);
    
    void sample_run_joint_post_process_yolov5(AX_U32 nOutputSize, AX_JOINT_IOMETA_T *pOutputsInfo, AX_JOINT_IO_BUFFER_T *pOutputs, sample_run_joint_results *pResults,
                                       int SAMPLE_ALGO_WIDTH, int SAMPLE_ALGO_HEIGHT, int SAMPLE_MAJOR_STREAM_WIDTH, int SAMPLE_MAJOR_STREAM_HEIGHT);
#ifdef __cplusplus
}
#endif

#endif