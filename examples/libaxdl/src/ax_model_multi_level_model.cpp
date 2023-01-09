#include "ax_model_multi_level_model.hpp"

#include "../../utilities/json.hpp"
#include "../../utilities/sample_log.h"

#include "opencv2/opencv.hpp"

#include "fstream"
#include "ax_sys_api.h"

int ax_model_human_pose_axppl::inference(const void *pstFrame, ax_joint_runner_box *crop_resize_box, sample_run_joint_results *results)
{
    int ret = model_0->inference(pstFrame, crop_resize_box, results);
    if (ret)
        return ret;
    int idx = -1;
    for (int i = 0; i < results->nObjSize; i++)
    {
        auto it = std::find(CLASS_IDS.begin(), CLASS_IDS.end(), results->mObjects[i].label);
        if (it != CLASS_IDS.end())
        {
            idx = i;
            break;
        }
    }

    if (idx >= 0)
    {
        model_1->set_current_index(idx);
        ret = model_1->inference(pstFrame, crop_resize_box, results);
        if (ret)
            return ret;
        if (idx != 0)
        {
            memcpy(&results->mObjects[0], &results->mObjects[idx], sizeof(sample_run_joint_object));
        }
        results->nObjSize = 1;
    }
    else
        results->nObjSize = 0;
    return 0;
}

int ax_model_animal_pose_hrnet::inference(const void *pstFrame, ax_joint_runner_box *crop_resize_box, sample_run_joint_results *results)
{
    int ret = model_0->inference(pstFrame, crop_resize_box, results);
    if (ret)
        return ret;
    int idx = -1;
    for (int i = 0; i < results->nObjSize; i++)
    {
        auto it = std::find(CLASS_IDS.begin(), CLASS_IDS.end(), results->mObjects[i].label);
        if (it != CLASS_IDS.end())
        {
            idx = i;
            break;
        }
    }

    if (idx >= 0)
    {
        model_1->set_current_index(idx);
        ret = model_1->inference(pstFrame, crop_resize_box, results);
        if (ret)
            return ret;
        if (idx != 0)
        {
            memcpy(&results->mObjects[0], &results->mObjects[idx], sizeof(sample_run_joint_object));
        }
        results->nObjSize = 1;
    }
    else
        results->nObjSize = 0;
    return 0;
}

void ax_model_hand_pose::deinit()
{
    model_1->deinit();
    model_0->deinit();
    AX_SYS_MemFree(pstFrame_RGB.pPhy, pstFrame_RGB.pVir);
}

int ax_model_hand_pose::inference(const void *pstFrame, ax_joint_runner_box *crop_resize_box, sample_run_joint_results *results)
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

int ax_model_face_recognition::inference(const void *pstFrame, ax_joint_runner_box *crop_resize_box, sample_run_joint_results *results)
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

            sample_run_joint_results Results = {0};
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

int ax_model_vehicle_license_recognition::inference(const void *pstFrame, ax_joint_runner_box *crop_resize_box, sample_run_joint_results *results)
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
