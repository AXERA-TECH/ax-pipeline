#include "sample_run_joint_post_process.h"
#include "../utilities/json.hpp"
#include "fstream"

static std::map<std::string, int> ModelTypeTable = {
    {"MT_UNKNOWN", MT_UNKNOWN},
    {"MT_DET_YOLOV5", MT_DET_YOLOV5},
    {"MT_DET_YOLOV5_FACE", MT_DET_YOLOV5_FACE},
    {"MT_DET_YOLOV7", MT_DET_YOLOV7},
    {"MT_DET_YOLOX", MT_DET_YOLOX},
    {"MT_DET_NANODET", MT_DET_NANODET},
    {"MT_SEG_PPHUMSEG", MT_SEG_PPHUMSEG},
    {"MT_INSEG_YOLOV5_MASK", MT_INSEG_YOLOV5_MASK},
    {"MT_MLM_HUMAN_POSE", MT_MLM_HUMAN_POSE},

};

int sample_run_joint_parse_param(char *json_file_path, sample_run_joint_models *pModels)
{
    std::ifstream f(json_file_path);
    if (f.fail())
    {
        return MT_UNKNOWN;
    }
    auto jsondata = nlohmann::json::parse(f);

    f.close();
    // update_val(jsondata, "MODEL_TYPE", &mt);
    if (jsondata.contains("MODEL_TYPE"))
    {
        if (jsondata["MODEL_TYPE"].is_number_integer())
        {
            int mt = -1;
            mt = jsondata["MODEL_TYPE"];
            pModels->ModelType_Main = (SAMPLE_RUN_JOINT_MODEL_TYPE)mt;
        }
        else if (jsondata["MODEL_TYPE"].is_string())
        {
            std::string strModelType = jsondata["MODEL_TYPE"];

            auto item = ModelTypeTable.find(strModelType);

            if (item != ModelTypeTable.end())
            {
                int mt = item->second;
                pModels->ModelType_Main = (SAMPLE_RUN_JOINT_MODEL_TYPE)mt;
            }
        }
    }

    if (jsondata.contains("MODEL_PATH"))
    {
        std::string path = jsondata["MODEL_PATH"];
        if (!path.empty())
        {
            strcpy(pModels->MODEL_PATH, path.data());
            pModels->bRunJoint = AX_TRUE;
        }
    }

    if (jsondata.contains("SAMPLE_IVPS_ALGO_WIDTH"))
    {
        pModels->SAMPLE_IVPS_ALGO_WIDTH = jsondata["SAMPLE_IVPS_ALGO_WIDTH"];
    }

    if (jsondata.contains("SAMPLE_IVPS_ALGO_HEIGHT"))
    {
        pModels->SAMPLE_IVPS_ALGO_HEIGHT = jsondata["SAMPLE_IVPS_ALGO_HEIGHT"];
    }

    switch (pModels->ModelType_Main)
    {
    case MT_DET_YOLOV5:
    case MT_DET_YOLOV5_FACE:
    case MT_DET_YOLOV7:
    case MT_DET_YOLOX:
    case MT_DET_NANODET:
    case MT_INSEG_YOLOV5_MASK:
        sample_parse_param_det(json_file_path);
        pModels->mMajor.ModelType = pModels->ModelType_Main;
        break;
    case MT_SEG_PPHUMSEG:
        pModels->mMajor.ModelType = pModels->ModelType_Main;
        break;
    case MT_MLM_HUMAN_POSE:
        if (jsondata.contains("MODEL_MAJOR"))
        {
            nlohmann::json json_major = jsondata["MODEL_MAJOR"];
            if (json_major.contains("MODEL_TYPE"))
            {
                if (json_major["MODEL_TYPE"].is_number_integer())
                {
                    int mt = -1;
                    mt = json_major["MODEL_TYPE"];
                    pModels->mMajor.ModelType = (SAMPLE_RUN_JOINT_MODEL_TYPE)mt;
                }
                else if (json_major["MODEL_TYPE"].is_string())
                {
                    std::string strModelType = json_major["MODEL_TYPE"];

                    auto item = ModelTypeTable.find(strModelType);

                    if (item != ModelTypeTable.end())
                    {
                        int mt = item->second;
                        pModels->mMajor.ModelType = (SAMPLE_RUN_JOINT_MODEL_TYPE)mt;
                    }
                }
            }

            if (json_major.contains("MODEL_PATH"))
            {
                std::string path = json_major["MODEL_PATH"];
                if (!path.empty())
                {
                    strcpy(pModels->MODEL_PATH, path.data());
                    pModels->bRunJoint = AX_TRUE;
                }
            }

            sample_set_param_det(&json_major);
        }
        if (jsondata.contains("MODEL_MINOR"))
        {
            nlohmann::json json_minor = jsondata["MODEL_MINOR"];
            if (json_minor.contains("MODEL_PATH"))
            {
                std::string hrnet_path = json_minor["MODEL_PATH"];
                strcpy(pModels->MODEL_PATH_L2, hrnet_path.data());
            }
        }

        break;
    default:
        break;
    }

    if (pModels->ModelType_Main == MT_UNKNOWN)
    {
        pModels->bRunJoint = AX_FALSE;
    }

    return 0;
}

int sample_run_joint_inference_single_func(sample_run_joint_models *pModels, const void *pstFrame, sample_run_joint_results *pResults)
{
    int ret;
    memset(pResults, 0, sizeof(sample_run_joint_results));
    ret = sample_run_joint_inference(pModels->mMajor.JointHandle, pstFrame, NULL);
    switch (pModels->ModelType_Main)
    {
    case MT_DET_YOLOV5:
    case MT_DET_YOLOV5_FACE:
    case MT_DET_YOLOV7:
    case MT_DET_YOLOX:
    case MT_DET_NANODET:
    {
        sample_run_joint_post_process_detection(pModels->ModelType_Main, &pModels->mMajor.JointAttr, pResults,
                                                pModels->SAMPLE_ALGO_WIDTH, pModels->SAMPLE_ALGO_HEIGHT,
                                                pModels->SAMPLE_RESTORE_WIDTH, pModels->SAMPLE_RESTORE_HEIGHT);

        for (AX_U8 i = 0; i < pResults->nObjSize; i++)
        {
            pResults->mObjects[i].bbox.x /= pModels->SAMPLE_RESTORE_WIDTH;
            pResults->mObjects[i].bbox.y /= pModels->SAMPLE_RESTORE_HEIGHT;
            pResults->mObjects[i].bbox.w /= pModels->SAMPLE_RESTORE_WIDTH;
            pResults->mObjects[i].bbox.h /= pModels->SAMPLE_RESTORE_HEIGHT;

            if (pModels->ModelType_Main == MT_DET_YOLOV5_FACE)
            {
                for (AX_U8 j = 0; j < SAMPLE_RUN_JOINT_FACE_LMK_SIZE; j++)
                {
                    pResults->mObjects[i].face_landmark[j].x /= pModels->SAMPLE_RESTORE_WIDTH;
                    pResults->mObjects[i].face_landmark[j].y /= pModels->SAMPLE_RESTORE_HEIGHT;
                }
            }
        }
    }
    break;
    case MT_INSEG_YOLOV5_MASK:
        sample_run_joint_post_process_yolov5_seg(pModels->ModelType_Main, &pModels->mMajor.JointAttr, pResults,
                                                 pModels->SAMPLE_ALGO_WIDTH, pModels->SAMPLE_ALGO_HEIGHT,
                                                 pModels->SAMPLE_RESTORE_WIDTH, pModels->SAMPLE_RESTORE_HEIGHT);
        for (AX_U8 i = 0; i < pResults->nObjSize; i++)
        {
            pResults->mObjects[i].bbox.x /= pModels->SAMPLE_RESTORE_WIDTH;
            pResults->mObjects[i].bbox.y /= pModels->SAMPLE_RESTORE_HEIGHT;
            pResults->mObjects[i].bbox.w /= pModels->SAMPLE_RESTORE_WIDTH;
            pResults->mObjects[i].bbox.h /= pModels->SAMPLE_RESTORE_HEIGHT;
        }
        break;
    case MT_SEG_PPHUMSEG:
    {
        // ret = sample_run_joint_inference(pModels->JointHandle_MAJOR, pstFrame, NULL);
        pResults->bPPHumSeg = 1;
        auto ptr = (float *)pModels->mMajor.JointAttr.pOutputs[0].pVirAddr;
        for (int j = 0; j < SAMPLE_RUN_JOINT_PP_HUM_SEG_SIZE; ++j)
        {
            pResults->mPPHumSeg.mask[j] = (ptr[j] < ptr[j + SAMPLE_RUN_JOINT_PP_HUM_SEG_SIZE]) ? 255 : 0;
        }
    }
    case MT_MLM_HUMAN_POSE:
    {
        // ret = sample_run_joint_inference(pModels->JointHandle_MAJOR, pstFrame, NULL);
        sample_run_joint_post_process_detection(pModels->mMajor.ModelType, &pModels->mMajor.JointAttr, pResults,
                                                pModels->SAMPLE_ALGO_WIDTH, pModels->SAMPLE_ALGO_HEIGHT,
                                                pModels->SAMPLE_RESTORE_WIDTH, pModels->SAMPLE_RESTORE_HEIGHT);

        sample_run_joint_object HumObj = {0};
        int idx = -1;
        AX_BOOL bHasHuman = AX_FALSE;
        for (size_t i = 0; i < pResults->nObjSize; i++)
        {
            if (pResults->mObjects[i].label == 0)
            {
                if (pResults->mObjects[i].bbox.w * pResults->mObjects[i].bbox.h > HumObj.bbox.w * HumObj.bbox.h)
                {
                    HumObj.bbox.x = std::max(pResults->mObjects[i].bbox.x, 1.f);
                    HumObj.bbox.y = std::max(pResults->mObjects[i].bbox.y, 1.f);
                    HumObj.bbox.w = std::min(pResults->mObjects[i].bbox.w, pModels->SAMPLE_RESTORE_WIDTH - HumObj.bbox.x - 1);
                    HumObj.bbox.h = std::min(pResults->mObjects[i].bbox.h, pModels->SAMPLE_RESTORE_HEIGHT - HumObj.bbox.y - 1);
                    bHasHuman = AX_TRUE;
                    idx = i;
                }
            }
        }

        if (bHasHuman == AX_TRUE && pModels->mMinor.JointHandle && HumObj.bbox.w > 0 && HumObj.bbox.h > 0)
        {
            ret = sample_run_joint_inference(pModels->mMinor.JointHandle, pstFrame, &HumObj.bbox);
            sample_run_joint_post_process_hrnet_pose(&pModels->mMinor.JointAttr, &HumObj);
            pResults->mObjects[idx].bHasPoseLmk = 1;
            memcpy(&pResults->mObjects[idx].pose_landmark[0], &HumObj.pose_landmark[0], sizeof(HumObj.pose_landmark));
        }

        for (int i = 0; i < pResults->nObjSize; i++)
        {
            pResults->mObjects[i].bbox.x /= pModels->SAMPLE_RESTORE_WIDTH;
            pResults->mObjects[i].bbox.y /= pModels->SAMPLE_RESTORE_HEIGHT;
            pResults->mObjects[i].bbox.w /= pModels->SAMPLE_RESTORE_WIDTH;
            pResults->mObjects[i].bbox.h /= pModels->SAMPLE_RESTORE_HEIGHT;

            if (pResults->mObjects[i].bHasPoseLmk)
            {
                for (int j = 0; j < SAMPLE_RUN_JOINT_POSE_LMK_SIZE; j++)
                {
                    pResults->mObjects[idx].pose_landmark[j].x /= pModels->SAMPLE_RESTORE_WIDTH;
                    pResults->mObjects[idx].pose_landmark[j].y /= pModels->SAMPLE_RESTORE_HEIGHT;
                }
            }
        }
    }
    break;
    default:
        break;
    }
    return 0;
}