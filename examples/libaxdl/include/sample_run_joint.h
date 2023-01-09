/*
 * AXERA is pleased to support the open source community by making ax-samples available.
 *
 * Copyright (c) 2022, AXERA Semiconductor (Shanghai) Co., Ltd. All rights reserved.
 *
 * Licensed under the BSD 3-Clause License (the "License"); you may not use this file except
 * in compliance with the License. You may obtain a copy of the License at
 *
 * https://opensource.org/licenses/BSD-3-Clause
 *
 * Unless required by applicable law or agreed to in writing, software distributed
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations under the License.
 */

/*
 * Author: ZHEQIUSHUI
 */

#ifndef _SAMPLE_RUN_JOINT_H_
#define _SAMPLE_RUN_JOINT_H_
#include "joint.h"

typedef struct _sample_run_joint_some_attr_you_need
{
    /// @param algo_width 算法的输入宽
    /// @param algo_height 算法的输入高
    /// @param algo_colorformat 来自 AX_IMG_FORMAT_E ，AX_YUV420_SEMIPLANAR = 1 ，AX_FORMAT_RGB888 = 65 ， AX_FORMAT_BGR888 = 83
    int algo_width, algo_height;
    int algo_colorformat;

    /// @param nOutputSize 模型输出的节点个数
    /// @param pOutputsInfo 模型输出的节点信息数组指针，数组的长度对应 @param nOutputSize
    /// @param pOutputs 模型输出的节点张量数组指针，数组的长度对应 @param nOutputSize
    AX_U32 nOutputSize;
    AX_JOINT_IOMETA_T *pOutputsInfo;
    AX_JOINT_IO_BUFFER_T *pOutputs;

    // 你可以在这添加一些你需要的其他模型的信息，并填充
} sample_run_joint_attr;

#ifdef __cplusplus
extern "C"
{
#endif
    /// @brief 创建算法handle，并输出算法的输入分辨率，以供其他模块设置
    /// @param model_file joint算法模型路径
    /// @param handle 输出的算法handle
    /// @param attr 一些你可能需要的参数输出，推理完之后，模型输出的数据会自动填充到这里面的结构体指针里
    /// @return
    int sample_run_joint_init(char *model_file, void **handle, sample_run_joint_attr *attr);

    /// @brief 推理输入的图像，检测出bbox，并将映射到 src_width/src_height 上，以便 osd 绘制
    /// @param handle 算法handle
    /// @param pstFrame AX_NPU_CV_Image结构体指针，使用 const void* 不引用"npu_cv_kit/ax_npu_imgproc.h"头文件是因为用了会报重定义错误，新版本SDK已经解决
    /// @param crop_resize_box AX_NPU_CV_Box结构体指针，需要扣的部分图像，如果不需要扣，就置为 NULL，使用 const void* 原因同上
    /// @return
    int sample_run_joint_inference(void *handle, const void *pstFrame, const void *crop_resize_box);

    int sample_run_joint_release(void *handle);
#ifdef __cplusplus
}
#endif

#endif