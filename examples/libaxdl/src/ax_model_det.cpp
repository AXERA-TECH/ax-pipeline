#include "ax_model_det.hpp"
#include "../../utilities/json.hpp"

#include "../../utilities/sample_log.h"

#define ANCHOR_SIZE_PER_STRIDE 6

int ax_model_yolov5::post_process(axdl_image_t *pstFrame, axdl_bbox_t *crop_resize_box, axdl_results_t *results)
{
    std::vector<detection::Object> proposals;
    std::vector<detection::Object> objects;
    int nOutputSize = m_runner->get_num_outputs();
    const ax_runner_tensor_t *pOutputsInfo = m_runner->get_outputs_ptr();

    if ((int)ANCHORS.size() != nOutputSize * ANCHOR_SIZE_PER_STRIDE)
    {
        ALOGE("ANCHORS size failed,should be %d,got %d", nOutputSize * ANCHOR_SIZE_PER_STRIDE, (int)ANCHORS.size());
        return -1;
    }

    float prob_threshold_unsigmoid = -1.0f * (float)std::log((1.0f / PROB_THRESHOLD) - 1.0f);
    for (uint32_t i = 0; i < STRIDES.size(); ++i)
    {
        auto &output = pOutputsInfo[i];
        auto ptr = (float *)output.pVirAddr;
        generate_proposals_yolov5(STRIDES[i], ptr, PROB_THRESHOLD, proposals, get_algo_width(), get_algo_height(), ANCHORS.data(), prob_threshold_unsigmoid, CLASS_NUM);
    }

    detection::get_out_bbox(proposals, objects, NMS_THRESHOLD, get_algo_height(), get_algo_width(), HEIGHT_DET_BBOX_RESTORE, WIDTH_DET_BBOX_RESTORE);
    std::sort(objects.begin(), objects.end(),
              [&](detection::Object &a, detection::Object &b)
              {
                  return a.rect.area() > b.rect.area();
              });

    results->nObjSize = MIN(objects.size(), SAMPLE_MAX_BBOX_COUNT);
    for (int i = 0; i < results->nObjSize; i++)
    {
        const detection::Object &obj = objects[i];
        results->mObjects[i].bbox.x = obj.rect.x;
        results->mObjects[i].bbox.y = obj.rect.y;
        results->mObjects[i].bbox.w = obj.rect.width;
        results->mObjects[i].bbox.h = obj.rect.height;
        results->mObjects[i].label = obj.label;
        results->mObjects[i].prob = obj.prob;
        if (obj.label < (int)CLASS_NAMES.size())
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

int ax_model_yolov5_seg::post_process(axdl_image_t *pstFrame, axdl_bbox_t *crop_resize_box, axdl_results_t *results)
{
    std::vector<detection::Object> proposals;
    std::vector<detection::Object> objects;
    int nOutputSize = m_runner->get_num_outputs();
    const ax_runner_tensor_t *pOutputsInfo = m_runner->get_outputs_ptr();

    if ((int)ANCHORS.size() != (nOutputSize - 1) * ANCHOR_SIZE_PER_STRIDE)
    {
        ALOGE("ANCHORS size failed,should be %d,got %d", (nOutputSize - 1) * ANCHOR_SIZE_PER_STRIDE, (int)ANCHORS.size());
        return -1;
    }

    float prob_threshold_unsigmoid = -1.0f * (float)std::log((1.0f / PROB_THRESHOLD) - 1.0f);
    for (uint32_t i = 0; i < STRIDES.size(); ++i)
    {
        auto &output = pOutputsInfo[i];
        auto ptr = (float *)output.pVirAddr;
        generate_proposals_yolov5_seg(STRIDES[i], ptr, PROB_THRESHOLD, proposals, get_algo_width(), get_algo_height(), ANCHORS.data(), prob_threshold_unsigmoid);
    }
    static const int DEFAULT_MASK_PROTO_DIM = 32;
    static const int DEFAULT_MASK_SAMPLE_STRIDE = 4;
    auto &output = pOutputsInfo[3];
    auto ptr = (float *)output.pVirAddr;
    detection::get_out_bbox_mask(proposals, objects, SAMPLE_MAX_YOLOV5_MASK_OBJ_COUNT, ptr, DEFAULT_MASK_PROTO_DIM, DEFAULT_MASK_SAMPLE_STRIDE, NMS_THRESHOLD,
                                 get_algo_height(), get_algo_width(), HEIGHT_DET_BBOX_RESTORE, WIDTH_DET_BBOX_RESTORE);

    std::sort(objects.begin(), objects.end(),
              [&](detection::Object &a, detection::Object &b)
              {
                  return a.rect.area() > b.rect.area();
              });

    static SimpleRingBuffer<cv::Mat> mSimpleRingBuffer(SAMPLE_MAX_YOLOV5_MASK_OBJ_COUNT * SAMPLE_RINGBUFFER_CACHE_COUNT);
    results->nObjSize = MIN(objects.size(), SAMPLE_MAX_BBOX_COUNT);
    for (int i = 0; i < results->nObjSize; i++)
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

        if (obj.label < (int)CLASS_NAMES.size())
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

void ax_model_yolov5_seg::draw_custom(cv::Mat &image, axdl_results_t *results, float fontscale, int thickness, int offset_x, int offset_y)
{
    draw_bbox(image, results, fontscale, thickness, offset_x, offset_y);
    for (int i = 0; i < results->nObjSize; i++)
    {
        cv::Rect rect(results->mObjects[i].bbox.x * image.cols + offset_x,
                      results->mObjects[i].bbox.y * image.rows + offset_y,
                      results->mObjects[i].bbox.w * image.cols,
                      results->mObjects[i].bbox.h * image.rows);

        if (results->mObjects[i].bHasMask && results->mObjects[i].mYolov5Mask.data)
        {
            cv::Mat mask(results->mObjects[i].mYolov5Mask.h, results->mObjects[i].mYolov5Mask.w, CV_8U, results->mObjects[i].mYolov5Mask.data);
            if (!mask.empty())
            {
                cv::Mat mask_target;

                cv::resize(mask, mask_target, cv::Size(results->mObjects[i].bbox.w * image.cols, results->mObjects[i].bbox.h * image.rows), 0, 0, cv::INTER_NEAREST);

                if (results->mObjects[i].label < (int)COCO_COLORS.size())
                {
                    image(rect).setTo(COCO_COLORS[results->mObjects[i].label], mask_target);
                }
                else
                {
                    image(rect).setTo(cv::Scalar(128, 128, 128, 128), mask_target);
                }
            }
        }
    }
}

int ax_model_yolov5_face::post_process(axdl_image_t *pstFrame, axdl_bbox_t *crop_resize_box, axdl_results_t *results)
{
    if (mSimpleRingBuffer.size() == 0)
    {
        mSimpleRingBuffer.resize(SAMPLE_RINGBUFFER_CACHE_COUNT * SAMPLE_MAX_FACE_BBOX_COUNT);
    }
    std::vector<detection::Object> proposals;
    std::vector<detection::Object> objects;
    int nOutputSize = m_runner->get_num_outputs();
    const ax_runner_tensor_t *pOutputsInfo = m_runner->get_outputs_ptr();

    if ((int)ANCHORS.size() != nOutputSize * ANCHOR_SIZE_PER_STRIDE)
    {
        ALOGE("ANCHORS size failed,should be %d,got %d", nOutputSize * ANCHOR_SIZE_PER_STRIDE, (int)ANCHORS.size());
        return -1;
    }

    float prob_threshold_unsigmoid = -1.0f * (float)std::log((1.0f / PROB_THRESHOLD) - 1.0f);
    for (uint32_t i = 0; i < STRIDES.size(); ++i)
    {
        auto &output = pOutputsInfo[i];
        auto ptr = (float *)output.pVirAddr;
        generate_proposals_yolov5_face(STRIDES[i], ptr, PROB_THRESHOLD, proposals, get_algo_width(), get_algo_height(), ANCHORS.data(), prob_threshold_unsigmoid, SAMPLE_FACE_LMK_SIZE);
    }

    detection::get_out_bbox(proposals, objects, NMS_THRESHOLD, get_algo_height(), get_algo_width(), HEIGHT_DET_BBOX_RESTORE, WIDTH_DET_BBOX_RESTORE);
    std::sort(objects.begin(), objects.end(),
              [&](detection::Object &a, detection::Object &b)
              {
                  return a.rect.area() > b.rect.area();
              });

    results->nObjSize = MIN(objects.size(), SAMPLE_MAX_FACE_BBOX_COUNT);
    for (int i = 0; i < results->nObjSize; i++)
    {
        const detection::Object &obj = objects[i];
        results->mObjects[i].bbox.x = obj.rect.x;
        results->mObjects[i].bbox.y = obj.rect.y;
        results->mObjects[i].bbox.w = obj.rect.width;
        results->mObjects[i].bbox.h = obj.rect.height;
        results->mObjects[i].label = obj.label;
        results->mObjects[i].prob = obj.prob;
        results->mObjects[i].nLandmark = SAMPLE_FACE_LMK_SIZE;
        std::vector<axdl_point_t> &points = mSimpleRingBuffer.next();
        points.resize(results->mObjects[i].nLandmark);
        results->mObjects[i].landmark = points.data();
        for (size_t j = 0; j < SAMPLE_FACE_LMK_SIZE; j++)
        {
            results->mObjects[i].landmark[j].x = obj.landmark[j].x;
            results->mObjects[i].landmark[j].y = obj.landmark[j].y;
        }

        if (obj.label < (int)CLASS_NAMES.size())
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

void ax_model_yolov5_face::draw_custom(cv::Mat &image, axdl_results_t *results, float fontscale, int thickness, int offset_x, int offset_y)
{
    draw_bbox(image, results, fontscale, thickness, offset_x, offset_y);
    for (int i = 0; i < results->nObjSize; i++)
    {
        for (int j = 0; j < results->mObjects[i].nLandmark; j++)
        {
            cv::Point p(results->mObjects[i].landmark[j].x * image.cols + offset_x,
                        results->mObjects[i].landmark[j].y * image.rows + offset_y);
            cv::circle(image, p, 1, cv::Scalar(255, 0, 0, 255), 2);
        }
    }
}

int ax_model_yolov5_lisence_plate::post_process(axdl_image_t *pstFrame, axdl_bbox_t *crop_resize_box, axdl_results_t *results)
{
    if (mSimpleRingBuffer.size() == 0)
    {
        mSimpleRingBuffer.resize(SAMPLE_RINGBUFFER_CACHE_COUNT * SAMPLE_MAX_BBOX_COUNT);
    }
    std::vector<detection::Object> proposals;
    std::vector<detection::Object> objects;
    int nOutputSize = m_runner->get_num_outputs();
    const ax_runner_tensor_t *pOutputsInfo = m_runner->get_outputs_ptr();

    if ((int)ANCHORS.size() != nOutputSize * ANCHOR_SIZE_PER_STRIDE)
    {
        ALOGE("ANCHORS size failed,should be %d,got %d", nOutputSize * ANCHOR_SIZE_PER_STRIDE, (int)ANCHORS.size());
        return -1;
    }

    float prob_threshold_unsigmoid = -1.0f * (float)std::log((1.0f / PROB_THRESHOLD) - 1.0f);
    for (uint32_t i = 0; i < STRIDES.size(); ++i)
    {
        auto &output = pOutputsInfo[i];
        auto ptr = (float *)output.pVirAddr;
        generate_proposals_yolov5_face(STRIDES[i], ptr, PROB_THRESHOLD, proposals, get_algo_width(), get_algo_height(), ANCHORS.data(), prob_threshold_unsigmoid, SAMPLE_PLATE_LMK_SIZE);
    }

    detection::get_out_bbox(proposals, objects, NMS_THRESHOLD, get_algo_height(), get_algo_width(), HEIGHT_DET_BBOX_RESTORE, WIDTH_DET_BBOX_RESTORE);
    std::sort(objects.begin(), objects.end(),
              [&](detection::Object &a, detection::Object &b)
              {
                  return a.rect.area() > b.rect.area();
              });

    results->nObjSize = MIN(objects.size(), SAMPLE_MAX_BBOX_COUNT);
    for (int i = 0; i < results->nObjSize; i++)
    {
        const detection::Object &obj = objects[i];
        results->mObjects[i].bbox.x = obj.rect.x;
        results->mObjects[i].bbox.y = obj.rect.y;
        results->mObjects[i].bbox.w = obj.rect.width;
        results->mObjects[i].bbox.h = obj.rect.height;
        results->mObjects[i].label = obj.label;
        results->mObjects[i].prob = obj.prob;
        results->mObjects[i].nLandmark = SAMPLE_PLATE_LMK_SIZE;
        std::vector<axdl_point_t> &points = mSimpleRingBuffer.next();
        points.resize(results->mObjects[i].nLandmark);
        results->mObjects[i].landmark = points.data();
        for (size_t j = 0; j < SAMPLE_PLATE_LMK_SIZE; j++)
        {
            results->mObjects[i].landmark[j].x = obj.landmark[j].x;
            results->mObjects[i].landmark[j].y = obj.landmark[j].y;
            results->mObjects[i].bbox_vertices[j].x = results->mObjects[i].landmark[j].x;
            results->mObjects[i].bbox_vertices[j].y = results->mObjects[i].landmark[j].y;
        }
        results->mObjects[i].bHasBoxVertices = 1;

        std::vector<axdl_point_t> pppp(4);
        memcpy(pppp.data(), &results->mObjects[i].bbox_vertices[0], 4 * sizeof(axdl_point_t));
        std::sort(pppp.begin(), pppp.end(), [](axdl_point_t &a, axdl_point_t &b)
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
        if (obj.label < (int)CLASS_NAMES.size())
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

int ax_model_yolov6::post_process(axdl_image_t *pstFrame, axdl_bbox_t *crop_resize_box, axdl_results_t *results)
{
    std::vector<detection::Object> proposals;
    std::vector<detection::Object> objects;
    // int nOutputSize = m_runner->get_num_outputs();
    const ax_runner_tensor_t *pOutputsInfo = m_runner->get_outputs_ptr();

    // float prob_threshold_unsigmoid = -1.0f * (float)std::log((1.0f / PROB_THRESHOLD) - 1.0f);
    for (uint32_t i = 0; i < STRIDES.size(); ++i)
    {
        auto &output = pOutputsInfo[i];
        auto ptr = (float *)output.pVirAddr;
        generate_proposals_yolov6(STRIDES[i], ptr, PROB_THRESHOLD, proposals, get_algo_width(), get_algo_height(), CLASS_NUM);
    }

    detection::get_out_bbox(proposals, objects, NMS_THRESHOLD, get_algo_height(), get_algo_width(), HEIGHT_DET_BBOX_RESTORE, WIDTH_DET_BBOX_RESTORE);
    std::sort(objects.begin(), objects.end(),
              [&](detection::Object &a, detection::Object &b)
              {
                  return a.rect.area() > b.rect.area();
              });

    results->nObjSize = MIN(objects.size(), SAMPLE_MAX_BBOX_COUNT);
    for (int i = 0; i < results->nObjSize; i++)
    {
        const detection::Object &obj = objects[i];
        results->mObjects[i].bbox.x = obj.rect.x;
        results->mObjects[i].bbox.y = obj.rect.y;
        results->mObjects[i].bbox.w = obj.rect.width;
        results->mObjects[i].bbox.h = obj.rect.height;
        results->mObjects[i].label = obj.label;
        results->mObjects[i].prob = obj.prob;
        if (obj.label < (int)CLASS_NAMES.size())
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

int ax_model_yolov7::post_process(axdl_image_t *pstFrame, axdl_bbox_t *crop_resize_box, axdl_results_t *results)
{
    std::vector<detection::Object> proposals;
    std::vector<detection::Object> objects;
    int nOutputSize = m_runner->get_num_outputs();
    const ax_runner_tensor_t *pOutputsInfo = m_runner->get_outputs_ptr();

    if ((int)ANCHORS.size() != nOutputSize * ANCHOR_SIZE_PER_STRIDE)
    {
        ALOGE("ANCHORS size failed,should be %d,got %d", nOutputSize * ANCHOR_SIZE_PER_STRIDE, (int)ANCHORS.size());
        return -1;
    }

    // float prob_threshold_unsigmoid = -1.0f * (float)std::log((1.0f / PROB_THRESHOLD) - 1.0f);
    for (uint32_t i = 0; i < STRIDES.size(); ++i)
    {
        auto &output = pOutputsInfo[i];
        auto ptr = (float *)output.pVirAddr;
        generate_proposals_yolov7(STRIDES[i], ptr, PROB_THRESHOLD, proposals, get_algo_width(), get_algo_height(), ANCHORS.data() + i * ANCHOR_SIZE_PER_STRIDE, CLASS_NUM);
    }

    detection::get_out_bbox(proposals, objects, NMS_THRESHOLD, get_algo_height(), get_algo_width(), HEIGHT_DET_BBOX_RESTORE, WIDTH_DET_BBOX_RESTORE);
    std::sort(objects.begin(), objects.end(),
              [&](detection::Object &a, detection::Object &b)
              {
                  return a.rect.area() > b.rect.area();
              });

    results->nObjSize = MIN(objects.size(), SAMPLE_MAX_BBOX_COUNT);
    for (int i = 0; i < results->nObjSize; i++)
    {
        const detection::Object &obj = objects[i];
        results->mObjects[i].bbox.x = obj.rect.x;
        results->mObjects[i].bbox.y = obj.rect.y;
        results->mObjects[i].bbox.w = obj.rect.width;
        results->mObjects[i].bbox.h = obj.rect.height;
        results->mObjects[i].label = obj.label;
        results->mObjects[i].prob = obj.prob;
        if (obj.label < (int)CLASS_NAMES.size())
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

int ax_model_yolov7_face::post_process(axdl_image_t *pstFrame, axdl_bbox_t *crop_resize_box, axdl_results_t *results)
{
    if (mSimpleRingBuffer.size() == 0)
    {
        mSimpleRingBuffer.resize(SAMPLE_RINGBUFFER_CACHE_COUNT * SAMPLE_MAX_FACE_BBOX_COUNT);
    }
    std::vector<detection::Object> proposals;
    std::vector<detection::Object> objects;
    int nOutputSize = m_runner->get_num_outputs();
    const ax_runner_tensor_t *pOutputsInfo = m_runner->get_outputs_ptr();

    if ((int)ANCHORS.size() != nOutputSize * ANCHOR_SIZE_PER_STRIDE)
    {
        ALOGE("ANCHORS size failed,should be %d,got %d", nOutputSize * ANCHOR_SIZE_PER_STRIDE, (int)ANCHORS.size());
        return -1;
    }

    float prob_threshold_unsigmoid = -1.0f * (float)std::log((1.0f / PROB_THRESHOLD) - 1.0f);
    for (uint32_t i = 0; i < STRIDES.size(); ++i)
    {
        auto &output = pOutputsInfo[i];
        auto ptr = (float *)output.pVirAddr;
        generate_proposals_yolov7_face(STRIDES[i], ptr, PROB_THRESHOLD, proposals, get_algo_width(), get_algo_height(), ANCHORS.data(), prob_threshold_unsigmoid);
    }

    detection::get_out_bbox(proposals, objects, NMS_THRESHOLD, get_algo_height(), get_algo_width(), HEIGHT_DET_BBOX_RESTORE, WIDTH_DET_BBOX_RESTORE);
    std::sort(objects.begin(), objects.end(),
              [&](detection::Object &a, detection::Object &b)
              {
                  return a.rect.area() > b.rect.area();
              });

    results->nObjSize = MIN(objects.size(), SAMPLE_MAX_FACE_BBOX_COUNT);
    for (int i = 0; i < results->nObjSize; i++)
    {
        const detection::Object &obj = objects[i];
        results->mObjects[i].bbox.x = obj.rect.x;
        results->mObjects[i].bbox.y = obj.rect.y;
        results->mObjects[i].bbox.w = obj.rect.width;
        results->mObjects[i].bbox.h = obj.rect.height;
        results->mObjects[i].label = obj.label;
        results->mObjects[i].prob = obj.prob;
        results->mObjects[i].nLandmark = SAMPLE_FACE_LMK_SIZE;
        std::vector<axdl_point_t> &points = mSimpleRingBuffer.next();
        points.resize(results->mObjects[i].nLandmark);
        results->mObjects[i].landmark = points.data();
        for (size_t j = 0; j < SAMPLE_FACE_LMK_SIZE; j++)
        {
            results->mObjects[i].landmark[j].x = obj.landmark[j].x;
            results->mObjects[i].landmark[j].y = obj.landmark[j].y;
        }
        if (obj.label < (int)CLASS_NAMES.size())
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

int ax_model_yolov7_plam_hand::post_process(axdl_image_t *pstFrame, axdl_bbox_t *crop_resize_box, axdl_results_t *results)
{
    std::vector<detection::PalmObject> proposals;
    std::vector<detection::PalmObject> objects;
    int nOutputSize = m_runner->get_num_outputs();
    const ax_runner_tensor_t *pOutputsInfo = m_runner->get_outputs_ptr();

    if ((int)ANCHORS.size() != nOutputSize * ANCHOR_SIZE_PER_STRIDE)
    {
        ALOGE("ANCHORS size failed,should be %d,got %d", nOutputSize * ANCHOR_SIZE_PER_STRIDE, (int)ANCHORS.size());
        return -1;
    }

    float prob_threshold_unsigmoid = -1.0f * (float)std::log((1.0f / PROB_THRESHOLD) - 1.0f);
    for (uint32_t i = 0; i < STRIDES.size(); ++i)
    {
        auto &output = pOutputsInfo[i];
        auto ptr = (float *)output.pVirAddr;
        generate_proposals_yolov7_palm(STRIDES[i], ptr, PROB_THRESHOLD, proposals, get_algo_width(), get_algo_height(), ANCHORS.data(), prob_threshold_unsigmoid);
    }

    detection::get_out_bbox_palm(proposals, objects, NMS_THRESHOLD, get_algo_height(), get_algo_width(), HEIGHT_DET_BBOX_RESTORE, WIDTH_DET_BBOX_RESTORE);
    std::sort(objects.begin(), objects.end(),
              [&](detection::PalmObject &a, detection::PalmObject &b)
              {
                  return a.rect.area() > b.rect.area();
              });

    results->nObjSize = MIN(objects.size(), SAMPLE_MAX_HAND_BBOX_COUNT);
    for (int i = 0; i < results->nObjSize; i++)
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

int ax_model_plam_hand::post_process(axdl_image_t *pstFrame, axdl_bbox_t *crop_resize_box, axdl_results_t *results)
{
    static const int map_size[2] = {24, 12};
    static const int strides[2] = {8, 16};
    static const int anchor_size[2] = {2, 6};
    static const float anchor_offset[2] = {0.5f, 0.5f};
    std::vector<detection::PalmObject> proposals;
    std::vector<detection::PalmObject> objects;

    auto &bboxes_info = m_runner->get_output(0);
    auto bboxes_ptr = (float *)bboxes_info.pVirAddr;
    auto &scores_info = m_runner->get_output(1);
    auto scores_ptr = (float *)scores_info.pVirAddr;
    float prob_threshold_unsigmoid = -1.0f * (float)std::log((1.0f / PROB_THRESHOLD) - 1.0f);
    detection::generate_proposals_palm(proposals, PROB_THRESHOLD, get_algo_width(), get_algo_height(), scores_ptr, bboxes_ptr, 2, strides, anchor_size, anchor_offset, map_size, prob_threshold_unsigmoid);

    detection::get_out_bbox_palm(proposals, objects, NMS_THRESHOLD, get_algo_height(), get_algo_width(), HEIGHT_DET_BBOX_RESTORE, WIDTH_DET_BBOX_RESTORE);

    std::sort(objects.begin(), objects.end(),
              [&](detection::PalmObject &a, detection::PalmObject &b)
              {
                  return a.rect.area() > b.rect.area();
              });

    results->nObjSize = MIN(objects.size(), SAMPLE_MAX_HAND_BBOX_COUNT);
    for (int i = 0; i < results->nObjSize; i++)
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

int ax_model_yolox::post_process(axdl_image_t *pstFrame, axdl_bbox_t *crop_resize_box, axdl_results_t *results)
{
    std::vector<detection::Object> proposals;
    std::vector<detection::Object> objects;
    // int nOutputSize = m_runner->get_num_outputs();
    const ax_runner_tensor_t *pOutputsInfo = m_runner->get_outputs_ptr();

    // float prob_threshold_unsigmoid = -1.0f * (float)std::log((1.0f / PROB_THRESHOLD) - 1.0f);
    for (uint32_t i = 0; i < STRIDES.size(); ++i)
    {
        auto &output = pOutputsInfo[i];
        auto ptr = (float *)output.pVirAddr;
        generate_proposals_yolox(STRIDES[i], ptr, PROB_THRESHOLD, proposals, get_algo_width(), get_algo_height(), CLASS_NUM);
    }

    detection::get_out_bbox(proposals, objects, NMS_THRESHOLD, get_algo_height(), get_algo_width(), HEIGHT_DET_BBOX_RESTORE, WIDTH_DET_BBOX_RESTORE);
    std::sort(objects.begin(), objects.end(),
              [&](detection::Object &a, detection::Object &b)
              {
                  return a.rect.area() > b.rect.area();
              });

    results->nObjSize = MIN(objects.size(), SAMPLE_MAX_BBOX_COUNT);
    for (int i = 0; i < results->nObjSize; i++)
    {
        const detection::Object &obj = objects[i];
        results->mObjects[i].bbox.x = obj.rect.x;
        results->mObjects[i].bbox.y = obj.rect.y;
        results->mObjects[i].bbox.w = obj.rect.width;
        results->mObjects[i].bbox.h = obj.rect.height;
        results->mObjects[i].label = obj.label;
        results->mObjects[i].prob = obj.prob;
        if (obj.label < (int)CLASS_NAMES.size())
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

int ax_model_yoloxppl::post_process(axdl_image_t *pstFrame, axdl_bbox_t *crop_resize_box, axdl_results_t *results)
{
    std::vector<detection::Object> proposals;
    std::vector<detection::Object> objects;
    int nOutputSize = m_runner->get_num_outputs();
    const ax_runner_tensor_t *pOutputsInfo = m_runner->get_outputs_ptr();

    // float prob_threshold_unsigmoid = -1.0f * (float)std::log((1.0f / PROB_THRESHOLD) - 1.0f);
    for (uint32_t i = 0; i < nOutputSize; ++i)
    {
        auto &output = pOutputsInfo[i];
        auto ptr = (float *)output.pVirAddr;
        std::vector<detection::GridAndStride> grid_stride;
        int wxc = output.vShape[2] * output.vShape[3];
        static std::vector<std::vector<int>> stride_ppl = {{8}, {16}, {32}};
        generate_grids_and_stride(get_algo_width(), get_algo_height(), stride_ppl[i], grid_stride);
        generate_yolox_proposals(grid_stride, ptr, PROB_THRESHOLD, proposals, wxc, CLASS_NUM);
    }

    detection::get_out_bbox(proposals, objects, NMS_THRESHOLD, get_algo_height(), get_algo_width(), HEIGHT_DET_BBOX_RESTORE, WIDTH_DET_BBOX_RESTORE);
    std::sort(objects.begin(), objects.end(),
              [&](detection::Object &a, detection::Object &b)
              {
                  return a.rect.area() > b.rect.area();
              });

    results->nObjSize = MIN(objects.size(), SAMPLE_MAX_BBOX_COUNT);
    for (int i = 0; i < results->nObjSize; i++)
    {
        const detection::Object &obj = objects[i];
        results->mObjects[i].bbox.x = obj.rect.x;
        results->mObjects[i].bbox.y = obj.rect.y;
        results->mObjects[i].bbox.w = obj.rect.width;
        results->mObjects[i].bbox.h = obj.rect.height;
        results->mObjects[i].label = obj.label;
        results->mObjects[i].prob = obj.prob;
        if (obj.label < (int)CLASS_NAMES.size())
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

int ax_model_yolopv2::post_process(axdl_image_t *pstFrame, axdl_bbox_t *crop_resize_box, axdl_results_t *results)
{
    int nOutputSize = m_runner->get_num_outputs();
    const ax_runner_tensor_t *pOutputsInfo = m_runner->get_outputs_ptr();

    if ((int)ANCHORS.size() != (nOutputSize - 2) * ANCHOR_SIZE_PER_STRIDE)
    {
        ALOGE("ANCHORS size failed,should be %d,got %d", (nOutputSize - 2) * ANCHOR_SIZE_PER_STRIDE, (int)ANCHORS.size());
        return -1;
    }

    std::vector<detection::Object> proposals;
    std::vector<detection::Object> objects;

    float prob_threshold_unsigmoid = -1.0f * (float)std::log((1.0f / PROB_THRESHOLD) - 1.0f);
    for (uint32_t i = 0; i < STRIDES.size(); ++i)
    {
        auto &output = pOutputsInfo[i + 2];
        auto ptr = (float *)output.pVirAddr;
        generate_proposals_yolov5(STRIDES[i], ptr, PROB_THRESHOLD, proposals, get_algo_width(), get_algo_height(), ANCHORS.data(), prob_threshold_unsigmoid, 80);
    }

    // static SimpleRingBuffer<cv::Mat> mSimpleRingBuffer_seg(SAMPLE_RINGBUFFER_CACHE_COUNT), mSimpleRingBuffer_ll(SAMPLE_RINGBUFFER_CACHE_COUNT);
    if (mSimpleRingBuffer_seg.size() == 0)
    {
        mSimpleRingBuffer_seg.resize(SAMPLE_RINGBUFFER_CACHE_COUNT);
        mSimpleRingBuffer_ll.resize(SAMPLE_RINGBUFFER_CACHE_COUNT);
    }
    auto &da_info = m_runner->get_output(0);
    auto da_ptr = (float *)da_info.pVirAddr;
    auto &ll_info = m_runner->get_output(1);
    auto ll_ptr = (float *)ll_info.pVirAddr;
    cv::Mat &da_seg_mask = mSimpleRingBuffer_seg.next();
    cv::Mat &ll_seg_mask = mSimpleRingBuffer_ll.next();

    detection::get_out_bbox_yolopv2(proposals, objects, da_ptr, ll_ptr, ll_seg_mask, da_seg_mask,
                                    NMS_THRESHOLD, get_algo_height(), get_algo_width(),
                                    HEIGHT_DET_BBOX_RESTORE, WIDTH_DET_BBOX_RESTORE);
    std::sort(objects.begin(), objects.end(),
              [&](detection::Object &a, detection::Object &b)
              {
                  return a.rect.area() > b.rect.area();
              });
    results->nObjSize = MIN(objects.size(), SAMPLE_MAX_BBOX_COUNT);
    for (int i = 0; i < results->nObjSize; i++)
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

void ax_model_yolopv2::draw_custom(cv::Mat &image, axdl_results_t *results, float fontscale, int thickness, int offset_x, int offset_y)
{
    if (results->bYolopv2Mask && results->mYolopv2ll.data && results->mYolopv2seg.data)
    {
        if (base_canvas.empty() || base_canvas.rows * base_canvas.cols < image.rows * image.cols)
        {
            base_canvas = cv::Mat(image.rows, image.cols, CV_8UC1);
        }
        cv::Mat tmp(image.rows, image.cols, CV_8UC1, base_canvas.data);

        cv::Mat seg_mask(results->mYolopv2seg.h, results->mYolopv2seg.w, CV_8UC1, results->mYolopv2seg.data);
        cv::resize(seg_mask, tmp, cv::Size(image.cols, image.rows), 0, 0, cv::INTER_NEAREST);
        image.setTo(cv::Scalar(66, 0, 0, 128), tmp);

        cv::Mat ll_mask(results->mYolopv2ll.h, results->mYolopv2ll.w, CV_8UC1, results->mYolopv2ll.data);
        cv::resize(ll_mask, tmp, cv::Size(image.cols, image.rows), 0, 0, cv::INTER_NEAREST);
        image.setTo(cv::Scalar(66, 0, 128, 0), tmp);
    }
    draw_bbox(image, results, fontscale, thickness, offset_x, offset_y);
}

int ax_model_yolo_fast_body::post_process(axdl_image_t *pstFrame, axdl_bbox_t *crop_resize_box, axdl_results_t *results)
{
    int nOutputSize = m_runner->get_num_outputs();
    const ax_runner_tensor_t *pOutputsInfo = m_runner->get_outputs_ptr();

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

        auto ptr = (float *)output.pVirAddr;

        yolo_inputs[i].batch = output.vShape[0];
        yolo_inputs[i].h = output.vShape[1];
        yolo_inputs[i].w = output.vShape[2];
        yolo_inputs[i].c = output.vShape[3];
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
    int letterbox_rows = get_algo_height();
    int letterbox_cols = get_algo_width();
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
        object.rect.x = data_row[2] * (float)get_algo_width();
        object.rect.y = data_row[3] * (float)get_algo_height();
        object.rect.width = (data_row[4] - data_row[2]) * (float)get_algo_width();
        object.rect.height = (data_row[5] - data_row[3]) * (float)get_algo_height();
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
    for (int i = 0; i < results->nObjSize; i++)
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

int ax_model_nanodet::post_process(axdl_image_t *pstFrame, axdl_bbox_t *crop_resize_box, axdl_results_t *results)
{
    std::vector<detection::Object> proposals;
    std::vector<detection::Object> objects;
    // int nOutputSize = m_runner->get_num_outputs();
    const ax_runner_tensor_t *pOutputsInfo = m_runner->get_outputs_ptr();

    // float prob_threshold_unsigmoid = -1.0f * (float)std::log((1.0f / PROB_THRESHOLD) - 1.0f);
    for (uint32_t i = 0; i < STRIDES.size(); ++i)
    {
        auto &output = pOutputsInfo[i];
        auto ptr = (float *)output.pVirAddr;
        // int32_t stride = (1 << i) * 8;

        // static const int DEFAULT_STRIDES[] = {32, 16, 8};
        generate_proposals_nanodet(ptr, STRIDES[i], get_algo_width(), get_algo_height(), PROB_THRESHOLD, proposals, CLASS_NUM);
    }

    detection::get_out_bbox(proposals, objects, NMS_THRESHOLD, get_algo_height(), get_algo_width(), HEIGHT_DET_BBOX_RESTORE, WIDTH_DET_BBOX_RESTORE);
    std::sort(objects.begin(), objects.end(),
              [&](detection::Object &a, detection::Object &b)
              {
                  return a.rect.area() > b.rect.area();
              });

    results->nObjSize = MIN(objects.size(), SAMPLE_MAX_BBOX_COUNT);
    for (int i = 0; i < results->nObjSize; i++)
    {
        const detection::Object &obj = objects[i];
        results->mObjects[i].bbox.x = obj.rect.x;
        results->mObjects[i].bbox.y = obj.rect.y;
        results->mObjects[i].bbox.w = obj.rect.width;
        results->mObjects[i].bbox.h = obj.rect.height;
        results->mObjects[i].label = obj.label;
        results->mObjects[i].prob = obj.prob;
        if (obj.label < (int)CLASS_NAMES.size())
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

int ax_model_scrfd::post_process(axdl_image_t *pstFrame, axdl_bbox_t *crop_resize_box, axdl_results_t *results)
{
    if (mSimpleRingBuffer.size() == 0)
    {
        mSimpleRingBuffer.resize(SAMPLE_RINGBUFFER_CACHE_COUNT * SAMPLE_MAX_FACE_BBOX_COUNT);
    }
    std::vector<detection::Object> proposals;
    std::vector<detection::Object> objects;
    int nOutputSize = m_runner->get_num_outputs();
    const ax_runner_tensor_t *pOutputsInfo = m_runner->get_outputs_ptr();

    std::map<std::string, float *> output_map;
    for (uint32_t i = 0; i < nOutputSize; i++)
    {
        output_map[pOutputsInfo[i].sName] = (float *)pOutputsInfo[i].pVirAddr;
    }
    static const char *score_pred_name[] = {
        "score_8", "score_16", "score_32"};
    static const char *bbox_pred_name[] = {
        "bbox_8", "bbox_16", "bbox_32"};
    static const char *kps_pred_name[] = {
        "kps_8", "kps_16", "kps_32"};
    float prob_threshold_unsigmoid = -1.0f * (float)std::log((1.0f / PROB_THRESHOLD) - 1.0f);
    for (int stride_index = 0; stride_index < STRIDES.size(); stride_index++)
    {
        float *score_pred = output_map[score_pred_name[stride_index]];
        float *bbox_pred = output_map[bbox_pred_name[stride_index]];
        float *kps_pred = output_map[kps_pred_name[stride_index]];

        generate_proposals_scrfd(STRIDES[stride_index], score_pred, bbox_pred, kps_pred, prob_threshold_unsigmoid, proposals, get_algo_height(), get_algo_width());
    }

    detection::get_out_bbox(proposals, objects, NMS_THRESHOLD, get_algo_height(), get_algo_width(), HEIGHT_DET_BBOX_RESTORE, WIDTH_DET_BBOX_RESTORE);
    std::sort(objects.begin(), objects.end(),
              [&](detection::Object &a, detection::Object &b)
              {
                  return a.rect.area() > b.rect.area();
              });

    results->nObjSize = MIN(objects.size(), SAMPLE_MAX_FACE_BBOX_COUNT);
    for (int i = 0; i < results->nObjSize; i++)
    {
        const detection::Object &obj = objects[i];
        results->mObjects[i].bbox.x = obj.rect.x;
        results->mObjects[i].bbox.y = obj.rect.y;
        results->mObjects[i].bbox.w = obj.rect.width;
        results->mObjects[i].bbox.h = obj.rect.height;
        results->mObjects[i].label = obj.label;
        results->mObjects[i].prob = obj.prob;
        results->mObjects[i].nLandmark = SAMPLE_FACE_LMK_SIZE;
        std::vector<axdl_point_t> &points = mSimpleRingBuffer.next();
        points.resize(results->mObjects[i].nLandmark);
        results->mObjects[i].landmark = points.data();
        for (size_t j = 0; j < SAMPLE_FACE_LMK_SIZE; j++)
        {
            results->mObjects[i].landmark[j].x = obj.landmark[j].x;
            results->mObjects[i].landmark[j].y = obj.landmark[j].y;
        }

        if (obj.label < (int)CLASS_NAMES.size())
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

int ax_model_yolov8::post_process(axdl_image_t *pstFrame, axdl_bbox_t *crop_resize_box, axdl_results_t *results)
{
    std::vector<detection::Object> proposals;
    std::vector<detection::Object> objects;
    // int nOutputSize = m_runner->get_num_outputs();
    const ax_runner_tensor_t *pOutputsInfo = m_runner->get_outputs_ptr();

    float prob_threshold_unsigmoid = -1.0f * (float)std::log((1.0f / PROB_THRESHOLD) - 1.0f);
    for (uint32_t i = 0; i < STRIDES.size(); ++i)
    {
        auto &dfl_info = pOutputsInfo[i];
        auto dfl_ptr = (float *)dfl_info.pVirAddr;
        auto &cls_info = pOutputsInfo[i + 3];
        auto cls_ptr = (float *)cls_info.pVirAddr;
        auto &cls_idx_info = pOutputsInfo[i + 6];
        auto cls_idx_ptr = (float *)cls_idx_info.pVirAddr;
        generate_proposals_yolov8(STRIDES[i], dfl_ptr, cls_ptr, cls_idx_ptr, prob_threshold_unsigmoid, proposals, get_algo_width(), get_algo_height(), CLASS_NUM);
    }

    detection::get_out_bbox(proposals, objects, NMS_THRESHOLD, get_algo_height(), get_algo_width(), HEIGHT_DET_BBOX_RESTORE, WIDTH_DET_BBOX_RESTORE);
    std::sort(objects.begin(), objects.end(),
              [&](detection::Object &a, detection::Object &b)
              {
                  return a.rect.area() > b.rect.area();
              });

    results->nObjSize = MIN(objects.size(), SAMPLE_MAX_BBOX_COUNT);
    for (int i = 0; i < results->nObjSize; i++)
    {
        const detection::Object &obj = objects[i];
        results->mObjects[i].bbox.x = obj.rect.x;
        results->mObjects[i].bbox.y = obj.rect.y;
        results->mObjects[i].bbox.w = obj.rect.width;
        results->mObjects[i].bbox.h = obj.rect.height;
        results->mObjects[i].label = obj.label;
        results->mObjects[i].prob = obj.prob;
        if (obj.label < (int)CLASS_NAMES.size())
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

int ax_model_yolov8_seg::post_process(axdl_image_t *pstFrame, axdl_bbox_t *crop_resize_box, axdl_results_t *results)
{
    std::vector<detection::Object> proposals;
    std::vector<detection::Object> objects;
    // int nOutputSize = m_runner->get_num_outputs();
    const ax_runner_tensor_t *pOutputsInfo = m_runner->get_outputs_ptr();

    float prob_threshold_unsigmoid = -1.0f * (float)std::log((1.0f / PROB_THRESHOLD) - 1.0f);
    for (uint32_t i = 0; i < STRIDES.size(); ++i)
    {
        auto &dfl_info = pOutputsInfo[i];
        auto dfl_ptr = (float *)dfl_info.pVirAddr;
        auto &cls_info = pOutputsInfo[i + 3];
        auto cls_ptr = (float *)cls_info.pVirAddr;
        auto &cls_idx_info = pOutputsInfo[i + 6];
        auto cls_idx_ptr = (float *)cls_idx_info.pVirAddr;
        generate_proposals_yolov8_seg(STRIDES[i], dfl_ptr, cls_ptr, cls_idx_ptr, prob_threshold_unsigmoid, proposals, get_algo_width(), get_algo_height());
    }
    static const int DEFAULT_MASK_PROTO_DIM = 32;
    static const int DEFAULT_MASK_SAMPLE_STRIDE = 4;
    auto &output = pOutputsInfo[9];
    auto ptr = (float *)output.pVirAddr;
    detection::get_out_bbox_mask(proposals, objects, SAMPLE_MAX_YOLOV5_MASK_OBJ_COUNT, ptr, DEFAULT_MASK_PROTO_DIM, DEFAULT_MASK_SAMPLE_STRIDE, NMS_THRESHOLD,
                                 get_algo_height(), get_algo_width(), HEIGHT_DET_BBOX_RESTORE, WIDTH_DET_BBOX_RESTORE);

    std::sort(objects.begin(), objects.end(),
              [&](detection::Object &a, detection::Object &b)
              {
                  return a.rect.area() > b.rect.area();
              });

    static SimpleRingBuffer<cv::Mat> mSimpleRingBuffer(SAMPLE_MAX_YOLOV5_MASK_OBJ_COUNT * SAMPLE_RINGBUFFER_CACHE_COUNT);
    results->nObjSize = MIN(objects.size(), SAMPLE_MAX_BBOX_COUNT);
    for (int i = 0; i < results->nObjSize; i++)
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

        if (obj.label < (int)CLASS_NAMES.size())
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
