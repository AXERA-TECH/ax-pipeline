#ifndef _SAMPLE_RUN_JOINT_POST_PROCESS_H_
#define _SAMPLE_RUN_JOINT_POST_PROCESS_H_
#include "sample_run_joint.h"
#define SAMPLE_MAX_BBOX_COUNT 64
#define SAMPLE_MAX_FACE_BBOX_COUNT 64
#define SAMPLE_MAX_POSE_OBJ_COUNT 5
#define SAMPLE_MAX_YOLOV5_MASK_OBJ_COUNT 8
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
    MT_INSEG_YOLOV5_MASK,

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

    int bHasFaceLmk;
#define SAMPLE_RUN_JOINT_FACE_LMK_SIZE 5
    sample_run_joint_point face_landmark[SAMPLE_RUN_JOINT_FACE_LMK_SIZE];

    int bHasPoseLmk;
#define SAMPLE_RUN_JOINT_POSE_LMK_SIZE 17
    sample_run_joint_point pose_landmark[SAMPLE_RUN_JOINT_POSE_LMK_SIZE];

    int bHaseMask;

    void *mYolov5Mask; // cv::Mat

    int label;
    float prob;
    char objname[SAMPLE_OBJ_NAME_MAX_LEN];
} sample_run_joint_object;

typedef struct _sample_run_joint_pphumseg
{
#define SAMPLE_RUN_JOINT_PP_HUM_SEG_SIZE 192 * 192
    unsigned char mask[SAMPLE_RUN_JOINT_PP_HUM_SEG_SIZE];
} sample_run_joint_pphumseg;

typedef struct _sample_run_joint_results
{
    int nObjSize;
    sample_run_joint_object mObjects[SAMPLE_MAX_BBOX_COUNT];

    int bPPHumSeg;
    sample_run_joint_pphumseg mPPHumSeg;

} sample_run_joint_results;

typedef struct _sample_run_joint_model_base
{
    SAMPLE_RUN_JOINT_MODEL_TYPE ModelType;
    void *JointHandle;
    sample_run_joint_attr JointAttr;
} sample_run_joint_model_base;

typedef struct _sample_run_joint_models
{
    AX_BOOL bRunJoint;
    SAMPLE_RUN_JOINT_MODEL_TYPE ModelType_Main;

    sample_run_joint_model_base mMajor, mMinor;

    int SAMPLE_ALGO_FORMAT;
    int SAMPLE_ALGO_WIDTH, SAMPLE_ALGO_HEIGHT;
    int SAMPLE_RESTORE_WIDTH, SAMPLE_RESTORE_HEIGHT;

    char MODEL_PATH[256], MODEL_PATH_L2[256];
    int SAMPLE_IVPS_ALGO_WIDTH, SAMPLE_IVPS_ALGO_HEIGHT;
} sample_run_joint_models;

#include "sample_run_joint_post_process_det.h"
#include "sample_run_joint_post_process_hrnet_pose.h"
#ifdef __cplusplus
extern "C"
{
#endif
    int sample_run_joint_parse_param(char *json_file_path, sample_run_joint_models *pModels);

    int sample_run_joint_inference_single_func(sample_run_joint_models *pModels, const void *pstFrame, sample_run_joint_results *pResults);
#ifdef __cplusplus
}
#endif

#endif