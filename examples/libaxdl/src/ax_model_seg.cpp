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

    cv::Mat &seg_mat = mSimpleRingBuffer.next();
    if (seg_mat.empty())
    {
        seg_mat = cv::Mat(seg_h, seg_w, CV_8UC1);
    }
    results->mPPHumSeg.h = seg_h;
    results->mPPHumSeg.w = seg_w;
    results->mPPHumSeg.data = seg_mat.data;

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