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

#include "../libaxdl/include/c_api.h"
#include "../libaxdl/include/ax_osd_helper.hpp"
#include "../common/common_func.h"
#include "common_pipeline.h"

#include "../utilities/sample_log.h"

#include "ax_ivps_api.h"

#include "fstream"
#include <getopt.h>
#include "unistd.h"
#include "stdlib.h"
#include "string.h"
#include "signal.h"
#include "vector"
#include "map"

#include "V4l2Capture.h"
#include "libyuv.h"

#define pipe_count 2

AX_S32 s_sample_framerate = 25;
int SAMPLE_MAJOR_STREAM_WIDTH = 1920;
int SAMPLE_MAJOR_STREAM_HEIGHT = 1080;

int SAMPLE_IVPS_ALGO_WIDTH = 960;
int SAMPLE_IVPS_ALGO_HEIGHT = 540;
volatile AX_S32 gLoopExit;

static struct _g_sample_
{
    int bRunJoint;
    void *gModels;
    AX_S32 g_isp_force_loop_exit;
    ax_osd_helper osd_helper;
    std::vector<pipeline_t *> pipes_need_osd;
    void Init()
    {
        g_isp_force_loop_exit = 0;
        bRunJoint = 0;
        gModels = nullptr;
        ALOGN("g_sample Init\n");
    }
    void Deinit()
    {
        pipes_need_osd.clear();
        
        ALOGN("g_sample Deinit\n");
    }
} g_sample;


void ai_inference_func(pipeline_buffer_t *buff)
{
    if (g_sample.bRunJoint)
    {
        static axdl_results_t mResults;
        axdl_image_t tSrcFrame = {0};
        switch (buff->d_type)
        {
        case po_buff_nv12:
            tSrcFrame.eDtype = axdl_color_space_nv12;
            break;
        case po_buff_bgr:
            tSrcFrame.eDtype = axdl_color_space_bgr;
            break;
        case po_buff_rgb:
            tSrcFrame.eDtype = axdl_color_space_rgb;
            break;
        default:
            break;
        }
        tSrcFrame.nWidth = buff->n_width;
        tSrcFrame.nHeight = buff->n_height;
        tSrcFrame.pVir = (unsigned char *)buff->p_vir;
        tSrcFrame.pPhy = buff->p_phy;
        tSrcFrame.tStride_W = buff->n_stride;
        tSrcFrame.nSize = buff->n_size;

        axdl_inference(g_sample.gModels, &tSrcFrame, &mResults);

g_sample.osd_helper.Update(&mResults);
    }
}

// 允许外部调用
extern "C" AX_VOID __sigExit(int iSigNo)
{
    // ALOGN("Catch signal %d!\n", iSigNo);
    gLoopExit = 1;
    return;
}

static AX_VOID PrintHelp(char *testApp)
{
    printf("Usage:%s -h for help\n\n", testApp);
    printf("\t-p: model config file path\n");

    printf("\t-r: Sensor&Video Framerate (framerate need supported by sensor), default is 25\n");

    exit(0);
}

int main(int argc, char *argv[])
{
    optind = 0;
    gLoopExit = 0;
    g_sample.Init();

    AX_S32 isExit = 0, ch;
    AX_S32 s32Ret = 0;
    COMMON_SYS_ARGS_T tCommonArgs = {0};
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, __sigExit);
    char config_file[256];

    ALOGN("sample begin\n\n");

    while ((ch = getopt(argc, argv, "p:r:h")) != -1)
    {
        switch (ch)
        {
        case 'p':
        {
            strcpy(config_file, optarg);
            break;
        }
        case 'r':
            s_sample_framerate = (AX_S32)atoi(optarg);
            if (s_sample_framerate <= 0)
            {
                s_sample_framerate = 30;
            }
            break;
        case 'h':
            isExit = 1;
            break;
        default:
            isExit = 1;
            break;
        }
    }

    if (isExit)
    {
        PrintHelp(argv[0]);
        exit(0);
    }

#ifdef AXERA_TARGET_CHIP_AX620
    COMMON_SYS_POOL_CFG_T poolcfg[] = {
        {1920, 1088, 1920, AX_YUV420_SEMIPLANAR, 10},
    };
#elif defined(AXERA_TARGET_CHIP_AX650)
    COMMON_SYS_POOL_CFG_T poolcfg[] = {
        {1920, 1088, 1920, AX_FORMAT_YUV420_SEMIPLANAR, 20},
    };
#endif
    tCommonArgs.nPoolCfgCnt = 1;
    tCommonArgs.pPoolCfg = poolcfg;
    /*step 1:sys init*/
    s32Ret = COMMON_SYS_Init(&tCommonArgs);
    if (s32Ret)
    {
        ALOGE("COMMON_SYS_Init failed,s32Ret:0x%x\n", s32Ret);
        return -1;
    }

    /*step 2:npu init*/
#ifdef AXERA_TARGET_CHIP_AX620
    AX_NPU_SDK_EX_ATTR_T sNpuAttr;
    sNpuAttr.eHardMode = AX_NPU_VIRTUAL_1_1;
    s32Ret = AX_NPU_SDK_EX_Init_with_attr(&sNpuAttr);
    if (0 != s32Ret)
    {
        ALOGE("AX_NPU_SDK_EX_Init_with_attr failed,s32Ret:0x%x\n", s32Ret);
        goto EXIT_2;
    }
#endif

    s32Ret = axdl_parse_param_init(config_file, &g_sample.gModels);
    if (s32Ret != 0)
    {
        ALOGE("sample_parse_param_det failed");
        g_sample.bRunJoint = 0;
    }
    else
    {
        s32Ret = axdl_get_ivps_width_height(g_sample.gModels, config_file, &SAMPLE_IVPS_ALGO_WIDTH, &SAMPLE_IVPS_ALGO_HEIGHT);
        ALOGI("IVPS AI channel width=%d height=%d", SAMPLE_IVPS_ALGO_WIDTH, SAMPLE_IVPS_ALGO_HEIGHT);
        g_sample.bRunJoint = 1;
    }

    pipeline_t pipelines[pipe_count];
    memset(&pipelines[0], 0, sizeof(pipelines));
    // 创建pipeline
    {

        pipeline_t &pipe0 = pipelines[0];
        {
            pipeline_ivps_config_t &config0 = pipe0.m_ivps_attr;
            config0.n_ivps_grp = 0;    // 重复的会创建失败
            config0.n_ivps_fps = 60;   // 屏幕只能是60gps
            config0.n_ivps_rotate = 1; // 旋转
            config0.n_ivps_width = 854;
            config0.n_ivps_height = 480;
            config0.n_osd_rgn = 4; // osd rgn 的个数，一个rgn可以osd 32个目标
        }
        pipe0.enable = 1;
        pipe0.pipeid = 0x90015;
        pipe0.m_input_type = pi_user;
        pipe0.m_output_type = po_vo_sipeed_maix3_screen;
        pipe0.n_loog_exit = 0;            // 可以用来控制线程退出（如果有的话）
        pipe0.m_vdec_attr.n_vdec_grp = 0; // 可以重复

        pipeline_t &pipe1 = pipelines[1];
        {
            pipeline_ivps_config_t &config1 = pipe1.m_ivps_attr;
            config1.n_ivps_grp = 1; // 重复的会创建失败
            config1.n_ivps_fps = 60;
            config1.n_ivps_width = SAMPLE_IVPS_ALGO_WIDTH;
            config1.n_ivps_height = SAMPLE_IVPS_ALGO_HEIGHT;
            if (axdl_get_model_type(g_sample.gModels) != MT_SEG_PPHUMSEG)
            {
                config1.b_letterbox = 1;
            }
            config1.n_fifo_count = 1; // 如果想要拿到数据并输出到回调 就设为1~4
        }
        pipe1.enable = g_sample.bRunJoint;
        pipe1.pipeid = 0x90016;
        pipe1.m_input_type = pi_user;
        if (g_sample.gModels && g_sample.bRunJoint)
        {
            switch (axdl_get_color_space(g_sample.gModels))
            {
            case axdl_color_space_rgb:
                pipe1.m_output_type = po_buff_rgb;
                break;
            case axdl_color_space_bgr:
                pipe1.m_output_type = po_buff_bgr;
                break;
            case axdl_color_space_nv12:
            default:
                pipe1.m_output_type = po_buff_nv12;
                break;
            }
        }
        else
        {
            pipe1.enable = 0;
        }
        pipe1.n_loog_exit = 0;
        pipe1.m_vdec_attr.n_vdec_grp = 0;
        pipe1.output_func = ai_inference_func; // 图像输出的回调函数

        for (size_t i = 0; i < pipe_count; i++)
        {
            create_pipeline(&pipelines[i]);
            if (pipelines[i].m_ivps_attr.n_osd_rgn > 0)
            {
                g_sample.pipes_need_osd.push_back(&pipelines[i]);
            }
        }

        if (g_sample.pipes_need_osd.size() && g_sample.bRunJoint)
        {
            g_sample.osd_helper.Start(g_sample.gModels,g_sample.pipes_need_osd);
        }
    }

    {
        const int v4l2_width_max = 1280, v4l2_height_max = 720;

        int sSize = v4l2_width_max * v4l2_height_max * 3;
        std::vector<unsigned char> cbuffer(sSize), nv12buffer(v4l2_width_max * v4l2_height_max * 3 / 2);
        pipeline_buffer_t buf_mjpg = {0};
        buf_mjpg.p_vir = cbuffer.data();
        pipeline_buffer_t buf_nv12 = {0};
        buf_nv12.p_vir = nv12buffer.data();

        V4L2DeviceParameters param("/dev/video0", V4L2_PIX_FMT_MJPEG, v4l2_width_max, v4l2_height_max, 30, IOTYPE_MMAP, 0);
        V4l2Capture *videoCapture = V4l2Capture::create(param);
        buf_mjpg.n_width = videoCapture->getWidth();
        buf_mjpg.n_height = videoCapture->getHeight();
        printf("v4l2 video width:%d height:%d\r\n", buf_mjpg.n_width, buf_mjpg.n_height);

        // AX_U32 sReadLen = 0;
        timeval timeout = {0};
        timeout.tv_usec = 200;
        // for (int i = 0; i < 60; i++)
        while (!gLoopExit)
        {
            if (videoCapture->isReadable(&timeout))
            {
                buf_mjpg.n_size = videoCapture->read((char *)buf_mjpg.p_vir, sSize);
                buf_mjpg.p_vir = cbuffer.data();

                // cap_set();
                auto ret = libyuv::MJPGToNV12((uint8_t *)buf_mjpg.p_vir,
                                              buf_mjpg.n_size,
                                              nv12buffer.data(),
                                              buf_mjpg.n_width,
                                              nv12buffer.data() + buf_mjpg.n_width * buf_mjpg.n_height,
                                              buf_mjpg.n_width,
                                              buf_mjpg.n_width,
                                              buf_mjpg.n_height,
                                              buf_mjpg.n_width,
                                              buf_mjpg.n_height);
                if (0 == ret)
                {
                    buf_nv12.p_vir = nv12buffer.data();
                    buf_nv12.n_width = buf_mjpg.n_width;
                    buf_nv12.n_height = buf_mjpg.n_height;
                    buf_nv12.n_size = buf_mjpg.n_width * buf_mjpg.n_height * 3 / 2;
                    user_input(&pipelines[0], pipe_count, &buf_nv12);
                }
                // cap_get("libyuv::MJPGToNV12");
            }
            else
            {
                // ALOGN("read fail");
                usleep(10 * 1000);
            }
        }
        pipeline_buffer_t end_buf = {0};
        user_input(&pipelines[0], pipe_count, &end_buf);
        delete videoCapture;
    }

    // 销毁pipeline
    {
        gLoopExit = 1;
        if (g_sample.pipes_need_osd.size() && g_sample.bRunJoint)
        {
            g_sample.osd_helper.Stop();
        }

        for (size_t i = 0; i < pipe_count; i++)
        {
            destory_pipeline(&pipelines[i]);
        }
    }

EXIT_6:

    axdl_deinit(&g_sample.gModels);

EXIT_2:

    COMMON_SYS_DeInit();

    g_sample.Deinit();

    ALOGN("sample end\n");
    return 0;
}