#include <string.h>
#include "fstream"

#include "sample_run_joint_post_process.h"
#include "sample_run_joint_post_process_det.h"
#include "detection.hpp"

#include "../utilities/json.hpp"

#include "joint.h"
#include "../utilities/sample_log.h"
#include "../utilities/ringbuffer.hpp"

static float PROB_THRESHOLD = 0.4f;
static float NMS_THRESHOLD = 0.45f;
static int CLASS_NUM = 80;

static std::vector<float> ANCHORS = {12, 16, 19, 36, 40, 28,
                                     36, 75, 76, 55, 72, 146,
                                     142, 110, 192, 243, 459, 401};

static std::vector<std::string> CLASS_NAMES = {
    "person", "bicycle", "car", "motorcycle", "airplane", "bus", "train", "truck", "boat", "traffic light",
    "fire hydrant", "stop sign", "parking meter", "bench", "bird", "cat", "dog", "horse", "sheep", "cow",
    "elephant", "bear", "zebra", "giraffe", "backpack", "umbrella", "handbag", "tie", "suitcase", "frisbee",
    "skis", "snowboard", "sports ball", "kite", "baseball bat", "baseball glove", "skateboard", "surfboard",
    "tennis racket", "bottle", "wine glass", "cup", "fork", "knife", "spoon", "bowl", "banana", "apple",
    "sandwich", "orange", "broccoli", "carrot", "hot dog", "pizza", "donut", "cake", "chair", "couch",
    "potted plant", "bed", "dining table", "toilet", "tv", "laptop", "mouse", "remote", "keyboard", "cell phone",
    "microwave", "oven", "toaster", "sink", "refrigerator", "book", "clock", "vase", "scissors", "teddy bear",
    "hair drier", "toothbrush"};

template <typename T>
void update_val(nlohmann::json &jsondata, const char *key, T *val)
{
    if (jsondata.contains(key))
    {
        *val = jsondata[key];
    }
}

template <typename T>
void update_val(nlohmann::json &jsondata, const char *key, std::vector<T> *val)
{
    if (jsondata.contains(key))
    {
        std::vector<T> tmp = jsondata[key];
        *val = tmp;
    }
}

int sample_get_model_type(char *json_file_path)
{
    std::ifstream f(json_file_path);
    if (f.fail())
    {
        return MT_UNKNOWN;
    }
    auto jsondata = nlohmann::json::parse(f);
    int mt = -1;
    update_val(jsondata, "MODEL_TYPE", &mt);
    return mt;
}

int sample_parse_param_det(char *json_file_path)
{
    std::ifstream f(json_file_path);
    if (f.fail())
    {
        ALOGE("%s doesn`t exist,generate it by default param\n", json_file_path);
        nlohmann::json json_data;
        json_data["MODEL_TYPE"] = MT_DET_YOLOV5;
        json_data["PROB_THRESHOLD"] = PROB_THRESHOLD;
        json_data["NMS_THRESHOLD"] = NMS_THRESHOLD;
        json_data["CLASS_NUM"] = CLASS_NUM;
        json_data["ANCHORS"] = ANCHORS;
        json_data["CLASS_NAMES"] = CLASS_NAMES;

        std::string json_ctx = json_data.dump(4);
        std::ofstream of(json_file_path);
        of << json_ctx;
        of.close();
        return -1;
    }

    auto jsondata = nlohmann::json::parse(f);

    update_val(jsondata, "PROB_THRESHOLD", &PROB_THRESHOLD);
    update_val(jsondata, "NMS_THRESHOLD", &NMS_THRESHOLD);
    update_val(jsondata, "CLASS_NUM", &CLASS_NUM);
    update_val(jsondata, "ANCHORS", &ANCHORS);
    update_val(jsondata, "CLASS_NAMES", &CLASS_NAMES);

    if (ANCHORS.size() != 18)
    {
        ALOGE("ANCHORS SIZE MUST BE 18\n");
        return -1;
    }

    if (CLASS_NUM != CLASS_NAMES.size())
    {
        ALOGE("CLASS_NUM != CLASS_NAMES SIZE(%d:%d)\n", CLASS_NUM, CLASS_NAMES.size());
        return -1;
    }
    return 0;
}

int sample_set_param_det(void *json_obj)
{
    auto jsondata = *((nlohmann::json *)json_obj);
    update_val(jsondata, "PROB_THRESHOLD", &PROB_THRESHOLD);
    update_val(jsondata, "NMS_THRESHOLD", &NMS_THRESHOLD);
    update_val(jsondata, "CLASS_NUM", &CLASS_NUM);
    update_val(jsondata, "ANCHORS", &ANCHORS);
    update_val(jsondata, "CLASS_NAMES", &CLASS_NAMES);

    if (ANCHORS.size() != 18)
    {
        ALOGE("ANCHORS SIZE MUST BE 18\n");
        return -1;
    }

    if (CLASS_NUM != CLASS_NAMES.size())
    {
        ALOGE("CLASS_NUM != CLASS_NAMES SIZE(%d:%d)\n", CLASS_NUM, CLASS_NAMES.size());
        return -1;
    }
    return 0;
}



void sample_run_joint_post_process_detection(SAMPLE_RUN_JOINT_MODEL_TYPE modeltype, sample_run_joint_attr *pJointAttr, sample_run_joint_results *pResults,
                                             int SAMPLE_ALGO_WIDTH, int SAMPLE_ALGO_HEIGHT, int SAMPLE_MAJOR_STREAM_WIDTH, int SAMPLE_MAJOR_STREAM_HEIGHT)
{
    std::vector<detection::Object> proposals;
    std::vector<detection::Object> objects;
    AX_U32 nOutputSize = pJointAttr->nOutputSize;
    AX_JOINT_IOMETA_T *pOutputsInfo = pJointAttr->pOutputsInfo;
    AX_JOINT_IO_BUFFER_T *pOutputs = pJointAttr->pOutputs;

    float prob_threshold_unsigmoid = -1.0f * (float)std::log((1.0f / PROB_THRESHOLD) - 1.0f);
    for (uint32_t i = 0; i < nOutputSize; ++i)
    {
        auto &output = pOutputsInfo[i];
        auto &info = pOutputs[i];
        auto ptr = (float *)info.pVirAddr;
        int32_t stride = (1 << i) * 8;
        switch (modeltype)
        {
        case MT_DET_YOLOV5:
            generate_proposals_yolov5(stride, ptr, PROB_THRESHOLD, proposals, SAMPLE_ALGO_WIDTH, SAMPLE_ALGO_HEIGHT, ANCHORS.data(), prob_threshold_unsigmoid, CLASS_NUM);
            break;
        case MT_DET_YOLOV5_FACE:
            generate_proposals_yolov5_face(stride, ptr, PROB_THRESHOLD, proposals, SAMPLE_ALGO_WIDTH, SAMPLE_ALGO_HEIGHT, ANCHORS.data(), prob_threshold_unsigmoid);
            break;
        case MT_DET_YOLOV7:
            generate_proposals_yolov7(stride, ptr, PROB_THRESHOLD, proposals, SAMPLE_ALGO_WIDTH, SAMPLE_ALGO_HEIGHT, ANCHORS.data() + i * 6, CLASS_NUM);
            break;
        case MT_DET_YOLOX:
            generate_proposals_yolox(stride, ptr, PROB_THRESHOLD, proposals, SAMPLE_ALGO_WIDTH, SAMPLE_ALGO_HEIGHT, CLASS_NUM);
            break;
        case MT_DET_NANODET:
        {
            static const int DEFAULT_STRIDES[] = {32, 16, 8};
            generate_proposals_nanodet(ptr, DEFAULT_STRIDES[i], SAMPLE_ALGO_WIDTH, SAMPLE_ALGO_HEIGHT, PROB_THRESHOLD, proposals, CLASS_NUM);
        }
        break;
        default:
            break;
        }
    }

    detection::get_out_bbox(proposals, objects, NMS_THRESHOLD, SAMPLE_ALGO_HEIGHT, SAMPLE_ALGO_WIDTH, SAMPLE_MAJOR_STREAM_HEIGHT, SAMPLE_MAJOR_STREAM_WIDTH);

    pResults->nObjSize = MIN(objects.size(), SAMPLE_MAX_BBOX_COUNT);
    for (size_t i = 0; i < pResults->nObjSize; i++)
    {
        const detection::Object &obj = objects[i];
        pResults->mObjects[i].bbox.x = obj.rect.x;
        pResults->mObjects[i].bbox.y = obj.rect.y;
        pResults->mObjects[i].bbox.w = obj.rect.width;
        pResults->mObjects[i].bbox.h = obj.rect.height;
        pResults->mObjects[i].label = obj.label;
        pResults->mObjects[i].prob = obj.prob;

        pResults->mObjects[i].bHasFaceLmk = pResults->mObjects[i].bHaseMask = pResults->mObjects[i].bHasPoseLmk = 0;
        if (modeltype == MT_DET_YOLOV5_FACE)
        {
            for (size_t j = 0; j < SAMPLE_RUN_JOINT_FACE_LMK_SIZE; j++)
            {
                pResults->mObjects[i].face_landmark[j].x = obj.landmark[j].x;
                pResults->mObjects[i].face_landmark[j].y = obj.landmark[j].y;
                pResults->mObjects[i].bHasFaceLmk = 1;
            }
        }

        if (obj.label < CLASS_NAMES.size())
        {
            strcpy(pResults->mObjects[i].objname, CLASS_NAMES[obj.label].c_str());
        }
        else
        {
            strcpy(pResults->mObjects[i].objname, "unknown");
        }
    }
}

void sample_run_joint_post_process_yolov5_seg(SAMPLE_RUN_JOINT_MODEL_TYPE modeltype, sample_run_joint_attr *pJointAttr, sample_run_joint_results *pResults,
                                              int SAMPLE_ALGO_WIDTH, int SAMPLE_ALGO_HEIGHT, int SAMPLE_MAJOR_STREAM_WIDTH, int SAMPLE_MAJOR_STREAM_HEIGHT)
{
    std::vector<detection::Object> proposals;
    std::vector<detection::Object> objects;
    AX_U32 nOutputSize = pJointAttr->nOutputSize;
    AX_JOINT_IOMETA_T *pOutputsInfo = pJointAttr->pOutputsInfo;
    AX_JOINT_IO_BUFFER_T *pOutputs = pJointAttr->pOutputs;

    float prob_threshold_unsigmoid = -1.0f * (float)std::log((1.0f / PROB_THRESHOLD) - 1.0f);
    for (uint32_t i = 0; i < nOutputSize - 1; ++i)
    {
        auto &output = pOutputsInfo[i];
        auto &info = pOutputs[i];
        auto ptr = (float *)info.pVirAddr;
        int32_t stride = (1 << i) * 8;
        generate_proposals_yolov5_seg(stride, ptr, PROB_THRESHOLD, proposals, SAMPLE_ALGO_WIDTH, SAMPLE_ALGO_HEIGHT, ANCHORS.data(), prob_threshold_unsigmoid);
    }
    static const int DEFAULT_MASK_PROTO_DIM = 32;
    static const int DEFAULT_MASK_SAMPLE_STRIDE = 4;
    auto &output = pOutputsInfo[3];
    auto &info = pOutputs[3];
    auto ptr = (float *)info.pVirAddr;
    detection::get_out_bbox_mask(proposals, objects, SAMPLE_MAX_YOLOV5_MASK_OBJ_COUNT, ptr, DEFAULT_MASK_PROTO_DIM, DEFAULT_MASK_SAMPLE_STRIDE, NMS_THRESHOLD,
                                 SAMPLE_ALGO_HEIGHT, SAMPLE_ALGO_WIDTH, SAMPLE_MAJOR_STREAM_HEIGHT, SAMPLE_MAJOR_STREAM_WIDTH);

    static SimpleRingBuffer<cv::Mat> mSimpleRingBuffer(64);
    pResults->nObjSize = MIN(objects.size(), SAMPLE_MAX_BBOX_COUNT);
    for (size_t i = 0; i < pResults->nObjSize; i++)
    {
        const detection::Object &obj = objects[i];
        pResults->mObjects[i].bbox.x = obj.rect.x;
        pResults->mObjects[i].bbox.y = obj.rect.y;
        pResults->mObjects[i].bbox.w = obj.rect.width;
        pResults->mObjects[i].bbox.h = obj.rect.height;
        pResults->mObjects[i].label = obj.label;
        pResults->mObjects[i].prob = obj.prob;

        pResults->mObjects[i].bHasFaceLmk = pResults->mObjects[i].bHaseMask = pResults->mObjects[i].bHasPoseLmk = 0;

        pResults->mObjects[i].bHaseMask = !obj.mask.empty();

        if (pResults->mObjects[i].bHaseMask)
        {
            cv::Mat&mask = mSimpleRingBuffer.next();
            mask = obj.mask;
            pResults->mObjects[i].mYolov5Mask = &mask;
        }

        if (obj.label < CLASS_NAMES.size())
        {
            strcpy(pResults->mObjects[i].objname, CLASS_NAMES[obj.label].c_str());
        }
        else
        {
            strcpy(pResults->mObjects[i].objname, "unknown");
        }
    }
}
