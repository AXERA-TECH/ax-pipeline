#include "sample_run_joint_post_process.h"
#include "../utilities/json.hpp"
#include "fstream"
#include "../utilities/sample_log.h"
#include "opencv2/opencv.hpp"
#include "npu_cv_kit/ax_npu_imgproc.h"
#include "ax_sys_api.h"
// #include "utilities/timer.hpp"
#include "../utilities/ringbuffer.hpp"
#include "../utilities/MultikeyMap.h"
#include "utilities/mat_pixel_affine.h"
extern "C"
{
    // 给sipeed的python包用的
    typedef int (*result_callback_for_sipeed_py)(void *, sample_run_joint_results *);
    result_callback_for_sipeed_py g_cb_results_sipeed_py = NULL;
    int register_result_callback(result_callback_for_sipeed_py cb)
    {
        g_cb_results_sipeed_py = cb;
        return 0;
    }
}

float FACE_RECOGNITION_THRESHOLD = 0.4f;

struct sample_run_joint_faceid
{
    std::string name;
    std::string path;
    std::vector<float> feat;
};

std::vector<sample_run_joint_faceid> face_ids;

typedef int (*inference_func)(sample_run_joint_models *pModels, const void *pstFrame, sample_run_joint_results *pResults);

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
    static SimpleRingBuffer<cv::Mat> mSimpleRingBuffer(SAMPLE_RINGBUFFER_CACHE_COUNT);

    int seg_h = pModels->mMajor.JointAttr.pOutputsInfo->pShape[2];
    int seg_w = pModels->mMajor.JointAttr.pOutputsInfo->pShape[3];
    int seg_size = seg_h * seg_w;

    cv::Mat &seg_mat = mSimpleRingBuffer.next();
    if (seg_mat.empty())
    {
        seg_mat = cv::Mat(seg_h, seg_w, CV_8UC1);
    }
    pResults->mPPHumSeg.h = seg_h;
    pResults->mPPHumSeg.w = seg_w;
    pResults->mPPHumSeg.data = seg_mat.data;

    for (int j = 0; j < seg_h * seg_w; ++j)
    {
        pResults->mPPHumSeg.data[j] = (ptr[j] < ptr[j + seg_size]) ? 255 : 0;
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
    for (int i = 0; i < pResults->nObjSize; i++)
    {
        if (pResults->mObjects[i].label == pModels->MINOR_CLASS_IDS[0])
        {
            memcpy(&HumObj, &pResults->mObjects[i], sizeof(sample_run_joint_object));
            bHasHuman = AX_TRUE;
            idx = i;
            break;
        }
    }

    if (bHasHuman == AX_TRUE && pModels->mMinor.JointHandle && HumObj.bbox.w > 0 && HumObj.bbox.h > 0)
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

        cv::Point2f src_pts[4];

        if ((HumObj.bbox.w / HumObj.bbox.h) >
            (float(pModels->mMinor.JointAttr.algo_width) / float(pModels->mMinor.JointAttr.algo_height)))
        {
            float offset = ((HumObj.bbox.w * (float(pModels->mMinor.JointAttr.algo_height) / float(pModels->mMinor.JointAttr.algo_width))) - HumObj.bbox.h) / 2;

            src_pts[0] = cv::Point2f(HumObj.bbox.x, HumObj.bbox.y - offset);
            src_pts[1] = cv::Point2f(HumObj.bbox.x + HumObj.bbox.w, HumObj.bbox.y - offset);
            src_pts[2] = cv::Point2f(HumObj.bbox.x + HumObj.bbox.w, HumObj.bbox.y + HumObj.bbox.h + offset);
            src_pts[3] = cv::Point2f(HumObj.bbox.x, HumObj.bbox.y + HumObj.bbox.h + offset);
        }
        else
        {
            float offset = ((HumObj.bbox.h * (float(pModels->mMinor.JointAttr.algo_width) / float(pModels->mMinor.JointAttr.algo_height))) - HumObj.bbox.w) / 2;

            src_pts[0] = cv::Point2f(HumObj.bbox.x - offset, HumObj.bbox.y);
            src_pts[1] = cv::Point2f(HumObj.bbox.x + HumObj.bbox.w + offset, HumObj.bbox.y);
            src_pts[2] = cv::Point2f(HumObj.bbox.x + HumObj.bbox.w + offset, HumObj.bbox.y + HumObj.bbox.h);
            src_pts[3] = cv::Point2f(HumObj.bbox.x - offset, HumObj.bbox.y + HumObj.bbox.h);
        }

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


        ret = sample_run_joint_inference(pModels->mMinor.JointHandle, &tmp, NULL);
        sample_run_joint_post_process_pose(pModels, &HumObj);
        pResults->nObjSize = 1;
        memcpy(&pResults->mObjects[0], &HumObj, sizeof(sample_run_joint_object));

        for (int j = 0; j < SAMPLE_RUN_JOINT_BODY_LMK_SIZE; j++)
        {
            /*
            [x`]   [m00,m01,m02]   [x]   [m00*x + m01*y + m02]
            [y`] = [m10,m11,m12] * [y] = [m10*x + m11*y + m12]
            [1 ]   [0  ,0  ,1  ]   [1]   [          1        ]
           */
            int x = affine_trans_mat_inv.at<double>(0, 0) * pResults->mObjects[0].landmark[j].x + affine_trans_mat_inv.at<double>(0, 1) * pResults->mObjects[0].landmark[j].y + affine_trans_mat_inv.at<double>(0, 2);
            int y = affine_trans_mat_inv.at<double>(1, 0) * pResults->mObjects[0].landmark[j].x + affine_trans_mat_inv.at<double>(1, 1) * pResults->mObjects[0].landmark[j].y + affine_trans_mat_inv.at<double>(1, 2);
            pResults->mObjects[0].landmark[j].x = x;
            pResults->mObjects[0].landmark[j].y = y;
        }
    }
    else
        pResults->nObjSize = 0;

    for (int i = 0; i < pResults->nObjSize; i++)
    {
        pResults->mObjects[i].bbox.x /= pModels->SAMPLE_RESTORE_WIDTH;
        pResults->mObjects[i].bbox.y /= pModels->SAMPLE_RESTORE_HEIGHT;
        pResults->mObjects[i].bbox.w /= pModels->SAMPLE_RESTORE_WIDTH;
        pResults->mObjects[i].bbox.h /= pModels->SAMPLE_RESTORE_HEIGHT;

        if (pResults->mObjects[i].nLandmark)
        {
            for (int j = 0; j < SAMPLE_RUN_JOINT_MAX_LMK_SIZE; j++)
            {
                pResults->mObjects[idx].landmark[j].x /= pModels->SAMPLE_RESTORE_WIDTH;
                pResults->mObjects[idx].landmark[j].y /= pModels->SAMPLE_RESTORE_HEIGHT;
            }
        }
    }
    return ret;
}

int _sample_run_joint_inference_animal_pose(sample_run_joint_models *pModels, const void *pstFrame, sample_run_joint_results *pResults)
{
    int ret = sample_run_joint_inference(pModels->mMajor.JointHandle, pstFrame, NULL);
    sample_run_joint_post_process_det_single_func(pResults, pModels);

    sample_run_joint_object HumObj = {0};
    int idx = -1;
    AX_BOOL bHasHuman = AX_FALSE;
    for (int i = 0; i < pResults->nObjSize; i++)
    {
        for (int j = 0; j < pModels->NUM_MINOR_CLASS_ID && j < SAMPLE_CLASS_ID_COUNT; j++)
        {
            if (pResults->mObjects[i].label == pModels->MINOR_CLASS_IDS[j])
            {
                if (pResults->mObjects[i].bbox.w * pResults->mObjects[i].bbox.h > HumObj.bbox.w * HumObj.bbox.h)
                {
                    memcpy(&HumObj, &pResults->mObjects[i], sizeof(sample_run_joint_object));
                    bHasHuman = AX_TRUE;
                    idx = i;
                    break; // pResults->nObjSize = 1;
                }
            }
        }
    }

    if (bHasHuman == AX_TRUE && pModels->mMinor.JointHandle && HumObj.bbox.w > 0 && HumObj.bbox.h > 0)
    {
        ret = sample_run_joint_inference(pModels->mMinor.JointHandle, pstFrame, &HumObj.bbox);
        sample_run_joint_post_process_pose(pModels, &HumObj);
        pResults->nObjSize = 1;
        memcpy(&pResults->mObjects[0], &HumObj, sizeof(sample_run_joint_object));
        // memcpy(&pResults->mObjects[0].landmark[0], &HumObj.landmark[0], sizeof(HumObj.landmark));
    }
    else
        pResults->nObjSize = 0;

    for (int i = 0; i < pResults->nObjSize; i++)
    {
        pResults->mObjects[i].bbox.x /= pModels->SAMPLE_RESTORE_WIDTH;
        pResults->mObjects[i].bbox.y /= pModels->SAMPLE_RESTORE_HEIGHT;
        pResults->mObjects[i].bbox.w /= pModels->SAMPLE_RESTORE_WIDTH;
        pResults->mObjects[i].bbox.h /= pModels->SAMPLE_RESTORE_HEIGHT;

        if (pResults->mObjects[i].nLandmark)
        {
            for (int j = 0; j < SAMPLE_RUN_JOINT_MAX_LMK_SIZE; j++)
            {
                pResults->mObjects[idx].landmark[j].x /= pModels->SAMPLE_RESTORE_WIDTH;
                pResults->mObjects[idx].landmark[j].y /= pModels->SAMPLE_RESTORE_HEIGHT;
            }
        }
    }
    return ret;
}

void _normalize(float *feature, int feature_len)
{
    float sum = 0;
    for (int it = 0; it < feature_len; it++)
        sum += feature[it] * feature[it];
    sum = sqrt(sum);
    for (int it = 0; it < feature_len; it++)
        feature[it] /= sum;
}

double _calcSimilar(float *feature1, float *feature2, int feature_len)
{
    // assert(feature1.size() == feature2.size());
    double sim = 0.0;
    for (int i = 0; i < feature_len; i++)
        sim += feature1[i] * feature2[i];
    sim = sim < 0 ? 0 : sim > 1 ? 1
                                : sim;
    return sim;
}

void align_face(sample_run_joint_object &obj, AX_NPU_CV_Image *npu_image, AX_NPU_CV_Image &npu_image_face_align)
{
    // static float target[10] = {30.2946, 51.6963,
    //                            65.5318, 51.5014,
    //                            48.0252, 71.7366,
    //                            33.5493, 92.3655,
    //                            62.7299, 92.2041};
    static float target[10] = {38.2946, 51.6963,
                               73.5318, 51.5014,
                               56.0252, 71.7366,
                               41.5493, 92.3655,
                               70.7299, 92.2041};
    float _tmp[10] = {obj.landmark[0].x, obj.landmark[0].y,
                      obj.landmark[1].x, obj.landmark[1].y,
                      obj.landmark[2].x, obj.landmark[2].y,
                      obj.landmark[3].x, obj.landmark[3].y,
                      obj.landmark[4].x, obj.landmark[4].y};
    float _m[6], _m_inv[6];
    get_affine_transform(_tmp, target, 5, _m);
    invert_affine_transform(_m, _m_inv);

    float mat3x3[3][3] = {
        {_m_inv[0], _m_inv[1], _m_inv[2]},
        {_m_inv[3], _m_inv[4], _m_inv[5]},
        {0, 0, 1}};
    // //这里要用AX_NPU_MODEL_TYPE_1_1_2
    npu_image_face_align.eDtype = npu_image->eDtype;
    if (npu_image_face_align.eDtype == AX_NPU_CV_FDT_RGB || npu_image_face_align.eDtype == AX_NPU_CV_FDT_BGR)
    {
        npu_image_face_align.nSize = 112 * 112 * 3;
    }
    else if (npu_image_face_align.eDtype == AX_NPU_CV_FDT_NV12 || npu_image_face_align.eDtype == AX_NPU_CV_FDT_NV21)
    {
        npu_image_face_align.nSize = 112 * 112 * 1.5;
    }
    else
    {
        ALOGE("just only support BGR/RGB/NV12 format");
    }
    int ret = AX_NPU_CV_Warp(AX_NPU_MODEL_TYPE_1_1_2, npu_image, &npu_image_face_align, &mat3x3[0][0], AX_NPU_CV_BILINEAR, 128);
}

int _sample_run_joint_inference_face_recognition(sample_run_joint_models *pModels, const void *pstFrame, sample_run_joint_results *pResults)
{
    static bool b_face_database_init = false;

    static AX_NPU_CV_Image npu_image_face_align;

    if (!b_face_database_init)
    {
        npu_image_face_align.nWidth = npu_image_face_align.nHeight = npu_image_face_align.tStride.nW = 112;
        AX_SYS_MemAlloc((AX_U64 *)&npu_image_face_align.pPhy, (void **)&npu_image_face_align.pVir, 112 * 112 * 3, 0x100, (AX_S8 *)"SAMPLE-CV");

        for (size_t i = 0; i < face_ids.size(); i++)
        {
            auto &faceid = face_ids[i];
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

            int ret = sample_run_joint_inference(pModels->mMajor.JointHandle, &npu_image, NULL);
            sample_run_joint_results Results = {0};

            int tmp_width = pModels->SAMPLE_RESTORE_WIDTH;
            int tmp_height = pModels->SAMPLE_RESTORE_HEIGHT;
            pModels->SAMPLE_RESTORE_WIDTH = npu_image.nWidth;
            pModels->SAMPLE_RESTORE_HEIGHT = npu_image.nHeight;
            sample_run_joint_post_process_det_single_func(&Results, pModels);
            pModels->SAMPLE_RESTORE_WIDTH = tmp_width;
            pModels->SAMPLE_RESTORE_HEIGHT = tmp_height;

            if (Results.nObjSize)
            {
                sample_run_joint_object &obj = Results.mObjects[0];

                align_face(obj, &npu_image, npu_image_face_align);

                ret = sample_run_joint_inference(pModels->mMinor.JointHandle, &npu_image_face_align, nullptr);

                faceid.feat.resize(SAMPLE_FACE_FEAT_LEN);
                memcpy(faceid.feat.data(), pModels->mMinor.JointAttr.pOutputs[0].pVirAddr, SAMPLE_FACE_FEAT_LEN * sizeof(float));
                _normalize(faceid.feat.data(), SAMPLE_FACE_FEAT_LEN);
            }

            AX_SYS_MemFree(npu_image.pPhy, npu_image.pVir);
        }

        b_face_database_init = true;
    }

    int ret = sample_run_joint_inference(pModels->mMajor.JointHandle, pstFrame, NULL);
    sample_run_joint_post_process_det_single_func(pResults, pModels);
    float feat[SAMPLE_FACE_FEAT_LEN];
    for (int i = 0; i < pResults->nObjSize; i++)
    {
        sample_run_joint_object &obj = pResults->mObjects[i];
        align_face(obj, (AX_NPU_CV_Image *)pstFrame, npu_image_face_align);

        ret = sample_run_joint_inference(pModels->mMinor.JointHandle, &npu_image_face_align, nullptr);

        memcpy(&feat[0], pModels->mMinor.JointAttr.pOutputs[0].pVirAddr, SAMPLE_FACE_FEAT_LEN * 4);
        _normalize(feat, SAMPLE_FACE_FEAT_LEN);
        int maxidx = -1;
        float max_score = 0;
        for (size_t j = 0; j < face_ids.size(); j++)
        {
            if (face_ids[j].feat.size() != SAMPLE_FACE_FEAT_LEN)
            {
                continue;
            }
            float sim = _calcSimilar(feat, face_ids[j].feat.data(), SAMPLE_FACE_FEAT_LEN);
            if (sim > max_score && sim > FACE_RECOGNITION_THRESHOLD)
            {
                maxidx = j;
                max_score = sim;
            }
        }

        if (maxidx >= 0)
        {
            if (max_score >= FACE_RECOGNITION_THRESHOLD)
            {
                memset(obj.objname, 0, SAMPLE_OBJ_NAME_MAX_LEN);
                int len = MIN(SAMPLE_OBJ_NAME_MAX_LEN - 1, face_ids[maxidx].name.size());
                memcpy(&obj.objname[0], face_ids[maxidx].name.data(), len);
            }
            else
            {
                sprintf(obj.objname, "unknow");
            }
        }
        else
        {
            sprintf(obj.objname, "unknow");
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
    }
    return ret;
}

int _sample_run_joint_inference_license_plate_recognition(sample_run_joint_models *pModels, const void *pstFrame, sample_run_joint_results *pResults)
{
    int ret = sample_run_joint_inference(pModels->mMajor.JointHandle, pstFrame, NULL);
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

        static const std::vector<std::string> plate_string = {
            // "#", "京", "沪", "津", "渝", "冀", "晋", "蒙", "辽", "吉", "黑", "苏", "浙", "皖",
            // "闽", "赣", "鲁", "豫", "鄂", "湘", "粤", "桂", "琼", "川", "贵", "云", "藏", "陕",
            // "甘", "青", "宁", "新", "学", "警", "港", "澳", "挂", "使", "领", "民", "航", "深",
            "#", "beijing", "shanghai", "tianjin", "chongqing", "hebei", "shan1xi", "neimenggu", "liaoning", "jilin", "heilongjiang", "jiangsu", "zhejiang", "anhui",
            "fujian", "jiangxi", "shandong", "henan", "hubei", "hunan", "guangdong", "guangxi", "hainan", "sichuan", "guizhou", "yunnan", "xizang", "shan3xi",
            "gansu", "qinghai", "ningxia", "xinjiang", "jiaolian", "jingcha", "xianggang", "aomen", "gua", "shi", "ling", "ming", "hang", "shen",
            "0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
            "A", "B", "C", "D", "E", "F", "G", "H",
            "J", "K", "L", "M", "N", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z"};

        ret = sample_run_joint_inference(pModels->mMinor.JointHandle, &tmp, nullptr);

        // 1x1x21x78
        float *outputdata = (float *)pModels->mMinor.JointAttr.pOutputs[0].pVirAddr;
        float argmax_data[21];
        float argmax_idx[21];

        for (int row = 0; row < 21; row++)
        {
            argmax_data[row] = outputdata[0];
            argmax_idx[row] = 0;
            for (int col = 0; col < 78; col++)
            {
                if (outputdata[0] > argmax_data[row])
                {
                    argmax_data[row] = outputdata[0];
                    argmax_idx[row] = col;
                }
                outputdata += 1;
            }
        }

        std::string plate = "";
        std::string pre_str = "#";
        for (int i = 0; i < 21; i++)
        {
            int index = argmax_idx[i];
            if (plate_string[index] != "#" && plate_string[index] != pre_str)
                plate += plate_string[index];
            pre_str = plate_string[index];
        }
        // return plate;

        // ALOGI("%s %d",plate.c_str(), plate.length());
        if (plate.length() < SAMPLE_OBJ_NAME_MAX_LEN) {
            sprintf(pResults->mObjects[i].objname, "%.*s", plate.length(), plate.c_str());
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

        if (pResults->mObjects[i].nLandmark)
        {
            for (size_t j = 0; j < SAMPLE_RUN_JOINT_MAX_LMK_SIZE; j++)
            {
                pResults->mObjects[i].landmark[j].x /= pModels->SAMPLE_RESTORE_WIDTH;
                pResults->mObjects[i].landmark[j].y /= pModels->SAMPLE_RESTORE_HEIGHT;
            }
        }
    }
    return ret;
}

static codepi::MultikeyMap<std::string, int, inference_func> ModelTypeTable = {
    {"MT_UNKNOWN", MT_UNKNOWN, nullptr},
    {"MT_DET_YOLOV5", MT_DET_YOLOV5, _sample_run_joint_inference_det},
    {"MT_DET_YOLOV5_FACE", MT_DET_YOLOV5_FACE, _sample_run_joint_inference_det},
    {"MT_DET_YOLOV7", MT_DET_YOLOV7, _sample_run_joint_inference_det},
    {"MT_DET_YOLOV6", MT_DET_YOLOV6, _sample_run_joint_inference_det},
    {"MT_DET_YOLOX", MT_DET_YOLOX, _sample_run_joint_inference_det},
    {"MT_DET_NANODET", MT_DET_NANODET, _sample_run_joint_inference_det},
    {"MT_INSEG_YOLOV5_MASK", MT_INSEG_YOLOV5_MASK, _sample_run_joint_inference_det},
    {"MT_DET_YOLOX_PPL", MT_DET_YOLOX_PPL, _sample_run_joint_inference_det},
    {"MT_DET_PALM_HAND", MT_DET_PALM_HAND, _sample_run_joint_inference_det},
    {"MT_DET_YOLOPV2", MT_DET_YOLOPV2, _sample_run_joint_inference_det},
    {"MT_DET_YOLO_FASTBODY", MT_DET_YOLO_FASTBODY, _sample_run_joint_inference_det},
    {"MT_DET_YOLOV5_LICENSE_PLATE", MT_DET_YOLOV5_LICENSE_PLATE, _sample_run_joint_inference_det},
    {"MT_DET_YOLOV7_FACE", MT_DET_YOLOV7_FACE, _sample_run_joint_inference_det},
    {"MT_DET_YOLOV7_PALM_HAND", MT_DET_YOLOV7_PALM_HAND, _sample_run_joint_inference_det},
    {"MT_SEG_PPHUMSEG", MT_SEG_PPHUMSEG, _sample_run_joint_inference_pphumseg},
    {"MT_MLM_HUMAN_POSE_HRNET", MT_MLM_HUMAN_POSE_HRNET, _sample_run_joint_inference_human_pose},
    {"MT_MLM_ANIMAL_POSE_HRNET", MT_MLM_ANIMAL_POSE_HRNET, _sample_run_joint_inference_animal_pose},
    {"MT_MLM_HUMAN_POSE_AXPPL", MT_MLM_HUMAN_POSE_AXPPL, _sample_run_joint_inference_human_pose},
    {"MT_MLM_HAND_POSE", MT_MLM_HAND_POSE, _sample_run_joint_inference_handpose},
    {"MT_MLM_VEHICLE_LICENSE_RECOGNITION", MT_MLM_VEHICLE_LICENSE_RECOGNITION, _sample_run_joint_inference_license_plate_recognition},
    {"MT_MLM_FACE_RECOGNITION", MT_MLM_FACE_RECOGNITION, _sample_run_joint_inference_face_recognition}};

int sample_run_joint_parse_param(char *json_file_path, sample_run_joint_models *pModels)
{
    std::ifstream f(json_file_path);
    if (f.fail())
    {
        return -1;
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
            if (ModelTypeTable.contain(mt))
            {
                pModels->ModelType_Main = (SAMPLE_RUN_JOINT_MODEL_TYPE)mt;
            }
            else
            {
                pModels->ModelType_Main = MT_UNKNOWN;
            }
        }
        else if (jsondata["MODEL_TYPE"].is_string())
        {
            std::string strModelType = jsondata["MODEL_TYPE"];

            // auto item = ModelTypeTable.find(strModelType);

            if (ModelTypeTable.contain(strModelType))
            {
                auto match_vec = ModelTypeTable.get1(strModelType);
                if (match_vec.size() > 1)
                {
                    ALOGE("[%s] multi define in ModelTypeTable,please check ModelTypeTable", strModelType.c_str());
                    return -1;
                }
                pModels->ModelType_Main = (SAMPLE_RUN_JOINT_MODEL_TYPE)match_vec[0]->key2;
            }
            else
            {
                pModels->ModelType_Main = MT_UNKNOWN;
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
    case MT_DET_YOLOPV2:
    case MT_DET_YOLOV5_FACE:
    case MT_DET_YOLOV6:
    case MT_DET_YOLOV7:
    case MT_DET_YOLOX:
    case MT_DET_NANODET:
    case MT_DET_YOLOX_PPL:
    case MT_INSEG_YOLOV5_MASK:
    case MT_DET_PALM_HAND:
    case MT_DET_YOLOV7_FACE:
    case MT_DET_YOLOV7_PALM_HAND:
    case MT_DET_YOLOV5_LICENSE_PLATE:
        sample_parse_param_det(json_file_path);
        pModels->mMajor.ModelType = pModels->ModelType_Main;
        break;
    case MT_SEG_PPHUMSEG:
        pModels->mMajor.ModelType = pModels->ModelType_Main;
        break;
    case MT_MLM_HUMAN_POSE_AXPPL:
    case MT_MLM_HUMAN_POSE_HRNET:
    case MT_MLM_ANIMAL_POSE_HRNET:
    case MT_MLM_HAND_POSE:
    case MT_MLM_FACE_RECOGNITION:
    case MT_MLM_VEHICLE_LICENSE_RECOGNITION:
        if (jsondata.contains("MODEL_MAJOR"))
        {
            nlohmann::json json_major = jsondata["MODEL_MAJOR"];
            if (json_major.contains("MODEL_TYPE"))
            {
                if (json_major["MODEL_TYPE"].is_number_integer())
                {
                    int mt = -1;
                    mt = json_major["MODEL_TYPE"];
                    // pModels->mMajor.ModelType = (SAMPLE_RUN_JOINT_MODEL_TYPE)mt;

                    if (ModelTypeTable.contain(mt))
                    {
                        pModels->mMajor.ModelType = (SAMPLE_RUN_JOINT_MODEL_TYPE)mt;
                    }
                    else
                    {
                        pModels->mMajor.ModelType = MT_UNKNOWN;
                    }
                }
                else if (json_major["MODEL_TYPE"].is_string())
                {
                    std::string strModelType = json_major["MODEL_TYPE"];

                    if (ModelTypeTable.contain(strModelType))
                    {
                        auto match_vec = ModelTypeTable.get1(strModelType);
                        if (match_vec.size() > 1)
                        {
                            ALOGE("[%s] multi define in ModelTypeTable,please check ModelTypeTable", strModelType.c_str());
                            return -1;
                        }
                        pModels->mMajor.ModelType = (SAMPLE_RUN_JOINT_MODEL_TYPE)match_vec[0]->key2;
                    }
                    else
                    {
                        pModels->mMajor.ModelType = MT_UNKNOWN;
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
            if (json_minor.contains("CLASS_ID"))
            {
                std::vector<int> clsids = json_minor["CLASS_ID"];
                pModels->NUM_MINOR_CLASS_ID = MIN(clsids.size(), SAMPLE_CLASS_ID_COUNT);
                for (int i = 0; i < pModels->NUM_MINOR_CLASS_ID; i++)
                {
                    pModels->MINOR_CLASS_IDS[i] = clsids[i];
                }
            }
            if (json_minor.contains("FACE_DATABASE"))
            {
                nlohmann::json database = json_minor["FACE_DATABASE"];
                for (nlohmann::json::iterator it = database.begin(); it != database.end(); ++it)
                {
                    ALOGI("name:%s path:%s", it.key().c_str(), it.value().get<std::string>().c_str());
                    sample_run_joint_faceid faceid;
                    faceid.path = it.value();
                    faceid.name = it.key();
                    face_ids.push_back(faceid);
                }
            }
            if (json_minor.contains("FACE_RECOGNITION_THRESHOLD"))
            {
                FACE_RECOGNITION_THRESHOLD = json_minor["FACE_RECOGNITION_THRESHOLD"];
            }
        }

        break;
    default:
        ALOGE("unkown model type %d", pModels->ModelType_Main);
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
    int ret = 0;
    memset(pResults, 0, sizeof(sample_run_joint_results));
    pResults->mModelType = pModels->ModelType_Main;
#if AX_DEBUG
    static int cnt = 0;
    if (cnt++ % 30 == 0)
    {
        cv::Mat src(((AX_NPU_CV_Image *)pstFrame)->nHeight, ((AX_NPU_CV_Image *)pstFrame)->nWidth, CV_8UC3, ((AX_NPU_CV_Image *)pstFrame)->pVir);
        char path[128];
        sprintf(path, "debug_%05d.jpg", cnt);
        cv::imwrite(path, src);
        printf("save %s\n", path);
    }
#endif

    if (ModelTypeTable.contain(pModels->ModelType_Main))
    {
        int mt = pModels->ModelType_Main;
        auto func = ModelTypeTable.get2(mt);
        if (func[0]->val != nullptr)
        {
            ret = func[0]->val(pModels, pstFrame, pResults);
        }
        else
        {
            ALOGE("[%s] func pointer is null", func[0]->key1.c_str());
        }
    }
    else
    {
        ALOGE("cannot find inference func for modeltype %d", pModels->ModelType_Main);
        ret = -1;
    }

    if (g_cb_results_sipeed_py)
    {
        ret = g_cb_results_sipeed_py((void *)pstFrame, pResults);
    }

    {
        static int fcnt = 0;
        static int fps = -1;
        fcnt++;
        static struct timespec ts1, ts2;
        clock_gettime(CLOCK_MONOTONIC, &ts2);
        if ((ts2.tv_sec * 1000 + ts2.tv_nsec / 1000000) - (ts1.tv_sec * 1000 + ts1.tv_nsec / 1000000) >= 1000)
        {
            // printf("%s => H26X FPS:%d     \r\n", tips, fcnt);
            fps = fcnt;
            ts1 = ts2;
            fcnt = 0;
        }
        pResults->niFps = fps;
    }

    return ret;
}