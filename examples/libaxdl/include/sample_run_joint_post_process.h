#ifndef _SAMPLE_RUN_JOINT_POST_PROCESS_H_
#define _SAMPLE_RUN_JOINT_POST_PROCESS_H_

#define SAMPLE_MAX_BBOX_COUNT 64
#define SAMPLE_MAX_FACE_BBOX_COUNT 64
// #define SAMPLE_MAX_POSE_OBJ_COUNT 5
#define SAMPLE_MAX_YOLOV5_MASK_OBJ_COUNT 8
#define SAMPLE_OBJ_NAME_MAX_LEN 20
#define SAMPLE_MAX_HAND_BBOX_COUNT 2
#define SAMPLE_RINGBUFFER_CACHE_COUNT 8
// #define SAMPLE_CLASS_ID_COUNT 5
#define SAMPLE_FACE_FEAT_LEN 512
typedef enum __SAMPLE_RUN_JOINT_MODEL_TYPE
{
    MT_UNKNOWN = 0,

    // detection
    MT_DET = 0x10000,
    MT_DET_YOLOV5,
    MT_DET_YOLOV5_FACE,
    MT_DET_YOLOV5_LICENSE_PLATE,
    MT_DET_YOLOV6,
    MT_DET_YOLOV7,
    MT_DET_YOLOV7_FACE,
    MT_DET_YOLOV7_PALM_HAND,
    MT_DET_YOLOX,
    MT_DET_NANODET,
    MT_DET_YOLOX_PPL,
    MT_DET_PALM_HAND,
    MT_DET_YOLOPV2,
    MT_DET_YOLO_FASTBODY,

    // segmentation
    MT_SEG = 0x20000,
    MT_SEG_PPHUMSEG,

    // instance segmentation
    MT_INSEG = 0x30000,
    MT_INSEG_YOLOV5_MASK,

    // multi level model
    MT_MLM = 0x40000,
    MT_MLM_HUMAN_POSE_AXPPL,
    MT_MLM_HUMAN_POSE_HRNET,
    MT_MLM_ANIMAL_POSE_HRNET,
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

typedef struct _sample_run_joint_mat
{
    int w, h;
    unsigned char *data;
} sample_run_joint_mat;

typedef struct _sample_run_joint_object
{
    sample_run_joint_bbox bbox;
    int bHasBoxVertices; // bbox with rotate
    sample_run_joint_point bbox_vertices[4];

    int nLandmark; // num of lmk
#define SAMPLE_RUN_JOINT_PLATE_LMK_SIZE 4
#define SAMPLE_RUN_JOINT_FACE_LMK_SIZE 5
#define SAMPLE_RUN_JOINT_BODY_LMK_SIZE 17
#define SAMPLE_RUN_JOINT_ANIMAL_LMK_SIZE 20
#define SAMPLE_RUN_JOINT_HAND_LMK_SIZE 21
    sample_run_joint_point *landmark;

    int bHasMask;
    sample_run_joint_mat mYolov5Mask; // cv::Mat

    int bHasFaceFeat;
    sample_run_joint_mat mFaceFeat;

    int label;
    float prob;
    char objname[SAMPLE_OBJ_NAME_MAX_LEN];
} sample_run_joint_object;

typedef struct _sample_run_joint_results
{
    int mModelType; // SAMPLE_RUN_JOINT_MODEL_TYPE
    int nObjSize;
    sample_run_joint_object mObjects[SAMPLE_MAX_BBOX_COUNT];

    int bPPHumSeg;
    sample_run_joint_mat mPPHumSeg;

    int bYolopv2Mask;
    sample_run_joint_mat mYolopv2seg;
    sample_run_joint_mat mYolopv2ll;

    int niFps /*inference*/, noFps /*osd*/;

} sample_run_joint_results;

// typedef struct sample_run_joint_models;

#ifdef __cplusplus
extern "C"
{
#endif
    int sample_run_joint_parse_param_init(char *json_file_path, void **pModels);
    void sample_run_joint_deinit(void **pModels);

    int get_ivps_width_height(void *pModels, char *json_file_path, int *width_ivps, int *height_ivps);
    int get_color_space(void *pModels);
    int get_model_type(void *pModels);

    int sample_run_joint_inference_single_func(void *pModels, const void *pstFrame, sample_run_joint_results *pResults);
#ifdef __cplusplus
}
#endif

#endif