#include "ax_model_seg.hpp"
#include "opencv2/opencv.hpp"

int ax_model_pphumseg::post_process(const void *pstFrame, ax_joint_runner_box *crop_resize_box, sample_run_joint_results *results)
{
    results->bPPHumSeg = 1;
    auto ptr = (float *)m_runner.m_attr.pOutputs[0].pVirAddr;
    // static SimpleRingBuffer<cv::Mat> mSimpleRingBuffer(SAMPLE_RINGBUFFER_CACHE_COUNT);
    if (mSimpleRingBuffer.size() == 0)
    {
        mSimpleRingBuffer.resize(SAMPLE_RINGBUFFER_CACHE_COUNT);
    }

    int seg_h = m_runner.m_attr.pOutputsInfo->pShape[2];
    int seg_w = m_runner.m_attr.pOutputsInfo->pShape[3];
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