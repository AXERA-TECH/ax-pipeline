#ifndef _POST_PROCESS_H_
#define _POST_PROCESS_H_

#ifdef __cplusplus
extern "C"
{
#endif

#define SAMPLE_MAX_BBOX_COUNT 64
#define SAMPLE_MAX_FACE_BBOX_COUNT 64
#define SAMPLE_MAX_YOLOV5_MASK_OBJ_COUNT 8
#define SAMPLE_OBJ_NAME_MAX_LEN 20
#define SAMPLE_MAX_HAND_BBOX_COUNT 2
#define SAMPLE_RINGBUFFER_CACHE_COUNT 8
#define SAMPLE_FACE_FEAT_LEN 512
typedef enum _MODEL_TYPE_E
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

} MODEL_TYPE_E;

typedef struct _bbox_t
{
    float x, y, w, h;
} libaxdl_bbox_t;

typedef struct _point_t
{
    float x, y;
} libaxdl_point_t;

typedef struct _mat_t
{
    int w, h;
    unsigned char *data;
} libaxdl_mat_t;

typedef struct _object_t
{
    libaxdl_bbox_t bbox;
    int bHasBoxVertices; // bbox with rotate
    libaxdl_point_t bbox_vertices[4];

    int nLandmark; // num of lmk
#define SAMPLE_PLATE_LMK_SIZE 4
#define SAMPLE_FACE_LMK_SIZE 5
#define SAMPLE_BODY_LMK_SIZE 17
#define SAMPLE_ANIMAL_LMK_SIZE 20
#define SAMPLE_HAND_LMK_SIZE 21
    libaxdl_point_t *landmark;

    int bHasMask;
    libaxdl_mat_t mYolov5Mask; // cv::Mat

    int bHasFaceFeat;
    libaxdl_mat_t mFaceFeat;

    int label;
    float prob;
    char objname[SAMPLE_OBJ_NAME_MAX_LEN];
} libaxdl_object_t;

typedef struct _results_t
{
    int mModelType; // MODEL_TYPE_E
    int nObjSize;
    libaxdl_object_t mObjects[SAMPLE_MAX_BBOX_COUNT];

    int bPPHumSeg;
    libaxdl_mat_t mPPHumSeg;

    int bYolopv2Mask;
    libaxdl_mat_t mYolopv2seg;
    libaxdl_mat_t mYolopv2ll;

    int niFps /*inference*/, noFps /*osd*/;

} libaxdl_results_t;

// typedef struct sample_run_joint_models;

typedef struct _canvas_t
{
    unsigned char *data;
    int width, height, channel;
} libaxdl_canvas_t;


    int libaxdl_parse_param_init(char *json_file_path, void **pModels);
    void libaxdl_deinit(void **pModels);

    int libaxdl_get_ivps_width_height(void *pModels, char *json_file_path, int *width_ivps, int *height_ivps);
    int libaxdl_get_color_space(void *pModels);
    int libaxdl_get_model_type(void *pModels);

    int libaxdl_inference(void *pModels, const void *pstFrame, libaxdl_results_t *pResults);

    int libaxdl_draw_results(void *pModels, libaxdl_canvas_t *canvas, libaxdl_results_t *pResults, float fontscale, int thickness, int offset_x, int offset_y);
#ifdef __cplusplus
}
#endif

#endif