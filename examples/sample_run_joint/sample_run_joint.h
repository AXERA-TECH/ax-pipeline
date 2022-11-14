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

#define SAMPLE_MAX_BBOX_COUNT 64
#define SAMPLE_OBJ_NAME_MAX_LEN 16

typedef struct _sample_run_joint_object
{
    float x, y, w, h;
    int label;
    float prob;
    char objname[SAMPLE_OBJ_NAME_MAX_LEN];
} sample_run_joint_object;

typedef struct _sample_run_joint_results
{
    int size;
    sample_run_joint_object objects[SAMPLE_MAX_BBOX_COUNT];
} sample_run_joint_results;

#ifdef __cplusplus
extern "C"
{
#endif
    /// @brief 创建算法handle，并输出算法的输入分辨率，以供其他模块设置
    /// @param model_file joint算法模型路径
    /// @param handle 输出的算法handle
    /// @param algo_width 算法的输入宽
    /// @param algo_height 算法的输入高
    /// @param algo_colorformat 来自 AX_IMG_FORMAT_E ，AX_YUV420_SEMIPLANAR = 1 ，AX_FORMAT_RGB888 = 65 ， AX_FORMAT_BGR888 = 83
    /// @return 
    int sample_run_joint_init(char *model_file, void **handle, int *algo_width, int *algo_height, int *algo_colorformat);
    /// @brief 推理输入的图像，检测出bbox，并将映射到 src_width/src_height 上，以便 osd 绘制
    /// @param handle 算法handle
    /// @param pstFrame AX_NPU_CV_Image结构体指针
    /// @param src_width 需要映射的目标分辨率的宽
    /// @param src_height 需要映射的目标分辨率的高
    /// @param pResults 推理结果
    /// @return 
    int sample_run_joint_inference(void *handle, const void *pstFrame, int src_width, int src_height, sample_run_joint_results *pResults);
    int sample_run_joint_release(void *handle);
    /// @brief 通过 json 解析 yolov5 所需的参数，如果某些 key 置空，则不更新该参数
    /// @param json_file_path 
    /// @return 
    int sample_parse_yolov5_param(char *json_file_path);
#ifdef __cplusplus
}
#endif

#endif