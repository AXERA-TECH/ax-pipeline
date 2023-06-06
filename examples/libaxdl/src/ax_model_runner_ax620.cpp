#ifdef AXERA_TARGET_CHIP_AX620
#include "ax_model_runner_ax620.hpp"
#include "../../utilities/sample_log.h"

#include "sample_run_joint.h"

#include "npu_cv_kit/ax_npu_imgproc.h"

struct ax_joint_runner_ax620_handle_t
{
    void *m_handle = nullptr;
    sample_run_joint_attr m_attr = {0};
};

int ax_runner_ax620::init(const char *model_file)
{
    if (m_handle)
    {
        return -1;
    }
    m_handle = new ax_joint_runner_ax620_handle_t;
    int ret = sample_run_joint_init((char *)model_file, &m_handle->m_handle, &m_handle->m_attr);
    if (ret)
    {
        ALOGE("sample_run_joint_init failed,s32Ret:0x%x", ret);
        return ret;
    }
    for (size_t i = 0; i < m_handle->m_attr.nOutputSize; i++)
    {
        ax_runner_tensor_t tensor;
        tensor.nIdx = i;
        tensor.sName = std::string(m_handle->m_attr.pOutputsInfo[i].pName);
        tensor.nSize = m_handle->m_attr.pOutputsInfo[i].nSize;
        for (size_t j = 0; j < m_handle->m_attr.pOutputsInfo[i].nShapeSize; j++)
        {
            tensor.vShape.push_back(m_handle->m_attr.pOutputsInfo[i].pShape[j]);
        }
        tensor.phyAddr = m_handle->m_attr.pOutputs[i].phyAddr;
        tensor.pVirAddr = m_handle->m_attr.pOutputs[i].pVirAddr;
        mtensors.push_back(tensor);
    }

    return ret;
}

void ax_runner_ax620::deinit()
{
    if (m_handle && m_handle->m_handle)
    {
        sample_run_joint_release(m_handle->m_handle);
    }
    delete m_handle;
    m_handle = nullptr;
}

int ax_runner_ax620::get_algo_width() { return m_handle->m_attr.algo_width; }
int ax_runner_ax620::get_algo_height() { return m_handle->m_attr.algo_height; }
axdl_color_space_e ax_runner_ax620::get_color_space()
{
    switch (m_handle->m_attr.algo_colorformat)
    {
    case AX_FORMAT_RGB888:
        return axdl_color_space_e::axdl_color_space_rgb;
    case AX_FORMAT_BGR888:
        return axdl_color_space_e::axdl_color_space_bgr;
    case AX_YUV420_SEMIPLANAR:
        return axdl_color_space_e::axdl_color_space_nv12;
    default:
        return axdl_color_space_unknown;
    }
}

int ax_runner_ax620::inference(axdl_image_t *pstFrame, const axdl_bbox_t *crop_resize_box)
{
    return sample_run_joint_inference(m_handle->m_handle, pstFrame, crop_resize_box);
}
#endif