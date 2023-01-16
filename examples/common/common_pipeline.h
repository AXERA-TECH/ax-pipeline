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

#ifndef _PIPELINE_COMMON_H_
#define _PIPELINE_COMMON_H_

#ifndef VDEC_LINK_MODE
#define VDEC_LINK_MODE 1
#endif

#if __cplusplus
extern "C"
{
#endif
    typedef enum
    {
        pi_none = 0x00,
        // user send nv12 frame
        pi_user,
        // link to vin
        pi_vin,
        // link to vdec
        pi_vdec_h264,
        pi_vdec_jpeg,
    } pipeline_input_e;

    typedef enum
    {
        po_none = 0x00,

        po_buff = 0x10,
        po_buff_rgb,
        po_buff_bgr,
        po_buff_nv12,
        po_buff_nv21,

        po_venc = 0x20,
        po_venc_mjpg,
        po_venc_h264,
        po_venc_h265,

        po_rtsp = 0x30,
        po_rtsp_h264,
        po_rtsp_h265,

        po_vo = 0x40,
        po_vo_sipeed_maix3_screen,
    } pipeline_output_e;

    typedef struct
    {
#define MAX_IVPS_GRP_COUNT 20
        int n_ivps_grp; // 少于20，并且不能重复

        int n_osd_rgn; // rgn的个数，设为0则表示不进行osd，少于 MAX_OSD_RGN_COUNT 个，每一个rgn可以绘制32个目标
#define MAX_OSD_RGN_COUNT 5
        int n_osd_rgn_chn[MAX_OSD_RGN_COUNT]; // rgn 的句柄

        int n_ivps_fps; // 输出的帧率

        // 0-0 1-90 2-180 3-270
        int n_ivps_rotate;

        // must be even number，必须是偶数
        int n_ivps_width;
        int n_ivps_height;

        int b_ivps_mirror; // 镜像
        int b_ivps_flip;   // 翻转

        int b_letterbox; // 填充，一般用于ai检测

        int n_fifo_count; // [0]表示不输出，[1-4]表示队列的个数，大于[0]则可以在调用回调输出图像
    } pipeline_ivps_config_t;

    typedef struct
    {
#define MAX_VENC_CHN_COUNT 64
        int n_venc_chn;     // 少于64 并且不能重复
        char end_point[32]; // rtsp的节点名称 例如 rtsp://x.x.x.x:554/end_point
    } pipeline_venc_config_t;

    typedef struct
    {
#define MAX_VDEC_GRP_COUNT 16
        int n_vdec_grp; // 少于 16，允许重复
        int poolid;     // internal variable,dont touch，内部使用，不要有任何操作
    } pipeline_vdec_config_t;

    typedef struct
    {
        int pipeid;                      // pipeline 的 id
        pipeline_output_e m_output_type; // 输出的类型
        // 图像或者buffer的一些参数
        int n_width, n_height, n_size, n_stride;
        int d_type; // AX_NPU_CV_FrameDataType
        void *p_vir;
        unsigned long long int p_phy;

        void *p_pipe; // pipeline_t 结构体指针
    } pipeline_buffer_t;

    // 回调函数，当 pipeline_ivps_config_t::n_fifo_count 大于0时候，用作输出给用户
    typedef void (*pipeline_frame_callback_func)(pipeline_buffer_t *buf);

    typedef struct
    {
        int enable;                      // 是否启用
        int pipeid;                      // pipeline的id，重复创建会失败
        pipeline_input_e m_input_type;   // 输入类型，暂时只支持pi_vin
        pipeline_output_e m_output_type; // 输出类型，暂时不支持po_venc_mjpg，po_vo_sipeed_maix3_screen只能创建一次

        // 可以用来控制线程退出（如果有的话）
        volatile int n_loog_exit;

// for input
#define MAX_VIN_PIPE_COUNT 4
        int n_vin_pipe; // lower than 4
#define MAX_VIN_CHN_COUNT 3
        int n_vin_chn; // lower than 3

        pipeline_vdec_config_t m_vdec_attr;

        pipeline_ivps_config_t m_ivps_attr;

        pipeline_venc_config_t m_venc_attr;

        pipeline_frame_callback_func output_func;

    } pipeline_t;
    
    int create_pipeline(pipeline_t *pipe);
    int destory_pipeline(pipeline_t *pipe);
    // 这里认为 pipe 指针含有 pipe_cnt 个 pipeline_t 结构体，并且每一个 pipeline_t 的输入类型 pipeline_input_e 是一样的，此函数会将同一张图片发送到所有 pipe_cnt 条链路中
    int user_input(pipeline_t *pipe, int pipe_cnt, pipeline_buffer_t *buf);
#if __cplusplus
}
#endif
#endif // _PIPELINE_COMMON_H_