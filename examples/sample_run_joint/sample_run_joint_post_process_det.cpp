#include <string.h>
#include "fstream"

#include "sample_run_joint_post_process.h"
#include "sample_run_joint_post_process_det.h"
#include "base/detection.hpp"
#include "base/yolo.hpp"

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

void sample_run_joint_post_process_detection(sample_run_joint_results *pResults, sample_run_joint_models *pModels)
{
    std::vector<detection::Object> proposals;
    std::vector<detection::Object> objects;
    AX_U32 nOutputSize = pModels->mMajor.JointAttr.nOutputSize;
    AX_JOINT_IOMETA_T *pOutputsInfo = pModels->mMajor.JointAttr.pOutputsInfo;
    AX_JOINT_IO_BUFFER_T *pOutputs = pModels->mMajor.JointAttr.pOutputs;

    float prob_threshold_unsigmoid = -1.0f * (float)std::log((1.0f / PROB_THRESHOLD) - 1.0f);
    for (uint32_t i = 0; i < nOutputSize; ++i)
    {
        auto &output = pOutputsInfo[i];
        auto &info = pOutputs[i];
        auto ptr = (float *)info.pVirAddr;
        int32_t stride = (1 << i) * 8;
        switch (pModels->mMajor.ModelType)
        {
        case MT_DET_YOLOV5:
            generate_proposals_yolov5(stride, ptr, PROB_THRESHOLD, proposals, pModels->mMajor.JointAttr.algo_width, pModels->mMajor.JointAttr.algo_height, ANCHORS.data(), prob_threshold_unsigmoid, CLASS_NUM);
            break;
        case MT_DET_YOLOV5_FACE:
            generate_proposals_yolov5_face(stride, ptr, PROB_THRESHOLD, proposals, pModels->mMajor.JointAttr.algo_width, pModels->mMajor.JointAttr.algo_height, ANCHORS.data(), prob_threshold_unsigmoid);
            break;
        case MT_DET_YOLOV7:
            generate_proposals_yolov7(stride, ptr, PROB_THRESHOLD, proposals, pModels->mMajor.JointAttr.algo_width, pModels->mMajor.JointAttr.algo_height, ANCHORS.data() + i * 6, CLASS_NUM);
            break;
        case MT_DET_YOLOX:
            generate_proposals_yolox(stride, ptr, PROB_THRESHOLD, proposals, pModels->mMajor.JointAttr.algo_width, pModels->mMajor.JointAttr.algo_height, CLASS_NUM);
            break;
        case MT_DET_NANODET:
        {
            static const int DEFAULT_STRIDES[] = {32, 16, 8};
            generate_proposals_nanodet(ptr, DEFAULT_STRIDES[i], pModels->mMajor.JointAttr.algo_width, pModels->mMajor.JointAttr.algo_height, PROB_THRESHOLD, proposals, CLASS_NUM);
        }
        break;
        case MT_DET_YOLOX_PPL:
        {
            std::vector<detection::GridAndStride> grid_stride;
            int wxc = output.pShape[2] * output.pShape[3];
            static std::vector<std::vector<int>> stride_ppl = {{8}, {16}, {32}};
            generate_grids_and_stride(pModels->mMajor.JointAttr.algo_width, pModels->mMajor.JointAttr.algo_height, stride_ppl[i], grid_stride);
            generate_yolox_proposals(grid_stride, ptr, PROB_THRESHOLD, proposals, wxc);
        }
        break;
        default:
            break;
        }
    }

    detection::get_out_bbox(proposals, objects, NMS_THRESHOLD, pModels->mMajor.JointAttr.algo_height, pModels->mMajor.JointAttr.algo_width, pModels->SAMPLE_RESTORE_HEIGHT, pModels->SAMPLE_RESTORE_WIDTH);
    std::sort(objects.begin(), objects.end(),
              [&](detection::Object &a, detection::Object &b)
              {
                  return a.rect.area() > b.rect.area();
              });

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

        pResults->mObjects[i].bHasFaceLmk = pResults->mObjects[i].bHaseMask = pResults->mObjects[i].bHasBodyLmk = pResults->mObjects[i].bHasHandLmk = 0;
        if (pModels->mMajor.ModelType == MT_DET_YOLOV5_FACE)
        {
            pResults->mObjects[i].bHasFaceLmk = 1;
            for (size_t j = 0; j < SAMPLE_RUN_JOINT_FACE_LMK_SIZE; j++)
            {
                pResults->mObjects[i].landmark[j].x = obj.landmark[j].x;
                pResults->mObjects[i].landmark[j].y = obj.landmark[j].y;
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

void sample_run_joint_post_process_yolov5_seg(sample_run_joint_results *pResults, sample_run_joint_models *pModels)
{
    std::vector<detection::Object> proposals;
    std::vector<detection::Object> objects;
    AX_U32 nOutputSize = pModels->mMajor.JointAttr.nOutputSize;
    AX_JOINT_IOMETA_T *pOutputsInfo = pModels->mMajor.JointAttr.pOutputsInfo;
    AX_JOINT_IO_BUFFER_T *pOutputs = pModels->mMajor.JointAttr.pOutputs;

    float prob_threshold_unsigmoid = -1.0f * (float)std::log((1.0f / PROB_THRESHOLD) - 1.0f);
    for (uint32_t i = 0; i < nOutputSize - 1; ++i)
    {
        auto &output = pOutputsInfo[i];
        auto &info = pOutputs[i];
        auto ptr = (float *)info.pVirAddr;
        int32_t stride = (1 << i) * 8;
        generate_proposals_yolov5_seg(stride, ptr, PROB_THRESHOLD, proposals, pModels->mMajor.JointAttr.algo_width, pModels->mMajor.JointAttr.algo_height, ANCHORS.data(), prob_threshold_unsigmoid);
    }
    static const int DEFAULT_MASK_PROTO_DIM = 32;
    static const int DEFAULT_MASK_SAMPLE_STRIDE = 4;
    auto &output = pOutputsInfo[3];
    auto &info = pOutputs[3];
    auto ptr = (float *)info.pVirAddr;
    detection::get_out_bbox_mask(proposals, objects, SAMPLE_MAX_YOLOV5_MASK_OBJ_COUNT, ptr, DEFAULT_MASK_PROTO_DIM, DEFAULT_MASK_SAMPLE_STRIDE, NMS_THRESHOLD,
                                 pModels->mMajor.JointAttr.algo_height, pModels->mMajor.JointAttr.algo_width, pModels->SAMPLE_RESTORE_HEIGHT, pModels->SAMPLE_RESTORE_WIDTH);

    std::sort(objects.begin(), objects.end(),
              [&](detection::Object &a, detection::Object &b)
              {
                  return a.rect.area() > b.rect.area();
              });

    static SimpleRingBuffer<cv::Mat> mSimpleRingBuffer(SAMPLE_MAX_YOLOV5_MASK_OBJ_COUNT * 6);
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

        pResults->mObjects[i].bHasFaceLmk = pResults->mObjects[i].bHaseMask = pResults->mObjects[i].bHasBodyLmk = pResults->mObjects[i].bHasHandLmk = 0;

        pResults->mObjects[i].bHaseMask = !obj.mask.empty();

        if (pResults->mObjects[i].bHaseMask)
        {
            cv::Mat &mask = mSimpleRingBuffer.next();
            mask = obj.mask;
            pResults->mObjects[i].mYolov5Mask.data = mask.data;
            pResults->mObjects[i].mYolov5Mask.w = mask.cols;
            pResults->mObjects[i].mYolov5Mask.h = mask.rows;
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

void sample_run_joint_post_process_palm_hand(sample_run_joint_results *pResults, sample_run_joint_models *pModels)
{
    static const int map_size[2] = {24, 12};
    static const int strides[2] = {8, 16};
    static const int anchor_size[2] = {2, 6};
    static const float anchor_offset[2] = {0.5f, 0.5f};
    std::vector<detection::PalmObject> proposals;
    std::vector<detection::PalmObject> objects;

    auto &bboxes_info = pModels->mMajor.JointAttr.pOutputs[0];
    auto bboxes_ptr = (float *)bboxes_info.pVirAddr;
    auto &scores_info = pModels->mMajor.JointAttr.pOutputs[1];
    auto scores_ptr = (float *)scores_info.pVirAddr;
    float prob_threshold_unsigmoid = -1.0f * (float)std::log((1.0f / PROB_THRESHOLD) - 1.0f);
    detection::generate_proposals_palm(proposals, PROB_THRESHOLD, pModels->mMajor.JointAttr.algo_width, pModels->mMajor.JointAttr.algo_height, scores_ptr, bboxes_ptr, 2, strides, anchor_size, anchor_offset, map_size, prob_threshold_unsigmoid);

    detection::get_out_bbox_palm(proposals, objects, NMS_THRESHOLD, pModels->mMajor.JointAttr.algo_height, pModels->mMajor.JointAttr.algo_width, pModels->SAMPLE_RESTORE_HEIGHT, pModels->SAMPLE_RESTORE_WIDTH);

    std::sort(objects.begin(), objects.end(),
              [&](detection::PalmObject &a, detection::PalmObject &b)
              {
                  return a.rect.area() > b.rect.area();
              });

    pResults->nObjSize = MIN(objects.size(), SAMPLE_MAX_HAND_BBOX_COUNT);
    for (size_t i = 0; i < pResults->nObjSize; i++)
    {
        const detection::PalmObject &obj = objects[i];
        pResults->mObjects[i].bbox.x = obj.rect.x * pModels->SAMPLE_RESTORE_WIDTH;
        pResults->mObjects[i].bbox.y = obj.rect.y * pModels->SAMPLE_RESTORE_HEIGHT;
        pResults->mObjects[i].bbox.w = obj.rect.width * pModels->SAMPLE_RESTORE_WIDTH;
        pResults->mObjects[i].bbox.h = obj.rect.height * pModels->SAMPLE_RESTORE_HEIGHT;
        pResults->mObjects[i].label = 0;
        pResults->mObjects[i].prob = obj.prob;
        pResults->mObjects[i].bHasBoxVertices = 1;
        for (size_t j = 0; j < 4; j++)
        {
            pResults->mObjects[i].bbox_vertices[j].x = obj.vertices[j].x;
            pResults->mObjects[i].bbox_vertices[j].y = obj.vertices[j].y;
        }

        pResults->mObjects[i].bHasFaceLmk = pResults->mObjects[i].bHaseMask = pResults->mObjects[i].bHasBodyLmk = pResults->mObjects[i].bHasHandLmk = 0;

        strcpy(pResults->mObjects[i].objname, "hand");
    }
}

void sample_run_joint_post_process_det_single_func(sample_run_joint_results *pResults, sample_run_joint_models *pModels)
{
    typedef void (*post_process_func)(sample_run_joint_results * pResults, sample_run_joint_models * pModels);
    static std::map<int, post_process_func> m_func_map{
        {MT_DET_YOLOV5, sample_run_joint_post_process_detection},
        {MT_DET_YOLOV5_FACE, sample_run_joint_post_process_detection},
        {MT_DET_YOLOV7, sample_run_joint_post_process_detection},
        {MT_DET_YOLOX, sample_run_joint_post_process_detection},
        {MT_DET_NANODET, sample_run_joint_post_process_detection},
        {MT_DET_YOLOX_PPL, sample_run_joint_post_process_detection},

        {MT_INSEG_YOLOV5_MASK, sample_run_joint_post_process_yolov5_seg},

        {MT_DET_PALM_HAND, sample_run_joint_post_process_palm_hand},
    };

    auto item = m_func_map.find(pModels->mMajor.ModelType);

    if (item != m_func_map.end())
    {
        item->second(pResults, pModels);
    }
    else
    {
        ALOGE("cannot find process func for modeltype %d", pModels->mMajor.ModelType);
    }
    
    
    switch (pModels->ModelType_Main)
    {
    case MT_MLM_HUMAN_POSE_AXPPL:
    case MT_MLM_HUMAN_POSE_HRNET:
    case MT_MLM_ANIMAL_POSE_HRNET:
    case MT_MLM_HAND_POSE:
    case MT_MLM_FACE_RECOGNITION:
    case MT_MLM_VEHICLE_LICENSE_RECOGNITION:
        break;
    default:
        for (AX_U8 i = 0; i < pResults->nObjSize; i++)
        {
            pResults->mObjects[i].bbox.x /= pModels->SAMPLE_RESTORE_WIDTH;
            pResults->mObjects[i].bbox.y /= pModels->SAMPLE_RESTORE_HEIGHT;
            pResults->mObjects[i].bbox.w /= pModels->SAMPLE_RESTORE_WIDTH;
            pResults->mObjects[i].bbox.h /= pModels->SAMPLE_RESTORE_HEIGHT;

            if (pResults->mObjects[i].bHasFaceLmk)
            {
                for (AX_U8 j = 0; j < SAMPLE_RUN_JOINT_FACE_LMK_SIZE; j++)
                {
                    pResults->mObjects[i].landmark[j].x /= pModels->SAMPLE_RESTORE_WIDTH;
                    pResults->mObjects[i].landmark[j].y /= pModels->SAMPLE_RESTORE_HEIGHT;
                }
            }

            if (pResults->mObjects[i].bHasBoxVertices)
            {
                for (size_t j = 0; j < 4; j++)
                {
                    pResults->mObjects[i].bbox_vertices[j].x /= pModels->SAMPLE_RESTORE_WIDTH;
                    pResults->mObjects[i].bbox_vertices[j].y /= pModels->SAMPLE_RESTORE_HEIGHT;
                }
            }
        }
        break;
    }
}