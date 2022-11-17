#ifndef _SAMPLE_RUN_JOINT_POST_PROCESS_H_
#define _SAMPLE_RUN_JOINT_POST_PROCESS_H_
#include "sample_run_joint.h"
#define SAMPLE_MAX_BBOX_COUNT 64
#define SAMPLE_OBJ_NAME_MAX_LEN 16

typedef enum __SAMPLE_RUN_JOINT_MODEL_TYPE
{
    MT_UNKNOWN = -1,

    // detection
    MT_DET = 0x10,
    MT_DET_YOLOV5,
    MT_DET_YOLOV5_FACE,
    MT_DET_YOLOV7,
    MT_DET_YOLOX,
    MT_DET_NANODET,

    // segmentation
    MT_SEG = 0x20,
    MT_SEG_PPHUMSEG,

    // instance segmentation
    MT_INSEG = 0x30,

    // multi level model
    MT_MLM = 0x40,
    MT_MLM_HUMAN_POSE,
    MT_MLM_HAND_POSE,
    MT_MLM_FACE_RECOGNITION,
    MT_MLM_VEHICLE_LICENSE_RECOGNITION,

} SAMPLE_RUN_JOINT_MODEL_TYPE;

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

#include "sample_run_joint_post_process_detection.h"
#ifdef __cplusplus
extern "C"
{
#endif
    int sample_get_model_type(char *json_file_path);
#ifdef __cplusplus
}
#endif

#endif