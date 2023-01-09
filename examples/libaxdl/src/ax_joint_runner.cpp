#include "../include/ax_joint_runner.hpp"
#include "../../utilities/sample_log.h"

int ax_joint_runner::init(const char *model_file)
{
    if (m_handle)
    {
        return -1;
    }
    int ret = sample_run_joint_init((char *)model_file, &m_handle, &m_attr);
    if (ret)
    {
        ALOGE("sample_run_joint_init failed,s32Ret:0x%x", ret);
    }
    return ret;
}

void ax_joint_runner::deinit()
{
    if (m_handle)
    {
        sample_run_joint_release(m_handle);
    }
    m_handle = nullptr;
}

int ax_joint_runner::inference(const void *pstFrame, const ax_joint_runner_box *crop_resize_box)
{
    return sample_run_joint_inference(m_handle, pstFrame, crop_resize_box);
}