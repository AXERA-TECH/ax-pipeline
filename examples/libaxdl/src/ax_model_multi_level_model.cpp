#include "ax_model_multi_level_model.hpp"

#include "../../utilities/json.hpp"
#include "../../utilities/sample_log.h"

#include "opencv2/opencv.hpp"

#include "fstream"
#include "ax_sys_api.h"

#include "base/pose.hpp"

static inline void draw_pose_result(cv::Mat &img, libaxdl_object_t *pObj, std::vector<pose::skeleton> &pairs, int joints_num, int offset_x, int offset_y)
{
    for (int i = 0; i < joints_num; i++)
    {
        cv::circle(img, cv::Point(pObj->landmark[i].x * img.cols + offset_x, pObj->landmark[i].y * img.rows + offset_y), 4, cv::Scalar(0, 255, 0), cv::FILLED);
    }

    cv::Scalar color;
    cv::Point pt1;
    cv::Point pt2;

    for (auto &element : pairs)
    {
        switch (element.left_right_neutral)
        {
        case 0:
            color = cv::Scalar(255, 255, 0, 0);
            break;
        case 1:
            color = cv::Scalar(255, 0, 0, 255);
            break;
        case 2:
            color = cv::Scalar(255, 0, 255, 0);
            break;
        case 3:
            color = cv::Scalar(255, 255, 0, 255);
            break;
        default:
            color = cv::Scalar(255, 255, 255, 255);
        }

        int x1 = (int)(pObj->landmark[element.connection[0]].x * img.cols) + offset_x;
        int y1 = (int)(pObj->landmark[element.connection[0]].y * img.rows) + offset_y;
        int x2 = (int)(pObj->landmark[element.connection[1]].x * img.cols) + offset_x;
        int y2 = (int)(pObj->landmark[element.connection[1]].y * img.rows) + offset_y;

        x1 = std::max(std::min(x1, (img.cols - 1)), 0);
        y1 = std::max(std::min(y1, (img.rows - 1)), 0);
        x2 = std::max(std::min(x2, (img.cols - 1)), 0);
        y2 = std::max(std::min(y2, (img.rows - 1)), 0);

        pt1 = cv::Point(x1, y1);
        pt2 = cv::Point(x2, y2);
        cv::line(img, pt1, pt2, color, 2);
    }
}

void ax_model_human_pose_axppl::draw_custom(cv::Mat &image, libaxdl_results_t *results, float fontscale, int thickness, int offset_x, int offset_y)
{
    draw_bbox(image, results, fontscale, thickness, offset_x, offset_y);
    for (int i = 0; i < results->nObjSize; i++)
    {
        static std::vector<pose::skeleton> pairs = {{15, 13, 0},
                                                    {13, 11, 0},
                                                    {16, 14, 0},
                                                    {14, 12, 0},
                                                    {11, 12, 0},
                                                    {5, 11, 0},
                                                    {6, 12, 0},
                                                    {5, 6, 0},
                                                    {5, 7, 0},
                                                    {6, 8, 0},
                                                    {7, 9, 0},
                                                    {8, 10, 0},
                                                    {1, 2, 0},
                                                    {0, 1, 0},
                                                    {0, 2, 0},
                                                    {1, 3, 0},
                                                    {2, 4, 0},
                                                    {0, 5, 0},
                                                    {0, 6, 0}};
        if (results->mObjects[i].nLandmark == SAMPLE_BODY_LMK_SIZE)
        {
            draw_pose_result(image, &results->mObjects[i], pairs, SAMPLE_BODY_LMK_SIZE, offset_x, offset_y);
        }
    }
}

int ax_model_human_pose_axppl::inference(const void *pstFrame, ax_joint_runner_box_t *crop_resize_box, libaxdl_results_t *results)
{
    int ret = model_0->inference(pstFrame, crop_resize_box, results);
    if (ret)
        return ret;
    std::vector<int> idxs;
    for (int i = 0; i < results->nObjSize; i++)
    {
        auto it = std::find(CLASS_IDS.begin(), CLASS_IDS.end(), results->mObjects[i].label);
        if (it != CLASS_IDS.end())
        {
            idxs.push_back(i);
        }
    }
    int count = MIN(idxs.size(), SAMPLE_MAX_POSE_COUNT);
    for (int i = 0; i < count; i++)
    {
        int idx = idxs[i];
        model_1->set_current_index(idx);
        ret = model_1->inference(pstFrame, crop_resize_box, results);
        if (ret)
            return ret;
        if (idx != 0)
        {
            memcpy(&results->mObjects[i], &results->mObjects[idx], sizeof(libaxdl_object_t));
        }
    }
    results->nObjSize = count;
    return 0;
}

void ax_model_animal_pose_hrnet::draw_custom(cv::Mat &image, libaxdl_results_t *results, float fontscale, int thickness, int offset_x, int offset_y)
{
    draw_bbox(image, results, fontscale, thickness, offset_x, offset_y);
    for (int i = 0; i < results->nObjSize; i++)
    {
        static std::vector<pose::skeleton> pairs = {{19, 15, 0},
                                                    {18, 14, 0},
                                                    {17, 13, 0},
                                                    {16, 12, 0},
                                                    {15, 11, 0},
                                                    {14, 10, 0},
                                                    {13, 9, 0},
                                                    {12, 8, 0},
                                                    {11, 6, 0},
                                                    {10, 6, 0},
                                                    {9, 7, 0},
                                                    {8, 7, 0},
                                                    {6, 7, 0},
                                                    {7, 5, 0},
                                                    {5, 4, 0},
                                                    {0, 2, 0},
                                                    {1, 3, 0},
                                                    {0, 1, 0},
                                                    {0, 4, 0},
                                                    {1, 4, 0}};
        if (results->mObjects[i].nLandmark == SAMPLE_ANIMAL_LMK_SIZE)
        {
            draw_pose_result(image, &results->mObjects[i], pairs, SAMPLE_ANIMAL_LMK_SIZE, offset_x, offset_y);
        }
    }
}

void ax_model_hand_pose::draw_custom(cv::Mat &image, libaxdl_results_t *results, float fontscale, int thickness, int offset_x, int offset_y)
{
    draw_bbox(image, results, fontscale, thickness, offset_x, offset_y);
    for (int i = 0; i < results->nObjSize; i++)
    {
        static std::vector<pose::skeleton> hand_pairs = {{0, 1, 0},
                                                         {1, 2, 0},
                                                         {2, 3, 0},
                                                         {3, 4, 0},
                                                         {0, 5, 1},
                                                         {5, 6, 1},
                                                         {6, 7, 1},
                                                         {7, 8, 1},
                                                         {0, 9, 2},
                                                         {9, 10, 2},
                                                         {10, 11, 2},
                                                         {11, 12, 2},
                                                         {0, 13, 3},
                                                         {13, 14, 3},
                                                         {14, 15, 3},
                                                         {15, 16, 3},
                                                         {0, 17, 4},
                                                         {17, 18, 4},
                                                         {18, 19, 4},
                                                         {19, 20, 4}};
        if (results->mObjects[i].nLandmark == SAMPLE_HAND_LMK_SIZE)
        {
            draw_pose_result(image, &results->mObjects[i], hand_pairs, SAMPLE_HAND_LMK_SIZE, offset_x, offset_y);
        }
    }
}

void ax_model_hand_pose::deinit()
{
    model_1->deinit();
    model_0->deinit();
    AX_SYS_MemFree(pstFrame_RGB.pPhy, pstFrame_RGB.pVir);
}

int ax_model_hand_pose::inference(const void *pstFrame, ax_joint_runner_box_t *crop_resize_box, libaxdl_results_t *results)
{
    if (!pstFrame_RGB.pVir)
    {
        memcpy(&pstFrame_RGB, pstFrame, sizeof(AX_NPU_CV_Image));
        pstFrame_RGB.eDtype = AX_NPU_CV_FDT_BGR;
        AX_SYS_MemAlloc(&pstFrame_RGB.pPhy, (void **)&pstFrame_RGB.pVir, pstFrame_RGB.nSize, 0x100, NULL);
    }
    pstFrame_RGB.eDtype = AX_NPU_CV_FDT_BGR;
    AX_NPU_CV_CSC(AX_NPU_MODEL_TYPE_1_1_1, (AX_NPU_CV_Image *)pstFrame, &pstFrame_RGB);
    pstFrame_RGB.eDtype = AX_NPU_CV_FDT_RGB;

    int ret = model_0->inference(&pstFrame_RGB, crop_resize_box, results);
    if (ret)
        return ret;

    for (size_t i = 0; i < results->nObjSize; i++)
    {
        model_1->set_current_index(i);
        ret = model_1->inference(pstFrame, crop_resize_box, results);
        if (ret)
            return ret;
    }
    return 0;
}

int ax_model_face_recognition::inference(const void *pstFrame, ax_joint_runner_box_t *crop_resize_box, libaxdl_results_t *results)
{
    if (!b_face_database_init)
    {
        for (size_t i = 0; i < face_register_ids.size(); i++)
        {
            auto &faceid = face_register_ids[i];
            cv::Mat image = cv::imread(faceid.path);
            if (image.empty())
            {
                ALOGE("image %s cannot open,name %s register failed", faceid.path.c_str(), faceid.name.c_str());
                continue;
            }
            AX_NPU_CV_Image npu_image;
            npu_image.eDtype = AX_NPU_CV_FDT_RGB;
            npu_image.nHeight = image.rows;
            npu_image.nWidth = image.cols;
            npu_image.tStride.nW = npu_image.nWidth;
            npu_image.nSize = npu_image.nWidth * npu_image.nHeight * 3;
            AX_SYS_MemAlloc((AX_U64 *)&npu_image.pPhy, (void **)&npu_image.pVir, npu_image.nSize, 0x100, (AX_S8 *)"SAMPLE-CV");
            memcpy(npu_image.pVir, image.data, npu_image.nSize);

            libaxdl_results_t Results = {0};
            int width, height;
            model_0->get_det_restore_resolution(width, height);
            model_0->set_det_restore_resolution(npu_image.nWidth, npu_image.nHeight);
            int ret = model_0->inference(&npu_image, nullptr, &Results);
            model_0->set_det_restore_resolution(width, height);
            if (ret)
            {
                AX_SYS_MemFree(npu_image.pPhy, npu_image.pVir);
                continue;
            }
            if (Results.nObjSize)
            {
                model_1->set_current_index(0);
                ret = model_1->inference(&npu_image, nullptr, &Results);
                if (ret)
                {
                    AX_SYS_MemFree(npu_image.pPhy, npu_image.pVir);
                    continue;
                }
                faceid.feat.resize(SAMPLE_FACE_FEAT_LEN);
                memcpy(faceid.feat.data(), Results.mObjects[0].mFaceFeat.data, SAMPLE_FACE_FEAT_LEN * sizeof(float));
                ALOGI("register name=%s", faceid.name.c_str());
            }
            AX_SYS_MemFree(npu_image.pPhy, npu_image.pVir);
        }
        b_face_database_init = true;
    }
    int ret = model_0->inference(pstFrame, crop_resize_box, results);
    if (ret)
        return ret;

    for (size_t i = 0; i < results->nObjSize; i++)
    {
        model_1->set_current_index(i);
        ret = model_1->inference(pstFrame, crop_resize_box, results);
        if (ret)
        {
            ALOGE("sub model inference failed");
            return ret;
        }

        int maxidx = -1;
        float max_score = 0;
        for (size_t j = 0; j < face_register_ids.size(); j++)
        {
            if (face_register_ids[j].feat.size() != SAMPLE_FACE_FEAT_LEN)
            {
                continue;
            }
            float sim = _calcSimilar((float *)results->mObjects[i].mFaceFeat.data, face_register_ids[j].feat.data(), SAMPLE_FACE_FEAT_LEN);
            if (sim > max_score && sim > FACE_RECOGNITION_THRESHOLD)
            {
                maxidx = j;
                max_score = sim;
            }
        }
        // ALOGI("%f", max_score);

        if (maxidx >= 0)
        {
            if (max_score >= FACE_RECOGNITION_THRESHOLD)
            {
                memset(results->mObjects[i].objname, 0, SAMPLE_OBJ_NAME_MAX_LEN);
                int len = MIN(SAMPLE_OBJ_NAME_MAX_LEN - 1, face_register_ids[maxidx].name.size());
                memcpy(&results->mObjects[i].objname[0], face_register_ids[maxidx].name.data(), len);
            }
            else
            {
                sprintf(results->mObjects[i].objname, "unknow");
            }
        }
        else
        {
            sprintf(results->mObjects[i].objname, "unknow");
        }
    }

    return 0;
}

int ax_model_vehicle_license_recognition::inference(const void *pstFrame, ax_joint_runner_box_t *crop_resize_box, libaxdl_results_t *results)
{
    int ret = model_0->inference(pstFrame, crop_resize_box, results);
    if (ret)
        return ret;

    for (size_t i = 0; i < results->nObjSize; i++)
    {
        model_1->set_current_index(i);
        ret = model_1->inference(pstFrame, crop_resize_box, results);
        if (ret)
            return ret;
    }
    return 0;
}
