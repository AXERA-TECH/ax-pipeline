#include "ax_model_ml_sub.hpp"
#include "base/pose.hpp"

#include "../../utilities/sample_log.h"
#include "utilities/mat_pixel_affine.h"

#include "ax_sys_api.h"

int ax_model_pose_hrnet_sub::preprocess(const void *pstFrame, ax_joint_runner_box_t *crop_resize_box, libaxdl_results_t *results)
{
    int ret;
    libaxdl_object_t &HumObj = results->mObjects[cur_idx];

    if (HumObj.bbox.w > 0 && HumObj.bbox.h > 0)
    {
        if (!dstFrame.pVir)
        {
            dstFrame.eDtype = ((AX_NPU_CV_Image *)pstFrame)->eDtype;
            dstFrame.nHeight = get_algo_height();
            dstFrame.nWidth = get_algo_width();
            dstFrame.tStride.nW = dstFrame.nWidth;
            if (dstFrame.eDtype == AX_NPU_CV_FDT_NV12)
            {
                dstFrame.nSize = dstFrame.nHeight * dstFrame.nWidth * 3 / 2;
            }
            else if (dstFrame.eDtype == AX_NPU_CV_FDT_RGB || dstFrame.eDtype == AX_NPU_CV_FDT_BGR)
            {
                dstFrame.eDtype = AX_NPU_CV_FDT_BGR;
                dstFrame.nSize = dstFrame.nHeight * dstFrame.nWidth * 3;
            }
            else
            {
                ALOGE("just only support nv12/rgb/bgr format\n");
                return -1;
            }
            AX_SYS_MemAlloc(&dstFrame.pPhy, (void **)&dstFrame.pVir, dstFrame.nSize, 0x100, NULL);
            bMalloc = true;
        }

        cv::Point2f src_pts[4];

        if ((HumObj.bbox.w / HumObj.bbox.h) >
            (float(get_algo_width()) / float(get_algo_height())))
        {
            float offset = ((HumObj.bbox.w * (float(get_algo_height()) / float(get_algo_width()))) - HumObj.bbox.h) / 2;

            src_pts[0] = cv::Point2f(HumObj.bbox.x, HumObj.bbox.y - offset);
            src_pts[1] = cv::Point2f(HumObj.bbox.x + HumObj.bbox.w, HumObj.bbox.y - offset);
            src_pts[2] = cv::Point2f(HumObj.bbox.x + HumObj.bbox.w, HumObj.bbox.y + HumObj.bbox.h + offset);
            src_pts[3] = cv::Point2f(HumObj.bbox.x, HumObj.bbox.y + HumObj.bbox.h + offset);
        }
        else
        {
            float offset = ((HumObj.bbox.h * (float(get_algo_width()) / float(get_algo_height()))) - HumObj.bbox.w) / 2;

            src_pts[0] = cv::Point2f(HumObj.bbox.x - offset, HumObj.bbox.y);
            src_pts[1] = cv::Point2f(HumObj.bbox.x + HumObj.bbox.w + offset, HumObj.bbox.y);
            src_pts[2] = cv::Point2f(HumObj.bbox.x + HumObj.bbox.w + offset, HumObj.bbox.y + HumObj.bbox.h);
            src_pts[3] = cv::Point2f(HumObj.bbox.x - offset, HumObj.bbox.y + HumObj.bbox.h);
        }

        cv::Point2f dst_pts[4];
        dst_pts[0] = cv::Point2f(0, 0);
        dst_pts[1] = cv::Point2f(get_algo_width(), 0);
        dst_pts[2] = cv::Point2f(get_algo_width(), get_algo_height());
        dst_pts[3] = cv::Point2f(0, get_algo_height());

        affine_trans_mat = cv::getAffineTransform(src_pts, dst_pts);
        affine_trans_mat_inv;
        cv::invertAffineTransform(affine_trans_mat, affine_trans_mat_inv);

        float mat3x3[3][3] = {
            {(float)affine_trans_mat_inv.at<double>(0, 0), (float)affine_trans_mat_inv.at<double>(0, 1), (float)affine_trans_mat_inv.at<double>(0, 2)},
            {(float)affine_trans_mat_inv.at<double>(1, 0), (float)affine_trans_mat_inv.at<double>(1, 1), (float)affine_trans_mat_inv.at<double>(1, 2)},
            {0, 0, 1}};
        // //这里要用AX_NPU_MODEL_TYPE_1_1_2
        ret = AX_NPU_CV_Warp(AX_NPU_MODEL_TYPE_1_1_2, (AX_NPU_CV_Image *)pstFrame, &dstFrame, &mat3x3[0][0], AX_NPU_CV_BILINEAR, 128);
        if (ret != 0)
        {
            return ret;
        }
    }
    else
        return -1;
    return 0;
}

int ax_model_pose_hrnet_sub::post_process(const void *pstFrame, ax_joint_runner_box_t *crop_resize_box, libaxdl_results_t *results)
{
    if (mSimpleRingBuffer.size() == 0)
    {
        mSimpleRingBuffer.resize(SAMPLE_RINGBUFFER_CACHE_COUNT);
    }
    pose::ai_body_parts_s ai_point_result;
    auto ptr = (float *)m_runner->get_output(0).pVirAddr;
    pose::hrnet_post_process(ptr, ai_point_result, SAMPLE_BODY_LMK_SIZE, get_algo_height(), get_algo_width());
    results->mObjects[cur_idx].nLandmark = SAMPLE_BODY_LMK_SIZE;
    std::vector<libaxdl_point_t> &points = mSimpleRingBuffer.next();
    points.resize(results->mObjects[cur_idx].nLandmark);
    results->mObjects[cur_idx].landmark = points.data();
    for (size_t i = 0; i < SAMPLE_BODY_LMK_SIZE; i++)
    {
        results->mObjects[cur_idx].landmark[i].x = ai_point_result.keypoints[i].x;
        results->mObjects[cur_idx].landmark[i].y = ai_point_result.keypoints[i].y;
        /*
        [x`]   [m00,m01,m02]   [x]   [m00*x + m01*y + m02]
        [y`] = [m10,m11,m12] * [y] = [m10*x + m11*y + m12]
        [1 ]   [0  ,0  ,1  ]   [1]   [          1        ]
        */
        int x = affine_trans_mat_inv.at<double>(0, 0) * results->mObjects[cur_idx].landmark[i].x + affine_trans_mat_inv.at<double>(0, 1) * results->mObjects[cur_idx].landmark[i].y + affine_trans_mat_inv.at<double>(0, 2);
        int y = affine_trans_mat_inv.at<double>(1, 0) * results->mObjects[cur_idx].landmark[i].x + affine_trans_mat_inv.at<double>(1, 1) * results->mObjects[cur_idx].landmark[i].y + affine_trans_mat_inv.at<double>(1, 2);

        results->mObjects[cur_idx].landmark[i].x = x;
        results->mObjects[cur_idx].landmark[i].y = y;
    }
    return 0;
}

int ax_model_pose_axppl_sub::post_process(const void *pstFrame, ax_joint_runner_box_t *crop_resize_box, libaxdl_results_t *results)
{
    if (mSimpleRingBuffer.size() == 0)
    {
        mSimpleRingBuffer.resize(SAMPLE_RINGBUFFER_CACHE_COUNT);
    }
    pose::ai_body_parts_s ai_point_result;
    auto ptr = (float *)m_runner->get_output(0).pVirAddr;
    auto ptr_index = (float *)m_runner->get_output(1).pVirAddr;
    pose::ppl_pose_post_process(ptr, ptr_index, ai_point_result, SAMPLE_BODY_LMK_SIZE);
    results->mObjects[cur_idx].nLandmark = SAMPLE_BODY_LMK_SIZE;
    std::vector<libaxdl_point_t> &points = mSimpleRingBuffer.next();
    points.resize(results->mObjects[cur_idx].nLandmark);
    results->mObjects[cur_idx].landmark = points.data();
    for (size_t i = 0; i < SAMPLE_BODY_LMK_SIZE; i++)
    {
        results->mObjects[cur_idx].landmark[i].x = ai_point_result.keypoints[i].x;
        results->mObjects[cur_idx].landmark[i].y = ai_point_result.keypoints[i].y;
        /*
        [x`]   [m00,m01,m02]   [x]   [m00*x + m01*y + m02]
        [y`] = [m10,m11,m12] * [y] = [m10*x + m11*y + m12]
        [1 ]   [0  ,0  ,1  ]   [1]   [          1        ]
        */
        int x = affine_trans_mat_inv.at<double>(0, 0) * results->mObjects[cur_idx].landmark[i].x + affine_trans_mat_inv.at<double>(0, 1) * results->mObjects[cur_idx].landmark[i].y + affine_trans_mat_inv.at<double>(0, 2);
        int y = affine_trans_mat_inv.at<double>(1, 0) * results->mObjects[cur_idx].landmark[i].x + affine_trans_mat_inv.at<double>(1, 1) * results->mObjects[cur_idx].landmark[i].y + affine_trans_mat_inv.at<double>(1, 2);

        results->mObjects[cur_idx].landmark[i].x = x;
        results->mObjects[cur_idx].landmark[i].y = y;
    }
    return 0;
}

int ax_model_pose_hrnet_animal_sub::post_process(const void *pstFrame, ax_joint_runner_box_t *crop_resize_box, libaxdl_results_t *results)
{
    if (mSimpleRingBuffer.size() == 0)
    {
        mSimpleRingBuffer.resize(SAMPLE_RINGBUFFER_CACHE_COUNT);
    }

    pose::ai_body_parts_s ai_point_result;
    auto ptr = (float *)m_runner->get_output(0).pVirAddr;
    pose::hrnet_post_process(ptr, ai_point_result, SAMPLE_ANIMAL_LMK_SIZE, get_algo_height(), get_algo_width());
    results->mObjects[cur_idx].nLandmark = SAMPLE_ANIMAL_LMK_SIZE;
    std::vector<libaxdl_point_t> &points = mSimpleRingBuffer.next();
    points.resize(results->mObjects[cur_idx].nLandmark);
    results->mObjects[cur_idx].landmark = points.data();
    for (size_t i = 0; i < SAMPLE_ANIMAL_LMK_SIZE; i++)
    {
        results->mObjects[cur_idx].landmark[i].x = ai_point_result.keypoints[i].x;
        results->mObjects[cur_idx].landmark[i].y = ai_point_result.keypoints[i].y;
        /*
        [x`]   [m00,m01,m02]   [x]   [m00*x + m01*y + m02]
        [y`] = [m10,m11,m12] * [y] = [m10*x + m11*y + m12]
        [1 ]   [0  ,0  ,1  ]   [1]   [          1        ]
        */
        int x = affine_trans_mat_inv.at<double>(0, 0) * results->mObjects[cur_idx].landmark[i].x + affine_trans_mat_inv.at<double>(0, 1) * results->mObjects[cur_idx].landmark[i].y + affine_trans_mat_inv.at<double>(0, 2);
        int y = affine_trans_mat_inv.at<double>(1, 0) * results->mObjects[cur_idx].landmark[i].x + affine_trans_mat_inv.at<double>(1, 1) * results->mObjects[cur_idx].landmark[i].y + affine_trans_mat_inv.at<double>(1, 2);

        results->mObjects[cur_idx].landmark[i].x = x;
        results->mObjects[cur_idx].landmark[i].y = y;
    }
    return 0;
}

int ax_model_pose_hand_sub::preprocess(const void *pstFrame, ax_joint_runner_box_t *crop_resize_box, libaxdl_results_t *results)
{
    if (!dstFrame.pVir)
    {
        dstFrame.eDtype = ((AX_NPU_CV_Image *)pstFrame)->eDtype;
        dstFrame.nHeight = get_algo_height();
        dstFrame.nWidth = get_algo_width();
        dstFrame.tStride.nW = dstFrame.nWidth;
        if (dstFrame.eDtype == AX_NPU_CV_FDT_NV12)
        {
            dstFrame.nSize = dstFrame.nHeight * dstFrame.nWidth * 3 / 2;
        }
        else if (dstFrame.eDtype == AX_NPU_CV_FDT_RGB || dstFrame.eDtype == AX_NPU_CV_FDT_BGR)
        {
            dstFrame.nSize = dstFrame.nHeight * dstFrame.nWidth * 3;
        }
        else
        {
            ALOGE("just only support nv12/rgb/bgr format\n");
            return -1;
        }
        AX_SYS_MemAlloc(&dstFrame.pPhy, (void **)&dstFrame.pVir, dstFrame.nSize, 0x100, NULL);
        bMalloc = true;
    }
    libaxdl_object_t &object = results->mObjects[cur_idx];

    cv::Point2f src_pts[4];
    src_pts[0] = cv::Point2f(object.bbox_vertices[0].x, object.bbox_vertices[0].y);
    src_pts[1] = cv::Point2f(object.bbox_vertices[1].x, object.bbox_vertices[1].y);
    src_pts[2] = cv::Point2f(object.bbox_vertices[2].x, object.bbox_vertices[2].y);
    src_pts[3] = cv::Point2f(object.bbox_vertices[3].x, object.bbox_vertices[3].y);

    cv::Point2f dst_pts[4];
    dst_pts[0] = cv::Point2f(0, 0);
    dst_pts[1] = cv::Point2f(get_algo_width(), 0);
    dst_pts[2] = cv::Point2f(get_algo_width(), get_algo_height());
    dst_pts[3] = cv::Point2f(0, get_algo_height());

    affine_trans_mat = cv::getAffineTransform(src_pts, dst_pts);
    affine_trans_mat_inv;
    cv::invertAffineTransform(affine_trans_mat, affine_trans_mat_inv);

    float mat3x3[3][3] = {
        {(float)affine_trans_mat_inv.at<double>(0, 0), (float)affine_trans_mat_inv.at<double>(0, 1), (float)affine_trans_mat_inv.at<double>(0, 2)},
        {(float)affine_trans_mat_inv.at<double>(1, 0), (float)affine_trans_mat_inv.at<double>(1, 1), (float)affine_trans_mat_inv.at<double>(1, 2)},
        {0, 0, 1}};
    // //这里要用AX_NPU_MODEL_TYPE_1_1_2
    int ret = AX_NPU_CV_Warp(AX_NPU_MODEL_TYPE_1_1_2, (AX_NPU_CV_Image *)pstFrame, &dstFrame, &mat3x3[0][0], AX_NPU_CV_BILINEAR, 128);
    if (ret)
        return ret;
    return 0;
}

int ax_model_pose_hand_sub::post_process(const void *pstFrame, ax_joint_runner_box_t *crop_resize_box, libaxdl_results_t *results)
{
    if (mSimpleRingBuffer.size() == 0)
    {
        mSimpleRingBuffer.resize(SAMPLE_RINGBUFFER_CACHE_COUNT * SAMPLE_MAX_HAND_BBOX_COUNT);
    }
    pose::ai_hand_parts_s ai_hand_point_result;
    auto &info_point = m_runner->get_output(0);
    auto &info_score = m_runner->get_output(1);
    auto point_ptr = (float *)info_point.pVirAddr;
    auto score_ptr = (float *)info_score.pVirAddr;
    pose::post_process_hand(point_ptr, score_ptr, ai_hand_point_result, SAMPLE_HAND_LMK_SIZE, get_algo_height(), get_algo_width());
    results->mObjects[cur_idx].nLandmark = SAMPLE_HAND_LMK_SIZE;
    std::vector<libaxdl_point_t> &points = mSimpleRingBuffer.next();
    points.resize(results->mObjects[cur_idx].nLandmark);
    results->mObjects[cur_idx].landmark = points.data();
    for (size_t i = 0; i < SAMPLE_HAND_LMK_SIZE; i++)
    {
        results->mObjects[cur_idx].landmark[i].x = ai_hand_point_result.keypoints[i].x;
        results->mObjects[cur_idx].landmark[i].y = ai_hand_point_result.keypoints[i].y;
        /*
        [x`]   [m00,m01,m02]   [x]   [m00*x + m01*y + m02]
        [y`] = [m10,m11,m12] * [y] = [m10*x + m11*y + m12]
        [1 ]   [0  ,0  ,1  ]   [1]   [          1        ]
        */
        int x = affine_trans_mat_inv.at<double>(0, 0) * results->mObjects[cur_idx].landmark[i].x + affine_trans_mat_inv.at<double>(0, 1) * results->mObjects[cur_idx].landmark[i].y + affine_trans_mat_inv.at<double>(0, 2);
        int y = affine_trans_mat_inv.at<double>(1, 0) * results->mObjects[cur_idx].landmark[i].x + affine_trans_mat_inv.at<double>(1, 1) * results->mObjects[cur_idx].landmark[i].y + affine_trans_mat_inv.at<double>(1, 2);
        results->mObjects[cur_idx].landmark[i].x = x;
        results->mObjects[cur_idx].landmark[i].y = y;
    }
    return 0;
}

void align_face(libaxdl_object_t &obj, AX_NPU_CV_Image *npu_image, AX_NPU_CV_Image &npu_image_face_align)
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

void ax_model_face_feat_extactor_sub::_normalize(float *feature, int feature_len)
{
    float sum = 0;
    for (int it = 0; it < feature_len; it++)
        sum += feature[it] * feature[it];
    sum = sqrt(sum);
    for (int it = 0; it < feature_len; it++)
        feature[it] /= sum;
}

int ax_model_face_feat_extactor_sub::preprocess(const void *pstFrame, ax_joint_runner_box_t *crop_resize_box, libaxdl_results_t *results)
{
    if (!dstFrame.pVir)
    {
        dstFrame.nWidth = dstFrame.nHeight = dstFrame.tStride.nW = 112;
        AX_SYS_MemAlloc((AX_U64 *)&dstFrame.pPhy, (void **)&dstFrame.pVir, 112 * 112 * 3, 0x100, (AX_S8 *)"SAMPLE-CV");
        bMalloc = true;
    }
    libaxdl_object_t &obj = results->mObjects[cur_idx];
    align_face(obj, (AX_NPU_CV_Image *)pstFrame, dstFrame);
    return 0;
}

int ax_model_face_feat_extactor_sub::post_process(const void *pstFrame, ax_joint_runner_box_t *crop_resize_box, libaxdl_results_t *results)
{
    if (mSimpleRingBuffer_FaceFeat.size() == 0)
    {
        mSimpleRingBuffer_FaceFeat.resize(SAMPLE_RINGBUFFER_CACHE_COUNT * SAMPLE_MAX_BBOX_COUNT);
    }
    auto &feat = mSimpleRingBuffer_FaceFeat.next();
    feat.resize(SAMPLE_FACE_FEAT_LEN);
    memcpy(feat.data(), m_runner->get_output(0).pVirAddr, SAMPLE_FACE_FEAT_LEN * sizeof(float));
    _normalize(feat.data(), SAMPLE_FACE_FEAT_LEN);
    results->mObjects[cur_idx].mFaceFeat.w = SAMPLE_FACE_FEAT_LEN * 4;
    results->mObjects[cur_idx].mFaceFeat.h = 1;
    results->mObjects[cur_idx].mFaceFeat.data = (unsigned char *)feat.data();
    return 0;
}

int ax_model_license_plate_recognition_sub::preprocess(const void *pstFrame, ax_joint_runner_box_t *crop_resize_box, libaxdl_results_t *results)
{
    if (!dstFrame.pVir)
    {
        dstFrame.eDtype = ((AX_NPU_CV_Image *)pstFrame)->eDtype;
        dstFrame.nHeight = get_algo_height();
        dstFrame.nWidth = get_algo_width();
        dstFrame.tStride.nW = dstFrame.nWidth;
        if (dstFrame.eDtype == AX_NPU_CV_FDT_NV12)
        {
            dstFrame.nSize = dstFrame.nHeight * dstFrame.nWidth * 3 / 2;
        }
        else if (dstFrame.eDtype == AX_NPU_CV_FDT_RGB || dstFrame.eDtype == AX_NPU_CV_FDT_BGR)
        {
            dstFrame.nSize = dstFrame.nHeight * dstFrame.nWidth * 3;
        }
        else
        {
            ALOGE("just only support nv12/rgb/bgr format\n");
            return -1;
        }
        AX_SYS_MemAlloc(&dstFrame.pPhy, (void **)&dstFrame.pVir, dstFrame.nSize, 0x100, NULL);
        bMalloc = true;
    }
    libaxdl_object_t &object = results->mObjects[cur_idx];
    cv::Point2f src_pts[4];
    src_pts[0] = cv::Point2f(object.bbox_vertices[0].x, object.bbox_vertices[0].y);
    src_pts[1] = cv::Point2f(object.bbox_vertices[1].x, object.bbox_vertices[1].y);
    src_pts[2] = cv::Point2f(object.bbox_vertices[2].x, object.bbox_vertices[2].y);
    src_pts[3] = cv::Point2f(object.bbox_vertices[3].x, object.bbox_vertices[3].y);

    cv::Point2f dst_pts[4];
    dst_pts[0] = cv::Point2f(0, 0);
    dst_pts[1] = cv::Point2f(get_algo_width(), 0);
    dst_pts[2] = cv::Point2f(get_algo_width(), get_algo_height());
    dst_pts[3] = cv::Point2f(0, get_algo_height());

    affine_trans_mat = cv::getAffineTransform(src_pts, dst_pts);
    affine_trans_mat_inv;
    cv::invertAffineTransform(affine_trans_mat, affine_trans_mat_inv);

    float mat3x3[3][3] = {
        {(float)affine_trans_mat_inv.at<double>(0, 0), (float)affine_trans_mat_inv.at<double>(0, 1), (float)affine_trans_mat_inv.at<double>(0, 2)},
        {(float)affine_trans_mat_inv.at<double>(1, 0), (float)affine_trans_mat_inv.at<double>(1, 1), (float)affine_trans_mat_inv.at<double>(1, 2)},
        {0, 0, 1}};
    // //这里要用AX_NPU_MODEL_TYPE_1_1_2
    int ret = AX_NPU_CV_Warp(AX_NPU_MODEL_TYPE_1_1_2, (AX_NPU_CV_Image *)pstFrame, &dstFrame, &mat3x3[0][0], AX_NPU_CV_BILINEAR, 128);
    if (ret != 0)
    {
        return ret;
    }
    return 0;
}

int ax_model_license_plate_recognition_sub::post_process(const void *pstFrame, ax_joint_runner_box_t *crop_resize_box, libaxdl_results_t *results)
{
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

    float *outputdata = (float *)m_runner->get_output(0).pVirAddr;

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
    // ALOGI("%s %d",plate.c_str(),plate.length());
    sprintf(results->mObjects[cur_idx].objname, plate.c_str());

    return 0;
}