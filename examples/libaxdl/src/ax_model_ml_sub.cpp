#include "ax_model_ml_sub.hpp"
#include "base/pose.hpp"

#include "../../utilities/sample_log.h"
#include "utilities/mat_pixel_affine.h"

// #include "ax_sys_api.h"
#include "ax_common_api.h"

int ax_model_pose_hrnet_sub::post_process(axdl_image_t *pstFrame, axdl_bbox_t *crop_resize_box, axdl_results_t *results)
{
    if (mSimpleRingBuffer.size() == 0)
    {
        mSimpleRingBuffer.resize(SAMPLE_RINGBUFFER_CACHE_COUNT * MAX_SUB_INFER_COUNT);
    }
    axdl_object_t &HumObj = results->mObjects[cur_idx];
    pose::ai_body_parts_s ai_point_result;
    auto ptr = (float *)m_runner->get_output(0).pVirAddr;
    pose::hrnet_post_process(ptr, ai_point_result, SAMPLE_BODY_LMK_SIZE, get_algo_height(), get_algo_width());
    results->mObjects[cur_idx].nLandmark = SAMPLE_BODY_LMK_SIZE;
    std::vector<axdl_point_t> &points = mSimpleRingBuffer.next();
    points.resize(results->mObjects[cur_idx].nLandmark);
    results->mObjects[cur_idx].landmark = points.data();

    float offset_w = 0, offset_h = 0;
    if ((HumObj.bbox.w / HumObj.bbox.h) >
        (float(get_algo_width()) / float(get_algo_height())))
    {
        offset_h = ((HumObj.bbox.w * (float(get_algo_height()) / float(get_algo_width()))) - HumObj.bbox.h) / 2;
    }
    else
    {
        offset_w = ((HumObj.bbox.h * (float(get_algo_width()) / float(get_algo_height()))) - HumObj.bbox.w) / 2;
    }

    for (size_t i = 0; i < SAMPLE_BODY_LMK_SIZE; i++)
    {
        results->mObjects[cur_idx].landmark[i].x = ai_point_result.keypoints[i].x / get_algo_width() * (HumObj.bbox.w + 2 * offset_w) + HumObj.bbox.x - offset_w;
        results->mObjects[cur_idx].landmark[i].y = ai_point_result.keypoints[i].y / get_algo_height() * (HumObj.bbox.h + 2 * offset_h) + HumObj.bbox.y - offset_h;
    }
    return 0;
}

int ax_model_pose_axppl_sub::post_process(axdl_image_t *pstFrame, axdl_bbox_t *crop_resize_box, axdl_results_t *results)
{
    if (mSimpleRingBuffer.size() == 0)
    {
        mSimpleRingBuffer.resize(SAMPLE_RINGBUFFER_CACHE_COUNT * MAX_SUB_INFER_COUNT);
    }
    axdl_object_t &HumObj = results->mObjects[cur_idx];
    pose::ai_body_parts_s ai_point_result;
    auto ptr = (float *)m_runner->get_output(0).pVirAddr;
    auto ptr_index = (float *)m_runner->get_output(1).pVirAddr;
    pose::ppl_pose_post_process(ptr, ptr_index, ai_point_result, SAMPLE_BODY_LMK_SIZE);
    results->mObjects[cur_idx].nLandmark = SAMPLE_BODY_LMK_SIZE;
    std::vector<axdl_point_t> &points = mSimpleRingBuffer.next();
    points.resize(results->mObjects[cur_idx].nLandmark);
    results->mObjects[cur_idx].landmark = points.data();

    float offset_w = 0, offset_h = 0;
    if ((HumObj.bbox.w / HumObj.bbox.h) >
        (float(get_algo_width()) / float(get_algo_height())))
    {
        offset_h = ((HumObj.bbox.w * (float(get_algo_height()) / float(get_algo_width()))) - HumObj.bbox.h) / 2;
    }
    else
    {
        offset_w = ((HumObj.bbox.h * (float(get_algo_width()) / float(get_algo_height()))) - HumObj.bbox.w) / 2;
    }

    for (size_t i = 0; i < SAMPLE_BODY_LMK_SIZE; i++)
    {
        results->mObjects[cur_idx].landmark[i].x = ai_point_result.keypoints[i].x / get_algo_width() * (HumObj.bbox.w + 2 * offset_w) + HumObj.bbox.x - offset_w;
        results->mObjects[cur_idx].landmark[i].y = ai_point_result.keypoints[i].y / get_algo_height() * (HumObj.bbox.h + 2 * offset_h) + HumObj.bbox.y - offset_h;
    }

    return 0;
}

int ax_model_pose_hrnet_animal_sub::post_process(axdl_image_t *pstFrame, axdl_bbox_t *crop_resize_box, axdl_results_t *results)
{
    if (mSimpleRingBuffer.size() == 0)
    {
        mSimpleRingBuffer.resize(SAMPLE_RINGBUFFER_CACHE_COUNT * MAX_SUB_INFER_COUNT);
    }
    axdl_object_t &HumObj = results->mObjects[cur_idx];
    pose::ai_body_parts_s ai_point_result;
    auto ptr = (float *)m_runner->get_output(0).pVirAddr;
    pose::hrnet_post_process(ptr, ai_point_result, SAMPLE_ANIMAL_LMK_SIZE, get_algo_height(), get_algo_width());
    results->mObjects[cur_idx].nLandmark = SAMPLE_ANIMAL_LMK_SIZE;
    std::vector<axdl_point_t> &points = mSimpleRingBuffer.next();
    points.resize(results->mObjects[cur_idx].nLandmark);
    results->mObjects[cur_idx].landmark = points.data();

    float offset_w = 0, offset_h = 0;
    if ((HumObj.bbox.w / HumObj.bbox.h) >
        (float(get_algo_width()) / float(get_algo_height())))
    {
        offset_h = ((HumObj.bbox.w * (float(get_algo_height()) / float(get_algo_width()))) - HumObj.bbox.h) / 2;
    }
    else
    {
        offset_w = ((HumObj.bbox.h * (float(get_algo_width()) / float(get_algo_height()))) - HumObj.bbox.w) / 2;
    }

    for (size_t i = 0; i < SAMPLE_ANIMAL_LMK_SIZE; i++)
    {
        results->mObjects[cur_idx].landmark[i].x = ai_point_result.keypoints[i].x / get_algo_width() * (HumObj.bbox.w + 2 * offset_w) + HumObj.bbox.x - offset_w;
        results->mObjects[cur_idx].landmark[i].y = ai_point_result.keypoints[i].y / get_algo_height() * (HumObj.bbox.h + 2 * offset_h) + HumObj.bbox.y - offset_h;
    }
    
    return 0;
}

int ax_model_pose_hand_sub::preprocess(axdl_image_t *pstFrame, axdl_bbox_t *crop_resize_box, axdl_results_t *results)
{
    if (!dstFrame.pVir)
    {
        dstFrame.eDtype = pstFrame->eDtype;
        dstFrame.nHeight = get_algo_height();
        dstFrame.nWidth = get_algo_width();
        dstFrame.tStride_W = dstFrame.nWidth;
        if (dstFrame.eDtype == axdl_color_space_nv12)
        {
            dstFrame.nSize = dstFrame.nHeight * dstFrame.nWidth * 3 / 2;
        }
        else if (dstFrame.eDtype == axdl_color_space_rgb || dstFrame.eDtype == axdl_color_space_bgr)
        {
            dstFrame.eDtype = axdl_color_space_bgr;
            dstFrame.nSize = dstFrame.nHeight * dstFrame.nWidth * 3;
        }
        else
        {
            ALOGE("just only support nv12/rgb/bgr format\n");
            return -1;
        }
        ax_sys_memalloc(&dstFrame.pPhy, (void **)&dstFrame.pVir, dstFrame.nSize, 0x100, NULL);
        bMalloc = true;
    }
    axdl_object_t &object = results->mObjects[cur_idx];

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
    int ret = ax_imgproc_warp(pstFrame, &dstFrame, &mat3x3[0][0], 128);
    if (ret)
        return ret;
    return 0;
}

int ax_model_pose_hand_sub::post_process(axdl_image_t *pstFrame, axdl_bbox_t *crop_resize_box, axdl_results_t *results)
{
    if (mSimpleRingBuffer.size() == 0)
    {
        mSimpleRingBuffer.resize(SAMPLE_RINGBUFFER_CACHE_COUNT * MAX_SUB_INFER_COUNT);
    }
    pose::ai_hand_parts_s ai_hand_point_result;
    auto &info_point = m_runner->get_output(0);
    auto &info_score = m_runner->get_output(1);
    auto point_ptr = (float *)info_point.pVirAddr;
    auto score_ptr = (float *)info_score.pVirAddr;
    pose::post_process_hand(point_ptr, score_ptr, ai_hand_point_result, SAMPLE_HAND_LMK_SIZE, get_algo_height(), get_algo_width());
    results->mObjects[cur_idx].nLandmark = SAMPLE_HAND_LMK_SIZE;
    std::vector<axdl_point_t> &points = mSimpleRingBuffer.next();
    points.resize(results->mObjects[cur_idx].nLandmark);
    results->mObjects[cur_idx].landmark = points.data();
    for (size_t i = 0; i < SAMPLE_HAND_LMK_SIZE; i++)
    {
        results->mObjects[cur_idx].landmark[i].x = ai_hand_point_result.keypoints[i].x;
        results->mObjects[cur_idx].landmark[i].y = ai_hand_point_result.keypoints[i].y;
        results->mObjects[cur_idx].landmark[i].score = ai_hand_point_result.keypoints[i].score;
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

void ax_model_face_feat_extactor_sub::_normalize(float *feature, int feature_len)
{
    float sum = 0;
    for (int it = 0; it < feature_len; it++)
        sum += feature[it] * feature[it];
    sum = sqrt(sum);
    for (int it = 0; it < feature_len; it++)
        feature[it] /= sum;
}

int ax_model_face_feat_extactor_sub::preprocess(axdl_image_t *pstFrame, axdl_bbox_t *crop_resize_box, axdl_results_t *results)
{
    if (!dstFrame.pVir)
    {
        dstFrame.nWidth = dstFrame.nHeight = dstFrame.tStride_W = 112;
        ax_sys_memalloc(&dstFrame.pPhy, (void **)&dstFrame.pVir, 112 * 112 * 3, 0x100, "SAMPLE-CV");
        bMalloc = true;
    }
    axdl_object_t &obj = results->mObjects[cur_idx];
    ax_imgproc_align_face(&obj, pstFrame, &dstFrame);
    return 0;
}

int ax_model_face_feat_extactor_sub::post_process(axdl_image_t *pstFrame, axdl_bbox_t *crop_resize_box, axdl_results_t *results)
{
    if (mSimpleRingBuffer_FaceFeat.size() == 0)
    {
        mSimpleRingBuffer_FaceFeat.resize(SAMPLE_RINGBUFFER_CACHE_COUNT * SAMPLE_MAX_BBOX_COUNT);
    }
    auto &feat = mSimpleRingBuffer_FaceFeat.next();
    feat.resize(FACE_FEAT_LEN);
    memcpy(feat.data(), m_runner->get_output(0).pVirAddr, FACE_FEAT_LEN * sizeof(float));
    _normalize(feat.data(), FACE_FEAT_LEN);
    results->mObjects[cur_idx].mFaceFeat.w = FACE_FEAT_LEN * 4;
    results->mObjects[cur_idx].mFaceFeat.h = 1;
    results->mObjects[cur_idx].mFaceFeat.c = 1;
    results->mObjects[cur_idx].mFaceFeat.s = results->mObjects[cur_idx].mFaceFeat.w;
    results->mObjects[cur_idx].mFaceFeat.data = (unsigned char *)feat.data();
    return 0;
}

int ax_model_license_plate_recognition_sub::preprocess(axdl_image_t *pstFrame, axdl_bbox_t *crop_resize_box, axdl_results_t *results)
{
    if (!dstFrame.pVir)
    {
        dstFrame.eDtype = pstFrame->eDtype;
        dstFrame.nHeight = get_algo_height();
        dstFrame.nWidth = get_algo_width();
        dstFrame.tStride_W = dstFrame.nWidth;
        if (dstFrame.eDtype == axdl_color_space_nv12)
        {
            dstFrame.nSize = dstFrame.nHeight * dstFrame.nWidth * 3 / 2;
        }
        else if (dstFrame.eDtype == axdl_color_space_rgb || dstFrame.eDtype == axdl_color_space_bgr)
        {
            dstFrame.eDtype = axdl_color_space_bgr;
            dstFrame.nSize = dstFrame.nHeight * dstFrame.nWidth * 3;
        }
        else
        {
            ALOGE("just only support nv12/rgb/bgr format\n");
            return -1;
        }
        ax_sys_memalloc(&dstFrame.pPhy, (void **)&dstFrame.pVir, dstFrame.nSize, 0x100, NULL);
        bMalloc = true;
    }
    axdl_object_t &object = results->mObjects[cur_idx];
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
    int ret = ax_imgproc_warp(pstFrame, &dstFrame, &mat3x3[0][0], 128);
    if (ret != 0)
    {
        return ret;
    }
    return 0;
}

int ax_model_license_plate_recognition_sub::post_process(axdl_image_t *pstFrame, axdl_bbox_t *crop_resize_box, axdl_results_t *results)
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