#include "sample_run_joint_post_process.h"
#include "../utilities/json.hpp"
#include "fstream"
#include "../utilities/sample_log.h"
#include "opencv2/opencv.hpp"
#include "npu_cv_kit/ax_npu_imgproc.h"
#include "ax_sys_api.h"
#include "utilities/timer.hpp"

extern "C"
{
    //给sipeed的python包用的
    typedef int (*result_callback_for_sipeed_py)(void *, sample_run_joint_results *);
    result_callback_for_sipeed_py g_cb_results_sipeed_py = NULL;
    int register_result_callback(result_callback_for_sipeed_py cb) { g_cb_results_sipeed_py = cb; }
}

static std::map<std::string, int> ModelTypeTable = {
    {"MT_UNKNOWN", MT_UNKNOWN},
    {"MT_DET_YOLOV5", MT_DET_YOLOV5},
    {"MT_DET_YOLOV5_FACE", MT_DET_YOLOV5_FACE},
    {"MT_DET_YOLOV7", MT_DET_YOLOV7},
    {"MT_DET_YOLOX", MT_DET_YOLOX},
    {"MT_DET_NANODET", MT_DET_NANODET},
    {"MT_SEG_PPHUMSEG", MT_SEG_PPHUMSEG},
    {"MT_INSEG_YOLOV5_MASK", MT_INSEG_YOLOV5_MASK},
    {"MT_MLM_HUMAN_POSE_AXPPL", MT_MLM_HUMAN_POSE_AXPPL},
    {"MT_MLM_HUMAN_POSE_HRNET", MT_MLM_HUMAN_POSE_HRNET},
    {"MT_MLM_HAND_POSE", MT_MLM_HAND_POSE},
    {"MT_DET_YOLOX_PPL", MT_DET_YOLOX_PPL},
    {"MT_DET_PALM_HAND", MT_DET_PALM_HAND},

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

        pModels->mMajor.ModelType = pModels->ModelType_Main;
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
    case MT_DET_YOLOX_PPL:
    case MT_INSEG_YOLOV5_MASK:
    case MT_DET_PALM_HAND:
        sample_parse_param_det(json_file_path);
        pModels->mMajor.ModelType = pModels->ModelType_Main;
        break;
    case MT_SEG_PPHUMSEG:
        pModels->mMajor.ModelType = pModels->ModelType_Main;
        break;
    case MT_MLM_HUMAN_POSE_AXPPL:
    case MT_MLM_HUMAN_POSE_HRNET:
    case MT_MLM_HAND_POSE:
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

int _sample_run_joint_inference_det(sample_run_joint_models *pModels, const void *pstFrame, sample_run_joint_results *pResults)
{
    int ret = sample_run_joint_inference(pModels->mMajor.JointHandle, pstFrame, NULL);
    sample_run_joint_post_process_det_single_func(pResults, pModels);
    return ret;
}

int _sample_run_joint_inference_pphumseg(sample_run_joint_models *pModels, const void *pstFrame, sample_run_joint_results *pResults)
{
    int ret = sample_run_joint_inference(pModels->mMajor.JointHandle, pstFrame, NULL);
    pResults->bPPHumSeg = 1;
    auto ptr = (float *)pModels->mMajor.JointAttr.pOutputs[0].pVirAddr;
    for (int j = 0; j < SAMPLE_RUN_JOINT_PP_HUM_SEG_SIZE; ++j)
    {
        pResults->mPPHumSeg.mask[j] = (ptr[j] < ptr[j + SAMPLE_RUN_JOINT_PP_HUM_SEG_SIZE]) ? 255 : 0;
    }
    return ret;
}

int _sample_run_joint_inference_human_pose(sample_run_joint_models *pModels, const void *pstFrame, sample_run_joint_results *pResults)
{
    int ret = sample_run_joint_inference(pModels->mMajor.JointHandle, pstFrame, NULL);
    sample_run_joint_post_process_det_single_func(pResults, pModels);

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
        sample_run_joint_post_process_pose(pModels, &HumObj);
        pResults->nObjSize = 1;
        pResults->mObjects[0].bHasBodyLmk = 1;
        memcpy(&pResults->mObjects[0].landmark[0], &HumObj.landmark[0], sizeof(HumObj.landmark));
    }

    for (int i = 0; i < pResults->nObjSize; i++)
    {
        pResults->mObjects[i].bbox.x /= pModels->SAMPLE_RESTORE_WIDTH;
        pResults->mObjects[i].bbox.y /= pModels->SAMPLE_RESTORE_HEIGHT;
        pResults->mObjects[i].bbox.w /= pModels->SAMPLE_RESTORE_WIDTH;
        pResults->mObjects[i].bbox.h /= pModels->SAMPLE_RESTORE_HEIGHT;

        if (pResults->mObjects[i].bHasBodyLmk)
        {
            for (int j = 0; j < SAMPLE_RUN_JOINT_BODY_LMK_SIZE; j++)
            {
                pResults->mObjects[idx].landmark[j].x /= pModels->SAMPLE_RESTORE_WIDTH;
                pResults->mObjects[idx].landmark[j].y /= pModels->SAMPLE_RESTORE_HEIGHT;
            }
        }
    }
    return ret;
}

int _sample_run_joint_inference_handpose(sample_run_joint_models *pModels, const void *pstFrame, sample_run_joint_results *pResults)
{
    static AX_NPU_CV_Image _pstFrame = {0};
    if (!_pstFrame.pVir)
    {
        memcpy(&_pstFrame, pstFrame, sizeof(AX_NPU_CV_Image));
        _pstFrame.eDtype = AX_NPU_CV_FDT_BGR;
        AX_SYS_MemAlloc(&_pstFrame.pPhy, (void **)&_pstFrame.pVir, _pstFrame.nSize, 0x100, NULL);
    }

    _pstFrame.eDtype = AX_NPU_CV_FDT_BGR;
    AX_NPU_CV_CSC(AX_NPU_MODEL_TYPE_1_1_1, (AX_NPU_CV_Image *)pstFrame, &_pstFrame);
    _pstFrame.eDtype = AX_NPU_CV_FDT_RGB;

    int ret = sample_run_joint_inference(pModels->mMajor.JointHandle, &_pstFrame, NULL);
    sample_run_joint_post_process_det_single_func(pResults, pModels);
    for (int i = 0; i < pResults->nObjSize; i++)
    {
        static AX_NPU_CV_Image tmp = {0};
        if (!tmp.pVir)
        {
            tmp.eDtype = ((AX_NPU_CV_Image *)pstFrame)->eDtype;
            tmp.nHeight = pModels->mMinor.JointAttr.algo_height;
            tmp.nWidth = pModels->mMinor.JointAttr.algo_width;
            tmp.tStride.nW = tmp.nWidth;
            if (tmp.eDtype == AX_NPU_CV_FDT_NV12)
            {
                tmp.nSize = tmp.nHeight * tmp.nWidth * 3 / 2;
            }
            else if (tmp.eDtype == AX_NPU_CV_FDT_RGB || tmp.eDtype == AX_NPU_CV_FDT_BGR)
            {
                tmp.nSize = tmp.nHeight * tmp.nWidth * 3;
            }
            else
            {
                ALOGE("just only support nv12/rgb/bgr format\n");
                return -1;
            }
            AX_SYS_MemAlloc(&tmp.pPhy, (void **)&tmp.pVir, tmp.nSize, 0x100, NULL);
        }

        sample_run_joint_object &object = pResults->mObjects[i];

        cv::Point2f src_pts[4];
        src_pts[0] = cv::Point2f(object.bbox_vertices[0].x, object.bbox_vertices[0].y);
        src_pts[1] = cv::Point2f(object.bbox_vertices[1].x, object.bbox_vertices[1].y);
        src_pts[2] = cv::Point2f(object.bbox_vertices[2].x, object.bbox_vertices[2].y);
        src_pts[3] = cv::Point2f(object.bbox_vertices[3].x, object.bbox_vertices[3].y);

        cv::Point2f dst_pts[4];
        dst_pts[0] = cv::Point2f(0, 0);
        dst_pts[1] = cv::Point2f(pModels->mMinor.JointAttr.algo_width, 0);
        dst_pts[2] = cv::Point2f(pModels->mMinor.JointAttr.algo_width, pModels->mMinor.JointAttr.algo_height);
        dst_pts[3] = cv::Point2f(0, pModels->mMinor.JointAttr.algo_height);

        cv::Mat affine_trans_mat = cv::getAffineTransform(src_pts, dst_pts);
        cv::Mat affine_trans_mat_inv;
        cv::invertAffineTransform(affine_trans_mat, affine_trans_mat_inv);

        float mat3x3[3][3] = {
            {(float)affine_trans_mat_inv.at<double>(0, 0), (float)affine_trans_mat_inv.at<double>(0, 1), (float)affine_trans_mat_inv.at<double>(0, 2)},
            {(float)affine_trans_mat_inv.at<double>(1, 0), (float)affine_trans_mat_inv.at<double>(1, 1), (float)affine_trans_mat_inv.at<double>(1, 2)},
            {0, 0, 1}};
        // //这里要用AX_NPU_MODEL_TYPE_1_1_2
        ret = AX_NPU_CV_Warp(AX_NPU_MODEL_TYPE_1_1_2, (AX_NPU_CV_Image *)pstFrame, &tmp, &mat3x3[0][0], AX_NPU_CV_BILINEAR, 128);

        ret = sample_run_joint_inference(pModels->mMinor.JointHandle, &tmp, nullptr);
        sample_run_joint_post_process_pose(pModels, &object);

        for (size_t j = 0; j < SAMPLE_RUN_JOINT_HAND_LMK_SIZE; j++)
        {
            /*
            [x`]   [m00,m01,m02]   [x]   [m00*x + m01*y + m02]
            [y`] = [m10,m11,m12] * [y] = [m10*x + m11*y + m12]
            [1 ]   [0  ,0  ,1  ]   [1]   [          1        ]
            */
            int x = affine_trans_mat_inv.at<double>(0, 0) * object.landmark[j].x + affine_trans_mat_inv.at<double>(0, 1) * object.landmark[j].y + affine_trans_mat_inv.at<double>(0, 2);
            int y = affine_trans_mat_inv.at<double>(1, 0) * object.landmark[j].x + affine_trans_mat_inv.at<double>(1, 1) * object.landmark[j].y + affine_trans_mat_inv.at<double>(1, 2);
            object.landmark[j].x = x;
            object.landmark[j].y = y;
        }
    }

    for (int i = 0; i < pResults->nObjSize; i++)
    {
        pResults->mObjects[i].bbox.x /= pModels->SAMPLE_RESTORE_WIDTH;
        pResults->mObjects[i].bbox.y /= pModels->SAMPLE_RESTORE_HEIGHT;
        pResults->mObjects[i].bbox.w /= pModels->SAMPLE_RESTORE_WIDTH;
        pResults->mObjects[i].bbox.h /= pModels->SAMPLE_RESTORE_HEIGHT;

        if (pResults->mObjects[i].bHasBoxVertices)
        {
            for (size_t j = 0; j < 4; j++)
            {
                pResults->mObjects[i].bbox_vertices[j].x /= pModels->SAMPLE_RESTORE_WIDTH;
                pResults->mObjects[i].bbox_vertices[j].y /= pModels->SAMPLE_RESTORE_HEIGHT;
            }
        }

        if (pResults->mObjects[i].bHasHandLmk)
        {
            for (size_t j = 0; j < SAMPLE_RUN_JOINT_HAND_LMK_SIZE; j++)
            {
                pResults->mObjects[i].landmark[j].x /= pModels->SAMPLE_RESTORE_WIDTH;
                pResults->mObjects[i].landmark[j].y /= pModels->SAMPLE_RESTORE_HEIGHT;
            }
        }
    }
    return ret;
}

int sample_run_joint_inference_single_func(sample_run_joint_models *pModels, const void *pstFrame, sample_run_joint_results *pResults)
{
    int ret;
    memset(pResults, 0, sizeof(sample_run_joint_results));
    typedef int (*inference_func)(sample_run_joint_models * pModels, const void *pstFrame, sample_run_joint_results *pResults);
    static std::map<int, inference_func> m_func_map{
        {MT_DET_YOLOV5, _sample_run_joint_inference_det},
        {MT_DET_YOLOV5_FACE, _sample_run_joint_inference_det},
        {MT_DET_YOLOV7, _sample_run_joint_inference_det},
        {MT_DET_YOLOX, _sample_run_joint_inference_det},
        {MT_DET_NANODET, _sample_run_joint_inference_det},
        {MT_INSEG_YOLOV5_MASK, _sample_run_joint_inference_det},
        {MT_DET_YOLOX_PPL, _sample_run_joint_inference_det},
        {MT_DET_PALM_HAND, _sample_run_joint_inference_det},

        {MT_SEG_PPHUMSEG, _sample_run_joint_inference_pphumseg},

        {MT_MLM_HUMAN_POSE_HRNET, _sample_run_joint_inference_human_pose},
        {MT_MLM_HUMAN_POSE_AXPPL, _sample_run_joint_inference_human_pose},

        {MT_MLM_HAND_POSE, _sample_run_joint_inference_handpose},
    };

    auto item = m_func_map.find(pModels->ModelType_Main);

    if (item != m_func_map.end())
    {
        ret = item->second(pModels, pstFrame, pResults);
    }
    else
    {
        ALOGE("cannot find inference func for modeltype %d", pModels->ModelType_Main);
        ret = -1;
    }

    if (g_cb_results_sipeed_py)
    {
        int retcbnikeyi = g_cb_results_sipeed_py((void *)pstFrame, pResults);
    }

    return ret;
}