#include "ax_model_seg.hpp"
#include "opencv2/opencv.hpp"

int ax_model_pphumseg::post_process(axdl_image_t *pstFrame, axdl_bbox_t *crop_resize_box, axdl_results_t *results)
{
    results->bPPHumSeg = 1;
    auto ptr = (float *)m_runner->get_output(0).pVirAddr;
    if (mSimpleRingBuffer.size() == 0)
    {
        mSimpleRingBuffer.resize(SAMPLE_RINGBUFFER_CACHE_COUNT);
    }

    int seg_h = m_runner->get_output(0).vShape[2];
    int seg_w = m_runner->get_output(0).vShape[3];
    int seg_size = seg_h * seg_w;

    auto &seg_mat_ptr = mSimpleRingBuffer.next();
    if (!seg_mat_ptr.get())
    {
        seg_mat_ptr.reset(new unsigned char[seg_h * seg_w], std::default_delete<unsigned char[]>());
        // seg_mat = cv::Mat(seg_h, seg_w, CV_8UC1);
    }
    results->mPPHumSeg.h = seg_h;
    results->mPPHumSeg.w = seg_w;
    results->mPPHumSeg.c = 1;
    results->mPPHumSeg.s = seg_w;
    results->mPPHumSeg.data = seg_mat_ptr.get();

    for (int j = 0; j < seg_h * seg_w; ++j)
    {
        results->mPPHumSeg.data[j] = (ptr[j] < ptr[j + seg_size]) ? 255 : 0;
    }
    return 0;
}

void ax_model_pphumseg::draw_custom(cv::Mat &image, axdl_results_t *results, float fontscale, int thickness, int offset_x, int offset_y)
{
    if (!results->bPPHumSeg || !results->mPPHumSeg.data)
    {
        return;
    }
    if (base_canvas.empty() || base_canvas.rows * base_canvas.cols < image.rows * image.cols)
    {
        base_canvas = cv::Mat(image.rows, image.cols, CV_8UC1);
    }
    cv::Mat tmp(image.rows, image.cols, CV_8UC1, base_canvas.data);
    cv::Mat mask(results->mPPHumSeg.h, results->mPPHumSeg.w, CV_8UC1, results->mPPHumSeg.data);
    cv::resize(mask, tmp, cv::Size(image.cols, image.rows), 0, 0, cv::INTER_NEAREST);
    image.setTo(cv::Scalar(66, 0, 0, 128), tmp);
}

void ax_model_pphumseg::draw_custom(int chn, axdl_results_t *results, float fontscale, int thickness)
{
    if (!results->bPPHumSeg || !results->mPPHumSeg.data)
    {
        return;
    }
    cv::Mat mask_mat(results->mPPHumSeg.h, results->mPPHumSeg.w, CV_8U, results->mPPHumSeg.data);
    cv::Mat mask_color(results->mPPHumSeg.h, results->mPPHumSeg.w, CV_8UC4);
    memset(mask_color.data, 0, mask_color.cols * mask_color.rows * 4);
    mask_color.setTo(cv::Scalar(66, 0, 0, 128), mask_mat);
    axdl_mat_t mask;
    mask.data = mask_color.data;
    mask.w = mask_color.cols;
    mask.h = mask_color.rows;
    mask.s = mask_color.step1();
    mask.c = 4;
    m_drawers[chn].add_mask(nullptr, &mask);
}

int ax_model_dinov2::post_process(axdl_image_t *pstFrame, axdl_bbox_t *crop_resize_box, axdl_results_t *results)
{
    if (mSimpleRingBuffer.size() == 0)
    {
        mSimpleRingBuffer.resize(SAMPLE_RINGBUFFER_CACHE_COUNT);
    }
    results->bPPHumSeg = 1;
    auto ptr = (float *)m_runner->get_output(0).pVirAddr;

    cv::Mat feature(m_runner->get_output(0).vShape[1], m_runner->get_output(0).vShape[2], CV_32FC1, ptr);
    cv::resize(feature, feature, cv::Size(feature.cols / 4, feature.rows), 0, 0, cv::InterpolationFlags::INTER_NEAREST);
    if (cnt++ % 100 == 0)
    {
        // bPCA_learn = true;
        pca(feature, cv::noArray(), cv::PCA::Flags::DATA_AS_ROW, 3);
    }

    cv::Mat pca_features = pca.project(feature);
    double minVal, maxVal;
    cv::minMaxLoc(pca_features, &minVal, &maxVal);
    pca_features -= minVal;
    pca_features /= (maxVal - minVal);
    pca_features *= 255;

    auto &seg_mat_ptr = mSimpleRingBuffer.next();
    if (!seg_mat_ptr.get())
    {
        seg_mat_ptr.reset(new unsigned char[37 * 37 * 4], std::default_delete<unsigned char[]>());
        // seg_mat = cv::Mat(37, 37, CV_8UC4, cv::Scalar(255, 0, 0, 200));
    }

    float *pca_features_data = (float *)pca_features.data;
    uchar *out_data = seg_mat_ptr.get();
    for (size_t i = 0; i < pca_features.cols * pca_features.rows / 3; i++)
    {
        out_data[4 * i + 0] = 255;
        out_data[4 * i + 1] = pca_features_data[3 * i + 0];
        out_data[4 * i + 2] = pca_features_data[3 * i + 1];
        out_data[4 * i + 3] = pca_features_data[3 * i + 2];
    }

    results->mPPHumSeg.h = 37;
    results->mPPHumSeg.w = 37;
    results->mPPHumSeg.c = 4;
    results->mPPHumSeg.s = 37 * 4;
    results->mPPHumSeg.data = seg_mat_ptr.get();

    return 0;
}

void ax_model_dinov2::draw_custom(cv::Mat &image, axdl_results_t *results, float fontscale, int thickness, int offset_x, int offset_y)
{
    if (!results->bPPHumSeg || !results->mPPHumSeg.data)
    {
        return;
    }
    // if (base_canvas.empty() || base_canvas.rows * base_canvas.cols < image.rows * image.cols)
    // {
    //     base_canvas = cv::Mat(image.rows, image.cols, CV_8UC4);
    // }
    // cv::Mat tmp(image.rows, image.cols, CV_8UC4, base_canvas.data);
    cv::Mat mask(results->mPPHumSeg.h, results->mPPHumSeg.w, CV_8UC4, results->mPPHumSeg.data);
    cv::resize(mask, image, cv::Size(image.cols, image.rows));
    // image.setTo(cv::Scalar(66, 0, 0, 128), tmp);
}

void ax_model_dinov2::draw_custom(int chn, axdl_results_t *results, float fontscale, int thickness)
{
    if (!results->bPPHumSeg || !results->mPPHumSeg.data)
    {
        return;
    }
    axdl_bbox_t box{0, 0, 0.25, 0.25};
    m_drawers[chn].add_mask(&box, &results->mPPHumSeg);
}