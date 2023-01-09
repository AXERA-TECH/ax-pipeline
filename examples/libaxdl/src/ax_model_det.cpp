#include "ax_model_det.hpp"
#include "../../utilities/json.hpp"

#include "../../utilities/sample_log.h"

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

int ax_model_yolov5::post_process(const void *pstFrame, ax_joint_runner_box *crop_resize_box, sample_run_joint_results *results)
{
    std::vector<detection::Object> proposals;
    std::vector<detection::Object> objects;
    AX_U32 nOutputSize = m_runner.m_attr.nOutputSize;
    AX_JOINT_IOMETA_T *pOutputsInfo = m_runner.m_attr.pOutputsInfo;
    AX_JOINT_IO_BUFFER_T *pOutputs = m_runner.m_attr.pOutputs;

    float prob_threshold_unsigmoid = -1.0f * (float)std::log((1.0f / PROB_THRESHOLD) - 1.0f);
    for (uint32_t i = 0; i < nOutputSize; ++i)
    {
        auto &output = pOutputsInfo[i];
        auto &info = pOutputs[i];
        auto ptr = (float *)info.pVirAddr;
        int32_t stride = (1 << i) * 8;

        generate_proposals_yolov5(stride, ptr, PROB_THRESHOLD, proposals, m_runner.m_attr.algo_width, m_runner.m_attr.algo_height, ANCHORS.data(), prob_threshold_unsigmoid, CLASS_NUM);
    }

    detection::get_out_bbox(proposals, objects, NMS_THRESHOLD, m_runner.m_attr.algo_height, m_runner.m_attr.algo_width, HEIGHT_DET_BBOX_RESTORE, WIDTH_DET_BBOX_RESTORE);
    std::sort(objects.begin(), objects.end(),
              [&](detection::Object &a, detection::Object &b)
              {
                  return a.rect.area() > b.rect.area();
              });

    results->nObjSize = MIN(objects.size(), SAMPLE_MAX_BBOX_COUNT);
    for (size_t i = 0; i < results->nObjSize; i++)
    {
        const detection::Object &obj = objects[i];
        results->mObjects[i].bbox.x = obj.rect.x;
        results->mObjects[i].bbox.y = obj.rect.y;
        results->mObjects[i].bbox.w = obj.rect.width;
        results->mObjects[i].bbox.h = obj.rect.height;
        results->mObjects[i].label = obj.label;
        results->mObjects[i].prob = obj.prob;
        if (obj.label < CLASS_NAMES.size())
        {
            strcpy(results->mObjects[i].objname, CLASS_NAMES[obj.label].c_str());
        }
        else
        {
            strcpy(results->mObjects[i].objname, "unknown");
        }
    }
    return 0;
}

int ax_model_yolov5_seg::post_process(const void *pstFrame, ax_joint_runner_box *crop_resize_box, sample_run_joint_results *results)
{
    std::vector<detection::Object> proposals;
    std::vector<detection::Object> objects;
    AX_U32 nOutputSize = m_runner.m_attr.nOutputSize;
    AX_JOINT_IOMETA_T *pOutputsInfo = m_runner.m_attr.pOutputsInfo;
    AX_JOINT_IO_BUFFER_T *pOutputs = m_runner.m_attr.pOutputs;

    float prob_threshold_unsigmoid = -1.0f * (float)std::log((1.0f / PROB_THRESHOLD) - 1.0f);
    for (uint32_t i = 0; i < nOutputSize - 1; ++i)
    {
        auto &output = pOutputsInfo[i];
        auto &info = pOutputs[i];
        auto ptr = (float *)info.pVirAddr;
        int32_t stride = (1 << i) * 8;
        generate_proposals_yolov5_seg(stride, ptr, PROB_THRESHOLD, proposals, m_runner.m_attr.algo_width, m_runner.m_attr.algo_height, ANCHORS.data(), prob_threshold_unsigmoid);
    }
    static const int DEFAULT_MASK_PROTO_DIM = 32;
    static const int DEFAULT_MASK_SAMPLE_STRIDE = 4;
    auto &output = pOutputsInfo[3];
    auto &info = pOutputs[3];
    auto ptr = (float *)info.pVirAddr;
    detection::get_out_bbox_mask(proposals, objects, SAMPLE_MAX_YOLOV5_MASK_OBJ_COUNT, ptr, DEFAULT_MASK_PROTO_DIM, DEFAULT_MASK_SAMPLE_STRIDE, NMS_THRESHOLD,
                                 m_runner.m_attr.algo_height, m_runner.m_attr.algo_width, HEIGHT_DET_BBOX_RESTORE, WIDTH_DET_BBOX_RESTORE);

    std::sort(objects.begin(), objects.end(),
              [&](detection::Object &a, detection::Object &b)
              {
                  return a.rect.area() > b.rect.area();
              });

    static SimpleRingBuffer<cv::Mat> mSimpleRingBuffer(SAMPLE_MAX_YOLOV5_MASK_OBJ_COUNT * SAMPLE_RINGBUFFER_CACHE_COUNT);
    results->nObjSize = MIN(objects.size(), SAMPLE_MAX_BBOX_COUNT);
    for (size_t i = 0; i < results->nObjSize; i++)
    {
        const detection::Object &obj = objects[i];
        results->mObjects[i].bbox.x = obj.rect.x;
        results->mObjects[i].bbox.y = obj.rect.y;
        results->mObjects[i].bbox.w = obj.rect.width;
        results->mObjects[i].bbox.h = obj.rect.height;
        results->mObjects[i].label = obj.label;
        results->mObjects[i].prob = obj.prob;

        results->mObjects[i].bHasMask = !obj.mask.empty();

        if (results->mObjects[i].bHasMask)
        {
            cv::Mat &mask = mSimpleRingBuffer.next();
            mask = obj.mask;
            results->mObjects[i].mYolov5Mask.data = mask.data;
            results->mObjects[i].mYolov5Mask.w = mask.cols;
            results->mObjects[i].mYolov5Mask.h = mask.rows;
        }

        if (obj.label < CLASS_NAMES.size())
        {
            strcpy(results->mObjects[i].objname, CLASS_NAMES[obj.label].c_str());
        }
        else
        {
            strcpy(results->mObjects[i].objname, "unknown");
        }
    }
    return 0;
}

int ax_model_yolov5_face::post_process(const void *pstFrame, ax_joint_runner_box *crop_resize_box, sample_run_joint_results *results)
{
    if (mSimpleRingBuffer.size() == 0)
    {
        mSimpleRingBuffer.resize(SAMPLE_RINGBUFFER_CACHE_COUNT * SAMPLE_MAX_FACE_BBOX_COUNT);
    }
    std::vector<detection::Object> proposals;
    std::vector<detection::Object> objects;
    AX_U32 nOutputSize = m_runner.m_attr.nOutputSize;
    AX_JOINT_IOMETA_T *pOutputsInfo = m_runner.m_attr.pOutputsInfo;
    AX_JOINT_IO_BUFFER_T *pOutputs = m_runner.m_attr.pOutputs;

    float prob_threshold_unsigmoid = -1.0f * (float)std::log((1.0f / PROB_THRESHOLD) - 1.0f);
    for (uint32_t i = 0; i < nOutputSize; ++i)
    {
        auto &output = pOutputsInfo[i];
        auto &info = pOutputs[i];
        auto ptr = (float *)info.pVirAddr;
        int32_t stride = (1 << i) * 8;

        generate_proposals_yolov5_face(stride, ptr, PROB_THRESHOLD, proposals, m_runner.m_attr.algo_width, m_runner.m_attr.algo_height, ANCHORS.data(), prob_threshold_unsigmoid, SAMPLE_RUN_JOINT_FACE_LMK_SIZE);
    }

    detection::get_out_bbox(proposals, objects, NMS_THRESHOLD, m_runner.m_attr.algo_height, m_runner.m_attr.algo_width, HEIGHT_DET_BBOX_RESTORE, WIDTH_DET_BBOX_RESTORE);
    std::sort(objects.begin(), objects.end(),
              [&](detection::Object &a, detection::Object &b)
              {
                  return a.rect.area() > b.rect.area();
              });

    results->nObjSize = MIN(objects.size(), SAMPLE_MAX_FACE_BBOX_COUNT);
    for (size_t i = 0; i < results->nObjSize; i++)
    {
        const detection::Object &obj = objects[i];
        results->mObjects[i].bbox.x = obj.rect.x;
        results->mObjects[i].bbox.y = obj.rect.y;
        results->mObjects[i].bbox.w = obj.rect.width;
        results->mObjects[i].bbox.h = obj.rect.height;
        results->mObjects[i].label = obj.label;
        results->mObjects[i].prob = obj.prob;
        results->mObjects[i].nLandmark = SAMPLE_RUN_JOINT_FACE_LMK_SIZE;
        std::vector<sample_run_joint_point> &points = mSimpleRingBuffer.next();
        points.resize(results->mObjects[i].nLandmark);
        results->mObjects[i].landmark = points.data();
        for (size_t j = 0; j < SAMPLE_RUN_JOINT_FACE_LMK_SIZE; j++)
        {
            results->mObjects[i].landmark[j].x = obj.landmark[j].x;
            results->mObjects[i].landmark[j].y = obj.landmark[j].y;
        }

        if (obj.label < CLASS_NAMES.size())
        {
            strcpy(results->mObjects[i].objname, CLASS_NAMES[obj.label].c_str());
        }
        else
        {
            strcpy(results->mObjects[i].objname, "unknown");
        }
    }
    return 0;
}

int ax_model_yolov5_lisence_plate::post_process(const void *pstFrame, ax_joint_runner_box *crop_resize_box, sample_run_joint_results *results)
{
    if (mSimpleRingBuffer.size() == 0)
    {
        mSimpleRingBuffer.resize(SAMPLE_RINGBUFFER_CACHE_COUNT * SAMPLE_MAX_BBOX_COUNT);
    }
    std::vector<detection::Object> proposals;
    std::vector<detection::Object> objects;
    AX_U32 nOutputSize = m_runner.m_attr.nOutputSize;
    AX_JOINT_IOMETA_T *pOutputsInfo = m_runner.m_attr.pOutputsInfo;
    AX_JOINT_IO_BUFFER_T *pOutputs = m_runner.m_attr.pOutputs;

    float prob_threshold_unsigmoid = -1.0f * (float)std::log((1.0f / PROB_THRESHOLD) - 1.0f);
    for (uint32_t i = 0; i < nOutputSize; ++i)
    {
        auto &output = pOutputsInfo[i];
        auto &info = pOutputs[i];
        auto ptr = (float *)info.pVirAddr;
        int32_t stride = (1 << i) * 8;

        generate_proposals_yolov5_face(stride, ptr, PROB_THRESHOLD, proposals, m_runner.m_attr.algo_width, m_runner.m_attr.algo_height, ANCHORS.data(), prob_threshold_unsigmoid, SAMPLE_RUN_JOINT_PLATE_LMK_SIZE);
    }

    detection::get_out_bbox(proposals, objects, NMS_THRESHOLD, m_runner.m_attr.algo_height, m_runner.m_attr.algo_width, HEIGHT_DET_BBOX_RESTORE, WIDTH_DET_BBOX_RESTORE);
    std::sort(objects.begin(), objects.end(),
              [&](detection::Object &a, detection::Object &b)
              {
                  return a.rect.area() > b.rect.area();
              });

    results->nObjSize = MIN(objects.size(), SAMPLE_MAX_BBOX_COUNT);
    for (size_t i = 0; i < results->nObjSize; i++)
    {
        const detection::Object &obj = objects[i];
        results->mObjects[i].bbox.x = obj.rect.x;
        results->mObjects[i].bbox.y = obj.rect.y;
        results->mObjects[i].bbox.w = obj.rect.width;
        results->mObjects[i].bbox.h = obj.rect.height;
        results->mObjects[i].label = obj.label;
        results->mObjects[i].prob = obj.prob;
        results->mObjects[i].nLandmark = SAMPLE_RUN_JOINT_PLATE_LMK_SIZE;
        std::vector<sample_run_joint_point> &points = mSimpleRingBuffer.next();
        points.resize(results->mObjects[i].nLandmark);
        results->mObjects[i].landmark = points.data();
        for (size_t j = 0; j < SAMPLE_RUN_JOINT_PLATE_LMK_SIZE; j++)
        {
            results->mObjects[i].landmark[j].x = obj.landmark[j].x;
            results->mObjects[i].landmark[j].y = obj.landmark[j].y;
            results->mObjects[i].bbox_vertices[j].x = results->mObjects[i].landmark[j].x;
            results->mObjects[i].bbox_vertices[j].y = results->mObjects[i].landmark[j].y;
        }
        results->mObjects[i].bHasBoxVertices = 1;

        std::vector<sample_run_joint_point> pppp(4);
        memcpy(pppp.data(), &results->mObjects[i].bbox_vertices[0], 4 * sizeof(sample_run_joint_point));
        std::sort(pppp.begin(), pppp.end(), [](sample_run_joint_point &a, sample_run_joint_point &b)
                  { return a.x < b.x; });
        if (pppp[0].y < pppp[1].y)
        {
            results->mObjects[i].bbox_vertices[0] = pppp[0];
            results->mObjects[i].bbox_vertices[3] = pppp[1];
        }
        else
        {
            results->mObjects[i].bbox_vertices[0] = pppp[1];
            results->mObjects[i].bbox_vertices[3] = pppp[0];
        }

        if (pppp[2].y < pppp[3].y)
        {
            results->mObjects[i].bbox_vertices[1] = pppp[2];
            results->mObjects[i].bbox_vertices[2] = pppp[3];
        }
        else
        {
            results->mObjects[i].bbox_vertices[1] = pppp[3];
            results->mObjects[i].bbox_vertices[2] = pppp[2];
        }
        if (obj.label < CLASS_NAMES.size())
        {
            strcpy(results->mObjects[i].objname, CLASS_NAMES[obj.label].c_str());
        }
        else
        {
            strcpy(results->mObjects[i].objname, "unknown");
        }
    }
    return 0;
}

int ax_model_yolov6::post_process(const void *pstFrame, ax_joint_runner_box *crop_resize_box, sample_run_joint_results *results)
{
    std::vector<detection::Object> proposals;
    std::vector<detection::Object> objects;
    AX_U32 nOutputSize = m_runner.m_attr.nOutputSize;
    AX_JOINT_IOMETA_T *pOutputsInfo = m_runner.m_attr.pOutputsInfo;
    AX_JOINT_IO_BUFFER_T *pOutputs = m_runner.m_attr.pOutputs;

    float prob_threshold_unsigmoid = -1.0f * (float)std::log((1.0f / PROB_THRESHOLD) - 1.0f);
    for (uint32_t i = 0; i < nOutputSize; ++i)
    {
        auto &output = pOutputsInfo[i];
        auto &info = pOutputs[i];
        auto ptr = (float *)info.pVirAddr;
        int32_t stride = (1 << i) * 8;

        generate_proposals_yolov6(stride, ptr, PROB_THRESHOLD, proposals, m_runner.m_attr.algo_width, m_runner.m_attr.algo_height, CLASS_NUM);
    }

    detection::get_out_bbox(proposals, objects, NMS_THRESHOLD, m_runner.m_attr.algo_height, m_runner.m_attr.algo_width, HEIGHT_DET_BBOX_RESTORE, WIDTH_DET_BBOX_RESTORE);
    std::sort(objects.begin(), objects.end(),
              [&](detection::Object &a, detection::Object &b)
              {
                  return a.rect.area() > b.rect.area();
              });

    results->nObjSize = MIN(objects.size(), SAMPLE_MAX_BBOX_COUNT);
    for (size_t i = 0; i < results->nObjSize; i++)
    {
        const detection::Object &obj = objects[i];
        results->mObjects[i].bbox.x = obj.rect.x;
        results->mObjects[i].bbox.y = obj.rect.y;
        results->mObjects[i].bbox.w = obj.rect.width;
        results->mObjects[i].bbox.h = obj.rect.height;
        results->mObjects[i].label = obj.label;
        results->mObjects[i].prob = obj.prob;
        if (obj.label < CLASS_NAMES.size())
        {
            strcpy(results->mObjects[i].objname, CLASS_NAMES[obj.label].c_str());
        }
        else
        {
            strcpy(results->mObjects[i].objname, "unknown");
        }
    }
    return 0;
}

int ax_model_yolov7::post_process(const void *pstFrame, ax_joint_runner_box *crop_resize_box, sample_run_joint_results *results)
{
    std::vector<detection::Object> proposals;
    std::vector<detection::Object> objects;
    AX_U32 nOutputSize = m_runner.m_attr.nOutputSize;
    AX_JOINT_IOMETA_T *pOutputsInfo = m_runner.m_attr.pOutputsInfo;
    AX_JOINT_IO_BUFFER_T *pOutputs = m_runner.m_attr.pOutputs;

    float prob_threshold_unsigmoid = -1.0f * (float)std::log((1.0f / PROB_THRESHOLD) - 1.0f);
    for (uint32_t i = 0; i < nOutputSize; ++i)
    {
        auto &output = pOutputsInfo[i];
        auto &info = pOutputs[i];
        auto ptr = (float *)info.pVirAddr;
        int32_t stride = (1 << i) * 8;

        generate_proposals_yolov7(stride, ptr, PROB_THRESHOLD, proposals, m_runner.m_attr.algo_width, m_runner.m_attr.algo_height, ANCHORS.data() + i * 6, CLASS_NUM);
    }

    detection::get_out_bbox(proposals, objects, NMS_THRESHOLD, m_runner.m_attr.algo_height, m_runner.m_attr.algo_width, HEIGHT_DET_BBOX_RESTORE, WIDTH_DET_BBOX_RESTORE);
    std::sort(objects.begin(), objects.end(),
              [&](detection::Object &a, detection::Object &b)
              {
                  return a.rect.area() > b.rect.area();
              });

    results->nObjSize = MIN(objects.size(), SAMPLE_MAX_BBOX_COUNT);
    for (size_t i = 0; i < results->nObjSize; i++)
    {
        const detection::Object &obj = objects[i];
        results->mObjects[i].bbox.x = obj.rect.x;
        results->mObjects[i].bbox.y = obj.rect.y;
        results->mObjects[i].bbox.w = obj.rect.width;
        results->mObjects[i].bbox.h = obj.rect.height;
        results->mObjects[i].label = obj.label;
        results->mObjects[i].prob = obj.prob;
        if (obj.label < CLASS_NAMES.size())
        {
            strcpy(results->mObjects[i].objname, CLASS_NAMES[obj.label].c_str());
        }
        else
        {
            strcpy(results->mObjects[i].objname, "unknown");
        }
    }
    return 0;
}

int ax_model_yolov7_face::post_process(const void *pstFrame, ax_joint_runner_box *crop_resize_box, sample_run_joint_results *results)
{
    if (mSimpleRingBuffer.size() == 0)
    {
        mSimpleRingBuffer.resize(SAMPLE_RINGBUFFER_CACHE_COUNT * SAMPLE_MAX_FACE_BBOX_COUNT);
    }
    std::vector<detection::Object> proposals;
    std::vector<detection::Object> objects;
    AX_U32 nOutputSize = m_runner.m_attr.nOutputSize;
    AX_JOINT_IOMETA_T *pOutputsInfo = m_runner.m_attr.pOutputsInfo;
    AX_JOINT_IO_BUFFER_T *pOutputs = m_runner.m_attr.pOutputs;

    float prob_threshold_unsigmoid = -1.0f * (float)std::log((1.0f / PROB_THRESHOLD) - 1.0f);
    for (uint32_t i = 0; i < nOutputSize; ++i)
    {
        auto &output = pOutputsInfo[i];
        auto &info = pOutputs[i];
        auto ptr = (float *)info.pVirAddr;
        int32_t stride = (1 << i) * 8;

        generate_proposals_yolov7_face(stride, ptr, PROB_THRESHOLD, proposals, m_runner.m_attr.algo_width, m_runner.m_attr.algo_height, ANCHORS.data(), prob_threshold_unsigmoid);
    }

    detection::get_out_bbox(proposals, objects, NMS_THRESHOLD, m_runner.m_attr.algo_height, m_runner.m_attr.algo_width, HEIGHT_DET_BBOX_RESTORE, WIDTH_DET_BBOX_RESTORE);
    std::sort(objects.begin(), objects.end(),
              [&](detection::Object &a, detection::Object &b)
              {
                  return a.rect.area() > b.rect.area();
              });

    results->nObjSize = MIN(objects.size(), SAMPLE_MAX_FACE_BBOX_COUNT);
    for (size_t i = 0; i < results->nObjSize; i++)
    {
        const detection::Object &obj = objects[i];
        results->mObjects[i].bbox.x = obj.rect.x;
        results->mObjects[i].bbox.y = obj.rect.y;
        results->mObjects[i].bbox.w = obj.rect.width;
        results->mObjects[i].bbox.h = obj.rect.height;
        results->mObjects[i].label = obj.label;
        results->mObjects[i].prob = obj.prob;
        results->mObjects[i].nLandmark = SAMPLE_RUN_JOINT_FACE_LMK_SIZE;
        std::vector<sample_run_joint_point> &points = mSimpleRingBuffer.next();
        points.resize(results->mObjects[i].nLandmark);
        results->mObjects[i].landmark = points.data();
        for (size_t j = 0; j < SAMPLE_RUN_JOINT_FACE_LMK_SIZE; j++)
        {
            results->mObjects[i].landmark[j].x = obj.landmark[j].x;
            results->mObjects[i].landmark[j].y = obj.landmark[j].y;
        }
        if (obj.label < CLASS_NAMES.size())
        {
            strcpy(results->mObjects[i].objname, CLASS_NAMES[obj.label].c_str());
        }
        else
        {
            strcpy(results->mObjects[i].objname, "unknown");
        }
    }
    return 0;
}

int ax_model_yolov7_plam_hand::post_process(const void *pstFrame, ax_joint_runner_box *crop_resize_box, sample_run_joint_results *results)
{
    std::vector<detection::PalmObject> proposals;
    std::vector<detection::PalmObject> objects;
    AX_U32 nOutputSize = m_runner.m_attr.nOutputSize;
    AX_JOINT_IOMETA_T *pOutputsInfo = m_runner.m_attr.pOutputsInfo;
    AX_JOINT_IO_BUFFER_T *pOutputs = m_runner.m_attr.pOutputs;

    float prob_threshold_unsigmoid = -1.0f * (float)std::log((1.0f / PROB_THRESHOLD) - 1.0f);
    for (uint32_t i = 0; i < nOutputSize; ++i)
    {
        auto &output = pOutputsInfo[i];
        auto &info = pOutputs[i];
        auto ptr = (float *)info.pVirAddr;
        int32_t stride = (1 << i) * 8;

        generate_proposals_yolov7_palm(stride, ptr, PROB_THRESHOLD, proposals, m_runner.m_attr.algo_width, m_runner.m_attr.algo_height, ANCHORS.data(), prob_threshold_unsigmoid);
    }

    detection::get_out_bbox_palm(proposals, objects, NMS_THRESHOLD, m_runner.m_attr.algo_height, m_runner.m_attr.algo_width, HEIGHT_DET_BBOX_RESTORE, WIDTH_DET_BBOX_RESTORE);
    std::sort(objects.begin(), objects.end(),
              [&](detection::PalmObject &a, detection::PalmObject &b)
              {
                  return a.rect.area() > b.rect.area();
              });

    results->nObjSize = MIN(objects.size(), SAMPLE_MAX_HAND_BBOX_COUNT);
    for (size_t i = 0; i < results->nObjSize; i++)
    {
        const detection::PalmObject &obj = objects[i];
        results->mObjects[i].bbox.x = obj.rect.x * WIDTH_DET_BBOX_RESTORE;
        results->mObjects[i].bbox.y = obj.rect.y * HEIGHT_DET_BBOX_RESTORE;
        results->mObjects[i].bbox.w = obj.rect.width * WIDTH_DET_BBOX_RESTORE;
        results->mObjects[i].bbox.h = obj.rect.height * HEIGHT_DET_BBOX_RESTORE;
        results->mObjects[i].label = 0;
        results->mObjects[i].prob = obj.prob;
        results->mObjects[i].bHasBoxVertices = 1;
        for (size_t j = 0; j < 4; j++)
        {
            results->mObjects[i].bbox_vertices[j].x = obj.vertices[j].x;
            results->mObjects[i].bbox_vertices[j].y = obj.vertices[j].y;
        }

        strcpy(results->mObjects[i].objname, "hand");
    }
    return 0;
}

int ax_model_plam_hand::post_process(const void *pstFrame, ax_joint_runner_box *crop_resize_box, sample_run_joint_results *results)
{
    static const int map_size[2] = {24, 12};
    static const int strides[2] = {8, 16};
    static const int anchor_size[2] = {2, 6};
    static const float anchor_offset[2] = {0.5f, 0.5f};
    std::vector<detection::PalmObject> proposals;
    std::vector<detection::PalmObject> objects;

    auto &bboxes_info = m_runner.m_attr.pOutputs[0];
    auto bboxes_ptr = (float *)bboxes_info.pVirAddr;
    auto &scores_info = m_runner.m_attr.pOutputs[1];
    auto scores_ptr = (float *)scores_info.pVirAddr;
    float prob_threshold_unsigmoid = -1.0f * (float)std::log((1.0f / PROB_THRESHOLD) - 1.0f);
    detection::generate_proposals_palm(proposals, PROB_THRESHOLD, m_runner.m_attr.algo_width, m_runner.m_attr.algo_height, scores_ptr, bboxes_ptr, 2, strides, anchor_size, anchor_offset, map_size, prob_threshold_unsigmoid);

    detection::get_out_bbox_palm(proposals, objects, NMS_THRESHOLD, m_runner.m_attr.algo_height, m_runner.m_attr.algo_width, HEIGHT_DET_BBOX_RESTORE, WIDTH_DET_BBOX_RESTORE);

    std::sort(objects.begin(), objects.end(),
              [&](detection::PalmObject &a, detection::PalmObject &b)
              {
                  return a.rect.area() > b.rect.area();
              });

    results->nObjSize = MIN(objects.size(), SAMPLE_MAX_HAND_BBOX_COUNT);
    for (size_t i = 0; i < results->nObjSize; i++)
    {
        const detection::PalmObject &obj = objects[i];
        results->mObjects[i].bbox.x = obj.rect.x * WIDTH_DET_BBOX_RESTORE;
        results->mObjects[i].bbox.y = obj.rect.y * HEIGHT_DET_BBOX_RESTORE;
        results->mObjects[i].bbox.w = obj.rect.width * WIDTH_DET_BBOX_RESTORE;
        results->mObjects[i].bbox.h = obj.rect.height * HEIGHT_DET_BBOX_RESTORE;
        results->mObjects[i].label = 0;
        results->mObjects[i].prob = obj.prob;
        results->mObjects[i].bHasBoxVertices = 1;
        for (size_t j = 0; j < 4; j++)
        {
            results->mObjects[i].bbox_vertices[j].x = obj.vertices[j].x;
            results->mObjects[i].bbox_vertices[j].y = obj.vertices[j].y;
        }

        strcpy(results->mObjects[i].objname, "hand");
    }
    return 0;
}

int ax_model_yolox::post_process(const void *pstFrame, ax_joint_runner_box *crop_resize_box, sample_run_joint_results *results)
{
    std::vector<detection::Object> proposals;
    std::vector<detection::Object> objects;
    AX_U32 nOutputSize = m_runner.m_attr.nOutputSize;
    AX_JOINT_IOMETA_T *pOutputsInfo = m_runner.m_attr.pOutputsInfo;
    AX_JOINT_IO_BUFFER_T *pOutputs = m_runner.m_attr.pOutputs;

    float prob_threshold_unsigmoid = -1.0f * (float)std::log((1.0f / PROB_THRESHOLD) - 1.0f);
    for (uint32_t i = 0; i < nOutputSize; ++i)
    {
        auto &output = pOutputsInfo[i];
        auto &info = pOutputs[i];
        auto ptr = (float *)info.pVirAddr;
        int32_t stride = (1 << i) * 8;

        generate_proposals_yolox(stride, ptr, PROB_THRESHOLD, proposals, m_runner.m_attr.algo_width, m_runner.m_attr.algo_height, CLASS_NUM);
    }

    detection::get_out_bbox(proposals, objects, NMS_THRESHOLD, m_runner.m_attr.algo_height, m_runner.m_attr.algo_width, HEIGHT_DET_BBOX_RESTORE, WIDTH_DET_BBOX_RESTORE);
    std::sort(objects.begin(), objects.end(),
              [&](detection::Object &a, detection::Object &b)
              {
                  return a.rect.area() > b.rect.area();
              });

    results->nObjSize = MIN(objects.size(), SAMPLE_MAX_BBOX_COUNT);
    for (size_t i = 0; i < results->nObjSize; i++)
    {
        const detection::Object &obj = objects[i];
        results->mObjects[i].bbox.x = obj.rect.x;
        results->mObjects[i].bbox.y = obj.rect.y;
        results->mObjects[i].bbox.w = obj.rect.width;
        results->mObjects[i].bbox.h = obj.rect.height;
        results->mObjects[i].label = obj.label;
        results->mObjects[i].prob = obj.prob;
        if (obj.label < CLASS_NAMES.size())
        {
            strcpy(results->mObjects[i].objname, CLASS_NAMES[obj.label].c_str());
        }
        else
        {
            strcpy(results->mObjects[i].objname, "unknown");
        }
    }
    return 0;
}

int ax_model_yoloxppl::post_process(const void *pstFrame, ax_joint_runner_box *crop_resize_box, sample_run_joint_results *results)
{
    std::vector<detection::Object> proposals;
    std::vector<detection::Object> objects;
    AX_U32 nOutputSize = m_runner.m_attr.nOutputSize;
    AX_JOINT_IOMETA_T *pOutputsInfo = m_runner.m_attr.pOutputsInfo;
    AX_JOINT_IO_BUFFER_T *pOutputs = m_runner.m_attr.pOutputs;

    float prob_threshold_unsigmoid = -1.0f * (float)std::log((1.0f / PROB_THRESHOLD) - 1.0f);
    for (uint32_t i = 0; i < nOutputSize; ++i)
    {
        auto &output = pOutputsInfo[i];
        auto &info = pOutputs[i];
        auto ptr = (float *)info.pVirAddr;
        int32_t stride = (1 << i) * 8;

        std::vector<detection::GridAndStride> grid_stride;
        int wxc = output.pShape[2] * output.pShape[3];
        static std::vector<std::vector<int>> stride_ppl = {{8}, {16}, {32}};
        generate_grids_and_stride(m_runner.m_attr.algo_width, m_runner.m_attr.algo_height, stride_ppl[i], grid_stride);
        generate_yolox_proposals(grid_stride, ptr, PROB_THRESHOLD, proposals, wxc);
    }

    detection::get_out_bbox(proposals, objects, NMS_THRESHOLD, m_runner.m_attr.algo_height, m_runner.m_attr.algo_width, HEIGHT_DET_BBOX_RESTORE, WIDTH_DET_BBOX_RESTORE);
    std::sort(objects.begin(), objects.end(),
              [&](detection::Object &a, detection::Object &b)
              {
                  return a.rect.area() > b.rect.area();
              });

    results->nObjSize = MIN(objects.size(), SAMPLE_MAX_BBOX_COUNT);
    for (size_t i = 0; i < results->nObjSize; i++)
    {
        const detection::Object &obj = objects[i];
        results->mObjects[i].bbox.x = obj.rect.x;
        results->mObjects[i].bbox.y = obj.rect.y;
        results->mObjects[i].bbox.w = obj.rect.width;
        results->mObjects[i].bbox.h = obj.rect.height;
        results->mObjects[i].label = obj.label;
        results->mObjects[i].prob = obj.prob;
        if (obj.label < CLASS_NAMES.size())
        {
            strcpy(results->mObjects[i].objname, CLASS_NAMES[obj.label].c_str());
        }
        else
        {
            strcpy(results->mObjects[i].objname, "unknown");
        }
    }
    return 0;
}

int ax_model_yolopv2::post_process(const void *pstFrame, ax_joint_runner_box *crop_resize_box, sample_run_joint_results *results)
{
    AX_U32 nOutputSize = m_runner.m_attr.nOutputSize;
    AX_JOINT_IOMETA_T *pOutputsInfo = m_runner.m_attr.pOutputsInfo;
    AX_JOINT_IO_BUFFER_T *pOutputs = m_runner.m_attr.pOutputs;

    std::vector<detection::Object> proposals;
    std::vector<detection::Object> objects;

    float prob_threshold_unsigmoid = -1.0f * (float)std::log((1.0f / PROB_THRESHOLD) - 1.0f);
    for (uint32_t i = 2; i < nOutputSize; ++i)
    {
        auto &output = pOutputsInfo[i];
        auto &info = pOutputs[i];

        auto ptr = (float *)info.pVirAddr;

        int32_t stride = (1 << (i - 2)) * 8;
        generate_proposals_yolov5(stride, ptr, PROB_THRESHOLD, proposals, m_runner.m_attr.algo_width, m_runner.m_attr.algo_height, ANCHORS.data(), prob_threshold_unsigmoid, 80);
    }

    // static SimpleRingBuffer<cv::Mat> mSimpleRingBuffer_seg(SAMPLE_RINGBUFFER_CACHE_COUNT), mSimpleRingBuffer_ll(SAMPLE_RINGBUFFER_CACHE_COUNT);
    if (mSimpleRingBuffer_seg.size() == 0)
    {
        mSimpleRingBuffer_seg.resize(SAMPLE_RINGBUFFER_CACHE_COUNT);
        mSimpleRingBuffer_ll.resize(SAMPLE_RINGBUFFER_CACHE_COUNT);
    }
    auto &da_info = pOutputs[0];
    auto da_ptr = (float *)da_info.pVirAddr;
    auto &ll_info = pOutputs[1];
    auto ll_ptr = (float *)ll_info.pVirAddr;
    cv::Mat &da_seg_mask = mSimpleRingBuffer_seg.next();
    cv::Mat &ll_seg_mask = mSimpleRingBuffer_ll.next();

    detection::get_out_bbox_yolopv2(proposals, objects, da_ptr, ll_ptr, ll_seg_mask, da_seg_mask,
                                    NMS_THRESHOLD, m_runner.m_attr.algo_height, m_runner.m_attr.algo_width,
                                    HEIGHT_DET_BBOX_RESTORE, WIDTH_DET_BBOX_RESTORE);
    std::sort(objects.begin(), objects.end(),
              [&](detection::Object &a, detection::Object &b)
              {
                  return a.rect.area() > b.rect.area();
              });
    results->nObjSize = MIN(objects.size(), SAMPLE_MAX_BBOX_COUNT);
    for (size_t i = 0; i < results->nObjSize; i++)
    {
        const detection::Object &obj = objects[i];
        results->mObjects[i].bbox.x = obj.rect.x;
        results->mObjects[i].bbox.y = obj.rect.y;
        results->mObjects[i].bbox.w = obj.rect.width;
        results->mObjects[i].bbox.h = obj.rect.height;
        results->mObjects[i].label = obj.label;
        results->mObjects[i].prob = obj.prob;

        results->mObjects[i].label = 0;
        strcpy(results->mObjects[i].objname, "car");
    }

    results->bYolopv2Mask = 1;
    results->mYolopv2seg.h = da_seg_mask.rows;
    results->mYolopv2seg.w = da_seg_mask.cols;
    results->mYolopv2seg.data = da_seg_mask.data;
    results->mYolopv2ll.h = ll_seg_mask.rows;
    results->mYolopv2ll.w = ll_seg_mask.cols;
    results->mYolopv2ll.data = ll_seg_mask.data;
    return 0;
}

int ax_model_yolo_fast_body::post_process(const void *pstFrame, ax_joint_runner_box *crop_resize_box, sample_run_joint_results *results)
{
    AX_U32 nOutputSize = m_runner.m_attr.nOutputSize;
    AX_JOINT_IOMETA_T *pOutputsInfo = m_runner.m_attr.pOutputsInfo;
    AX_JOINT_IO_BUFFER_T *pOutputs = m_runner.m_attr.pOutputs;

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

    float scale_letterbox;
    int resize_rows;
    int resize_cols;
    int letterbox_rows = m_runner.m_attr.algo_height;
    int letterbox_cols = m_runner.m_attr.algo_width;
    int src_rows = HEIGHT_DET_BBOX_RESTORE;
    int src_cols = WIDTH_DET_BBOX_RESTORE;
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
        object.rect.x = data_row[2] * (float)m_runner.m_attr.algo_width;
        object.rect.y = data_row[3] * (float)m_runner.m_attr.algo_height;
        object.rect.width = (data_row[4] - data_row[2]) * (float)m_runner.m_attr.algo_width;
        object.rect.height = (data_row[5] - data_row[3]) * (float)m_runner.m_attr.algo_height;
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

    results->nObjSize = MIN(objects.size(), SAMPLE_MAX_BBOX_COUNT);
    for (size_t i = 0; i < results->nObjSize; i++)
    {
        const detection::Object &obj = objects[i];
        results->mObjects[i].bbox.x = obj.rect.x;
        results->mObjects[i].bbox.y = obj.rect.y;
        results->mObjects[i].bbox.w = obj.rect.width;
        results->mObjects[i].bbox.h = obj.rect.height;
        results->mObjects[i].label = obj.label;
        results->mObjects[i].prob = obj.prob;
        results->mObjects[i].label = 0;
        strcpy(results->mObjects[i].objname, "person");
    }
    return 0;
}

int ax_model_nanodet::post_process(const void *pstFrame, ax_joint_runner_box *crop_resize_box, sample_run_joint_results *results)
{
    std::vector<detection::Object> proposals;
    std::vector<detection::Object> objects;
    AX_U32 nOutputSize = m_runner.m_attr.nOutputSize;
    AX_JOINT_IOMETA_T *pOutputsInfo = m_runner.m_attr.pOutputsInfo;
    AX_JOINT_IO_BUFFER_T *pOutputs = m_runner.m_attr.pOutputs;

    float prob_threshold_unsigmoid = -1.0f * (float)std::log((1.0f / PROB_THRESHOLD) - 1.0f);
    for (uint32_t i = 0; i < nOutputSize; ++i)
    {
        auto &output = pOutputsInfo[i];
        auto &info = pOutputs[i];
        auto ptr = (float *)info.pVirAddr;
        // int32_t stride = (1 << i) * 8;

        static const int DEFAULT_STRIDES[] = {32, 16, 8};
        generate_proposals_nanodet(ptr, DEFAULT_STRIDES[i], m_runner.m_attr.algo_width, m_runner.m_attr.algo_height, PROB_THRESHOLD, proposals, CLASS_NUM);
    }

    detection::get_out_bbox(proposals, objects, NMS_THRESHOLD, m_runner.m_attr.algo_height, m_runner.m_attr.algo_width, HEIGHT_DET_BBOX_RESTORE, WIDTH_DET_BBOX_RESTORE);
    std::sort(objects.begin(), objects.end(),
              [&](detection::Object &a, detection::Object &b)
              {
                  return a.rect.area() > b.rect.area();
              });

    results->nObjSize = MIN(objects.size(), SAMPLE_MAX_BBOX_COUNT);
    for (size_t i = 0; i < results->nObjSize; i++)
    {
        const detection::Object &obj = objects[i];
        results->mObjects[i].bbox.x = obj.rect.x;
        results->mObjects[i].bbox.y = obj.rect.y;
        results->mObjects[i].bbox.w = obj.rect.width;
        results->mObjects[i].bbox.h = obj.rect.height;
        results->mObjects[i].label = obj.label;
        results->mObjects[i].prob = obj.prob;
        if (obj.label < CLASS_NAMES.size())
        {
            strcpy(results->mObjects[i].objname, CLASS_NAMES[obj.label].c_str());
        }
        else
        {
            strcpy(results->mObjects[i].objname, "unknown");
        }
    }
    return 0;
}
