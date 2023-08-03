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
        return -1;
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
            generate_proposals_yolov5_face(stride, ptr, PROB_THRESHOLD, proposals, pModels->mMajor.JointAttr.algo_width, pModels->mMajor.JointAttr.algo_height, ANCHORS.data(), prob_threshold_unsigmoid, SAMPLE_RUN_JOINT_FACE_LMK_SIZE);
            break;
        case MT_DET_YOLOV5_LICENSE_PLATE:
            generate_proposals_yolov5_face(stride, ptr, PROB_THRESHOLD, proposals, pModels->mMajor.JointAttr.algo_width, pModels->mMajor.JointAttr.algo_height, ANCHORS.data(), prob_threshold_unsigmoid, SAMPLE_RUN_JOINT_PLATE_LMK_SIZE);
            break;
        case MT_DET_YOLOV6:
            generate_proposals_yolov6(stride, ptr, PROB_THRESHOLD, proposals, pModels->mMajor.JointAttr.algo_width, pModels->mMajor.JointAttr.algo_height, CLASS_NUM);
            break;
        case MT_DET_YOLOV7:
            generate_proposals_yolov7(stride, ptr, PROB_THRESHOLD, proposals, pModels->mMajor.JointAttr.algo_width, pModels->mMajor.JointAttr.algo_height, ANCHORS.data() + i * 6, CLASS_NUM);
            break;
        case MT_DET_YOLOX:
            generate_proposals_yolox(stride, ptr, PROB_THRESHOLD, proposals, pModels->mMajor.JointAttr.algo_width, pModels->mMajor.JointAttr.algo_height, CLASS_NUM);
            break;
        case MT_DET_YOLOV7_FACE:
            generate_proposals_yolov7_face(stride, ptr, PROB_THRESHOLD, proposals, pModels->mMajor.JointAttr.algo_width, pModels->mMajor.JointAttr.algo_height, ANCHORS.data(), prob_threshold_unsigmoid);
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
            generate_yolox_proposals(grid_stride, ptr, PROB_THRESHOLD, proposals, wxc, CLASS_NUM);
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

        // pResults->mObjects[i].bHasLandmark = pResults->mObjects[i].bHasMask = 0;
        if (pModels->mMajor.ModelType == MT_DET_YOLOV5_FACE || pModels->mMajor.ModelType == MT_DET_YOLOV7_FACE)
        {
            pResults->mObjects[i].nLandmark = SAMPLE_RUN_JOINT_FACE_LMK_SIZE;
            for (size_t j = 0; j < SAMPLE_RUN_JOINT_FACE_LMK_SIZE; j++)
            {
                pResults->mObjects[i].landmark[j].x = obj.landmark[j].x;
                pResults->mObjects[i].landmark[j].y = obj.landmark[j].y;
            }
        }
        else if (pModels->mMajor.ModelType == MT_DET_YOLOV5_LICENSE_PLATE)
        {
            pResults->mObjects[i].nLandmark = SAMPLE_RUN_JOINT_PLATE_LMK_SIZE;
            for (size_t j = 0; j < SAMPLE_RUN_JOINT_PLATE_LMK_SIZE; j++)
            {
                pResults->mObjects[i].landmark[j].x = obj.landmark[j].x;
                pResults->mObjects[i].landmark[j].y = obj.landmark[j].y;
                pResults->mObjects[i].bbox_vertices[j].x = pResults->mObjects[i].landmark[j].x;
                pResults->mObjects[i].bbox_vertices[j].y = pResults->mObjects[i].landmark[j].y;
            }
            pResults->mObjects[i].bHasBoxVertices = 1;

            std::vector<sample_run_joint_point> pppp(4);
            memcpy(pppp.data(), &pResults->mObjects[i].bbox_vertices[0], 4 * sizeof(sample_run_joint_point));
            std::sort(pppp.begin(), pppp.end(), [](sample_run_joint_point &a, sample_run_joint_point &b)
                      { return a.x < b.x; });
            if (pppp[0].y < pppp[1].y)
            {
                pResults->mObjects[i].bbox_vertices[0] = pppp[0];
                pResults->mObjects[i].bbox_vertices[3] = pppp[1];
            }
            else
            {
                pResults->mObjects[i].bbox_vertices[0] = pppp[1];
                pResults->mObjects[i].bbox_vertices[3] = pppp[0];
            }

            if (pppp[2].y < pppp[3].y)
            {
                pResults->mObjects[i].bbox_vertices[1] = pppp[2];
                pResults->mObjects[i].bbox_vertices[2] = pppp[3];
            }
            else
            {
                pResults->mObjects[i].bbox_vertices[1] = pppp[3];
                pResults->mObjects[i].bbox_vertices[2] = pppp[2];
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

    static SimpleRingBuffer<cv::Mat> mSimpleRingBuffer(SAMPLE_MAX_YOLOV5_MASK_OBJ_COUNT * SAMPLE_RINGBUFFER_CACHE_COUNT);
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

        pResults->mObjects[i].bHasMask = !obj.mask.empty();

        if (pResults->mObjects[i].bHasMask)
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

void sample_run_joint_post_process_yolov7_palm_hand(sample_run_joint_results *pResults, sample_run_joint_models *pModels)
{
    std::vector<detection::PalmObject> proposals;
    std::vector<detection::PalmObject> objects;
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
        detection::generate_proposals_yolov7_palm(stride, ptr, PROB_THRESHOLD, proposals, pModels->mMajor.JointAttr.algo_width, pModels->mMajor.JointAttr.algo_height, ANCHORS.data(), prob_threshold_unsigmoid);
    }

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

        strcpy(pResults->mObjects[i].objname, "hand");
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

        strcpy(pResults->mObjects[i].objname, "hand");
    }
}

void sample_run_joint_post_process_yolopv2(sample_run_joint_results *pResults, sample_run_joint_models *pModels)
{
    AX_U32 nOutputSize = pModels->mMajor.JointAttr.nOutputSize;
    AX_JOINT_IOMETA_T *pOutputsInfo = pModels->mMajor.JointAttr.pOutputsInfo;
    AX_JOINT_IO_BUFFER_T *pOutputs = pModels->mMajor.JointAttr.pOutputs;

    std::vector<detection::Object> proposals;
    std::vector<detection::Object> objects;

    float prob_threshold_unsigmoid = -1.0f * (float)std::log((1.0f / PROB_THRESHOLD) - 1.0f);
    for (uint32_t i = 2; i < nOutputSize; ++i)
    {
        auto &output = pOutputsInfo[i];
        auto &info = pOutputs[i];

        auto ptr = (float *)info.pVirAddr;

        int32_t stride = (1 << (i - 2)) * 8;
        generate_proposals_yolov5(stride, ptr, PROB_THRESHOLD, proposals, pModels->mMajor.JointAttr.algo_width, pModels->mMajor.JointAttr.algo_height, ANCHORS.data(), prob_threshold_unsigmoid, 80);
    }

    static SimpleRingBuffer<cv::Mat> mSimpleRingBuffer_seg(SAMPLE_RINGBUFFER_CACHE_COUNT), mSimpleRingBuffer_ll(SAMPLE_RINGBUFFER_CACHE_COUNT);
    auto &da_info = pOutputs[0];
    auto da_ptr = (float *)da_info.pVirAddr;
    auto &ll_info = pOutputs[1];
    auto ll_ptr = (float *)ll_info.pVirAddr;
    cv::Mat &da_seg_mask = mSimpleRingBuffer_seg.next();
    cv::Mat &ll_seg_mask = mSimpleRingBuffer_ll.next();

    detection::get_out_bbox_yolopv2(proposals, objects, da_ptr, ll_ptr, ll_seg_mask, da_seg_mask,
                                    NMS_THRESHOLD, pModels->mMajor.JointAttr.algo_height, pModels->mMajor.JointAttr.algo_width,
                                    pModels->SAMPLE_RESTORE_HEIGHT, pModels->SAMPLE_RESTORE_WIDTH);
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

        pResults->mObjects[i].label = 0;
        strcpy(pResults->mObjects[i].objname, "car");
    }

    pResults->bYolopv2Mask = 1;
    pResults->mYolopv2seg.h = da_seg_mask.rows;
    pResults->mYolopv2seg.w = da_seg_mask.cols;
    pResults->mYolopv2seg.data = da_seg_mask.data;
    pResults->mYolopv2ll.h = ll_seg_mask.rows;
    pResults->mYolopv2ll.w = ll_seg_mask.cols;
    pResults->mYolopv2ll.data = ll_seg_mask.data;
}

void sample_run_joint_post_process_yolofastbody(sample_run_joint_results *pResults, sample_run_joint_models *pModels)
{
    AX_U32 nOutputSize = pModels->mMajor.JointAttr.nOutputSize;
    AX_JOINT_IOMETA_T *pOutputsInfo = pModels->mMajor.JointAttr.pOutputsInfo;
    AX_JOINT_IO_BUFFER_T *pOutputs = pModels->mMajor.JointAttr.pOutputs;

    static yolo::YoloDetectionOutput yolo{};
    static std::vector<yolo::TMat> yolo_inputs, yolo_outputs;
    static std::vector<float> output_buf;

    static bool bInit = false;
    if (!bInit)
    {
        bInit = true;
        yolo.init(yolo::YOLO_FASTEST_BODY, NMS_THRESHOLD, PROB_THRESHOLD, 1);
        yolo_inputs.resize(nOutputSize);
        yolo_outputs.resize(1);
        output_buf.resize(1000 * 6, 0);
    }

    for (uint32_t i = 0; i < nOutputSize; ++i)
    {
        auto &output = pOutputsInfo[i];
        auto &info = pOutputs[i];

        auto ptr = (float *)info.pVirAddr;

        yolo_inputs[i].batch = output.pShape[0];
        yolo_inputs[i].h = output.pShape[1];
        yolo_inputs[i].w = output.pShape[2];
        yolo_inputs[i].c = output.pShape[3];
        yolo_inputs[i].data = ptr;
    }

    yolo_outputs[0].batch = 1;
    yolo_outputs[0].c = 1;
    yolo_outputs[0].h = 1000;
    yolo_outputs[0].w = 6;
    yolo_outputs[0].data = output_buf.data();

    yolo.forward_nhwc(yolo_inputs, yolo_outputs);

    std::vector<detection::Object> objects(yolo_outputs[0].h);
    // for (size_t i = 0; i < yolo_outputs[0].h; i++)
    // {
    //     float *data_row = yolo_outputs[0].row((int)i);
    //     detection::Object &object = objects[i];
    //     object.rect.x = data_row[2] * (float)pModels->SAMPLE_ALGO_WIDTH;
    //     object.rect.y = data_row[3] * (float)pModels->SAMPLE_ALGO_HEIGHT;
    //     object.rect.width = (data_row[4] - data_row[2]) * (float)pModels->SAMPLE_ALGO_WIDTH;
    //     object.rect.height = (data_row[5] - data_row[3]) * (float)pModels->SAMPLE_ALGO_HEIGHT;
    //     object.label = (int)data_row[0];
    //     object.prob = data_row[1];
    // }

    float scale_letterbox;
    int resize_rows;
    int resize_cols;
    int letterbox_rows = pModels->SAMPLE_ALGO_HEIGHT;
    int letterbox_cols = pModels->SAMPLE_ALGO_WIDTH;
    int src_rows = pModels->SAMPLE_RESTORE_HEIGHT;
    int src_cols = pModels->SAMPLE_RESTORE_WIDTH;
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

    for (int i = 0; i < yolo_outputs[0].h; i++)
    {
        float *data_row = yolo_outputs[0].row((int)i);
        detection::Object &object = objects[i];
        object.rect.x = data_row[2] * (float)pModels->SAMPLE_ALGO_WIDTH;
        object.rect.y = data_row[3] * (float)pModels->SAMPLE_ALGO_HEIGHT;
        object.rect.width = (data_row[4] - data_row[2]) * (float)pModels->SAMPLE_ALGO_WIDTH;
        object.rect.height = (data_row[5] - data_row[3]) * (float)pModels->SAMPLE_ALGO_HEIGHT;
        object.label = (int)data_row[0];
        object.prob = data_row[1];

        float x0 = (objects[i].rect.x);
        float y0 = (objects[i].rect.y);
        float x1 = (objects[i].rect.x + objects[i].rect.width);
        float y1 = (objects[i].rect.y + objects[i].rect.height);

        x0 = (x0 - tmp_w) * ratio_x;
        y0 = (y0 - tmp_h) * ratio_y;
        x1 = (x1 - tmp_w) * ratio_x;
        y1 = (y1 - tmp_h) * ratio_y;

        x0 = std::max(std::min(x0, (float)(src_cols - 1)), 0.f);
        y0 = std::max(std::min(y0, (float)(src_rows - 1)), 0.f);
        x1 = std::max(std::min(x1, (float)(src_cols - 1)), 0.f);
        y1 = std::max(std::min(y1, (float)(src_rows - 1)), 0.f);

        objects[i].rect.x = x0;
        objects[i].rect.y = y0;
        objects[i].rect.width = x1 - x0;
        objects[i].rect.height = y1 - y0;
    }

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
        pResults->mObjects[i].label = 0;
        strcpy(pResults->mObjects[i].objname, "person");
    }
}

void sample_run_joint_post_process_det_single_func(sample_run_joint_results *pResults, sample_run_joint_models *pModels)
{
    typedef void (*post_process_func)(sample_run_joint_results * pResults, sample_run_joint_models * pModels);
    static std::map<int, post_process_func> m_func_map{
        {MT_DET_YOLOV5, sample_run_joint_post_process_detection},
        {MT_DET_YOLOV5_FACE, sample_run_joint_post_process_detection},
        {MT_DET_YOLOV6, sample_run_joint_post_process_detection},
        {MT_DET_YOLOV7, sample_run_joint_post_process_detection},
        {MT_DET_YOLOX, sample_run_joint_post_process_detection},
        {MT_DET_NANODET, sample_run_joint_post_process_detection},
        {MT_DET_YOLOX_PPL, sample_run_joint_post_process_detection},
        {MT_DET_YOLOV5_LICENSE_PLATE, sample_run_joint_post_process_detection},
        {MT_DET_YOLOV7_FACE, sample_run_joint_post_process_detection},

        {MT_DET_YOLOPV2, sample_run_joint_post_process_yolopv2},

        {MT_DET_YOLO_FASTBODY, sample_run_joint_post_process_yolofastbody},

        {MT_INSEG_YOLOV5_MASK, sample_run_joint_post_process_yolov5_seg},

        {MT_DET_PALM_HAND, sample_run_joint_post_process_palm_hand},
        {MT_DET_YOLOV7_PALM_HAND, sample_run_joint_post_process_yolov7_palm_hand},
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

            if (pModels->mMajor.ModelType == MT_DET_YOLOV5_FACE || pModels->mMajor.ModelType == MT_DET_YOLOV7_FACE)
            {
                for (AX_U8 j = 0; j < SAMPLE_RUN_JOINT_FACE_LMK_SIZE; j++)
                {
                    pResults->mObjects[i].landmark[j].x /= pModels->SAMPLE_RESTORE_WIDTH;
                    pResults->mObjects[i].landmark[j].y /= pModels->SAMPLE_RESTORE_HEIGHT;
                }
            }
            else if (pResults->mModelType == MT_DET_YOLOV5_LICENSE_PLATE)
            {
                for (AX_U8 j = 0; j < SAMPLE_RUN_JOINT_PLATE_LMK_SIZE; j++)
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