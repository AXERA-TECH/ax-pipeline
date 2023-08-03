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

#include "../common/video_demux.hpp"

#include "ax_ivps_api.h"

#include "fstream"
#include <getopt.h>
#include "unistd.h"
#include "stdlib.h"
#include "string.h"
#include "signal.h"
#include "vector"
#include "map"

#include "opencv2/opencv.hpp"

#define pipe_count 2

#ifdef AXERA_TARGET_CHIP_AX620
#define rtsp_max_count 2
#elif defined(AXERA_TARGET_CHIP_AX650)
#define rtsp_max_count 4
#endif

AX_S32 s_sample_framerate = 25;

volatile AX_S32 gLoopExit = 0;

int SAMPLE_MAJOR_STREAM_WIDTH = 1920;
int SAMPLE_MAJOR_STREAM_HEIGHT = 1080;

int SAMPLE_IVPS_ALGO_WIDTH[rtsp_max_count] = {960};
int SAMPLE_IVPS_ALGO_HEIGHT[rtsp_max_count] = {540};

static struct _g_sample_
{
    struct _model
    {
        int bRunJoint;
        void *gModel;
        ax_osd_helper osd_helper;
        std::vector<pipeline_t *> pipes_need_osd;
    } gModels[rtsp_max_count];

    // int bRunJoint[4];
    // void *gModels[rtsp_max_count];
    // std::map<int, ax_osd_helper> osd_helpers;
    // std::vector<pipeline_t *> pipes_need_osd[rtsp_max_count];
    std::map<int, _model *> osd_target_map;
    // std::map<int, int> osd_target;
    void Init()
    {
        for (size_t i = 0; i < rtsp_max_count; i++)
        {
            gModels[i].pipes_need_osd.clear();
            gModels[i].gModel = nullptr;
            gModels[i].bRunJoint = 0;
            // pthread_mutex_init(&g_result_mutexs[i], NULL);
        }
        osd_target_map.clear();
        // memset(&g_result_disps[0], 0, sizeof(g_result_disps));

        ALOGN("g_sample Init\n");
    }
    void Deinit()
    {

        for (size_t i = 0; i < rtsp_max_count; i++)
        {
            gModels[i].pipes_need_osd.clear();
            gModels[i].gModel = nullptr;
            gModels[i].bRunJoint = 0;
            // pthread_mutex_init(&g_result_mutexs[i], NULL);
        }

        ALOGN("g_sample Deinit\n");
    }
} g_sample;

void ai_inference_func(pipeline_buffer_t *buff)
{
    pipeline_t *pipe = (pipeline_t *)buff->p_pipe;
    if (g_sample.osd_target_map[pipe->pipeid]->bRunJoint)
    {
        static std::map<int, axdl_results_t> mResults;
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

        axdl_inference(g_sample.osd_target_map[pipe->pipeid]->gModel, &tSrcFrame, &mResults[pipe->pipeid]);
        // ALOGI("pipe=%d detect%d", pipe->pipeid, mResults[pipe->pipeid].nObjSize);
        g_sample.osd_target_map[pipe->pipeid]->osd_helper.Update(&mResults[pipe->pipeid]);
    }
}

void _demux_frame_callback(const void *buff, int len, void *reserve)
{
    if (len == 0)
    {
        pipeline_buffer_t end_buf = {0};
        user_input((pipeline_t *)reserve, 1, &end_buf);
        ALOGN("mp4 file decode finish,quit the loop");
        gLoopExit = 1;
    }
    pipeline_buffer_t buf_h26x = {0};
    buf_h26x.p_vir = (void *)buff;
    buf_h26x.n_size = len;
    user_input((pipeline_t *)reserve, 1, &buf_h26x);
    usleep(5 * 1000);
}

// 允许外部调用
extern "C" AX_VOID __sigExit(int iSigNo)
{
    // ALOGN("Catch signal %d!\n", iSigNo);
    gLoopExit = 1;
    sleep(1);
    return;
}

static AX_VOID PrintHelp(char *testApp)
{
    printf("Usage:%s -h for help\n\n", testApp);
    printf("\t-p: model config file path\n");

    printf("\t-f: mp4 file/rtsp url(just only support h264 format)\n");

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
    char rtsp_url[512];
    std::vector<std::string> rtsp_urls;
    std::vector<std::vector<pipeline_t>> vpipelines;

    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, __sigExit);
    char config_file[256];

    ALOGN("sample begin\n\n");

    while ((ch = getopt(argc, argv, "p:f:r:h")) != -1)
    {
        switch (ch)
        {
        case 'f':
        {
            strcpy(rtsp_url, optarg);
            ALOGI("rtsp url : %s", rtsp_url);
            std::string tmp(rtsp_url);
            if (rtsp_urls.size() >= rtsp_max_count)
            {
                ALOGE("support only %d rtsp inputs", rtsp_max_count);
            }
            else
            {
                rtsp_urls.push_back(tmp);
            }
        }
        break;
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

    if (rtsp_urls.size() == 0)
    {
        ALOGE("video urls is empty");
        PrintHelp(argv[0]);
        exit(0);
    }

#ifdef AXERA_TARGET_CHIP_AX620
    COMMON_SYS_POOL_CFG_T poolcfg[] = {
        {1920, 1088, 1920, AX_YUV420_SEMIPLANAR, 20},
    };
#elif defined(AXERA_TARGET_CHIP_AX650)
    COMMON_SYS_POOL_CFG_T poolcfg[] = {
        {1920, 1088, 1920, AX_FORMAT_YUV420_SEMIPLANAR, uint32_t(rtsp_urls.size() * 15)},
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

    /*step 3:npu init*/
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

    vpipelines.resize(rtsp_urls.size());

    for (size_t i = 0; i < rtsp_urls.size(); i++)
    {

        s32Ret = axdl_parse_param_init(config_file, &g_sample.gModels[i].gModel);
        if (s32Ret != 0)
        {
            ALOGE("sample_parse_param_det failed,run joint skip");
            g_sample.gModels[i].bRunJoint = 0;
        }
        else
        {
            s32Ret = axdl_get_ivps_width_height(g_sample.gModels[i].gModel, config_file, &SAMPLE_IVPS_ALGO_WIDTH[i], &SAMPLE_IVPS_ALGO_HEIGHT[i]);
            ALOGI("IVPS AI channel width=%d height=%d", SAMPLE_IVPS_ALGO_WIDTH[i], SAMPLE_IVPS_ALGO_HEIGHT[i]);
            g_sample.gModels[i].bRunJoint = 1;
        }
        auto &pipelines = vpipelines[i];
        pipelines.resize(pipe_count);
        memset(pipelines.data(), 0, pipe_count * sizeof(pipeline_t));

        // 创建pipeline
        {
            pipeline_t &pipe1 = pipelines[1];
            {
                pipeline_ivps_config_t &config1 = pipe1.m_ivps_attr;
                config1.n_ivps_grp = pipe_count * i + 1; // 重复的会创建失败
                config1.n_ivps_fps = 60;
                config1.n_ivps_width = SAMPLE_IVPS_ALGO_WIDTH[i];
                config1.n_ivps_height = SAMPLE_IVPS_ALGO_HEIGHT[i];
                if (axdl_get_model_type(g_sample.gModels[i].gModel) != MT_SEG_PPHUMSEG && axdl_get_model_type(g_sample.gModels[i].gModel) != MT_SEG_DINOV2)
                {
                    config1.b_letterbox = 1;
                }
                config1.n_fifo_count = 1; // 如果想要拿到数据并输出到回调 就设为1~4
            }
            pipe1.enable = g_sample.gModels[i].bRunJoint;
            pipe1.pipeid = pipe_count * i + 1;
            pipe1.m_input_type = pi_vdec_h264;
            if (g_sample.gModels[i].gModel && g_sample.gModels[i].bRunJoint)
            {
                switch (axdl_get_color_space(g_sample.gModels[i].gModel))
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
            pipe1.m_vdec_attr.n_vdec_grp = i;
            pipe1.output_func = ai_inference_func; // 图像输出的回调函数

            pipeline_t &pipe2 = pipelines[0];
            {
                pipeline_ivps_config_t &config2 = pipe2.m_ivps_attr;
                config2.n_ivps_grp = pipe_count * i + 2; // 重复的会创建失败
                config2.n_ivps_rotate = 0;               // 旋转90度，现在rtsp流是竖着的画面了
                config2.n_ivps_fps = s_sample_framerate;
                config2.n_ivps_width = 1920;
                config2.n_ivps_height = 1080;
                config2.n_osd_rgn = 4;
            }
            pipe2.enable = 1;
            pipe2.pipeid = pipe_count * i + 2; // 重复的会创建失败
            pipe2.m_input_type = pi_vdec_h264;
            pipe2.m_output_type = po_rtsp_h264;
            pipe2.n_loog_exit = 0;

            sprintf(pipe2.m_venc_attr.end_point, "%s%d", "axstream", (int)i); // 重复的会创建失败
            pipe2.m_venc_attr.n_venc_chn = i;                            // 重复的会创建失败
            pipe2.m_vdec_attr.n_vdec_grp = i;
        }
    }

    for (size_t i = 0; i < vpipelines.size(); i++)
    {
        auto &pipelines = vpipelines[i];
        for (size_t j = 0; j < pipelines.size(); j++)
        {
            create_pipeline(&pipelines[j]);
            if (pipelines[j].m_ivps_attr.n_osd_rgn > 0)
            {
                g_sample.gModels[i].pipes_need_osd.push_back(&pipelines[j]);
            }
        }

        if (g_sample.gModels[i].pipes_need_osd.size() && g_sample.gModels[i].bRunJoint)
        {
            g_sample.gModels[i].osd_helper.Start(g_sample.gModels[i].gModel, g_sample.gModels[i].pipes_need_osd);
            // pthread_create(&g_sample.osd_tid[i], NULL, osd_funcs[i], NULL);
            // g_sample.osd_target_map[pipelines[1].pipeid] = g_sample.gModels[i].pipes_need_osd[0]->pipeid;
            g_sample.osd_target_map[pipelines[1].pipeid] = &g_sample.gModels[i];
        }
    }

    {
        std::vector<VideoDemux *> video_demuxes;
        for (size_t i = 0; i < rtsp_urls.size(); i++)
        {
            auto &pipelines = vpipelines[i];
            VideoDemux *demux = new VideoDemux();
            if (demux->Open(rtsp_urls[i].c_str(), true, _demux_frame_callback, pipelines.data()))
            {
                video_demuxes.push_back(demux);
            }
        }

        while (!gLoopExit)
        {
            usleep(1000 * 1000);
        }
        for (size_t i = 0; i < video_demuxes.size(); i++)
        {
            VideoDemux *demux = video_demuxes[i];
            demux->Stop();
            delete demux;
        }

        gLoopExit = 1;
        sleep(1);
        pipeline_buffer_t end_buf = {0};
        for (size_t i = 0; i < rtsp_urls.size(); i++)
        {
            auto &pipelines = vpipelines[i];
            user_input(pipelines.data(), 1, &end_buf);
        }
    }

    // 销毁pipeline
    {
        gLoopExit = 1;
        for (size_t i = 0; i < rtsp_urls.size(); i++)
        {
            if (g_sample.gModels[i].pipes_need_osd.size() && g_sample.gModels[i].bRunJoint)
            {
                g_sample.gModels[i].osd_helper.Stop();
                //            pthread_cancel(g_sample.osd_tid);
                // s32Ret = pthread_join(g_sample.osd_tid[i], NULL);
                // if (s32Ret < 0)
                // {
                //     ALOGE(" osd_tid exit failed,s32Ret:0x%x\n", s32Ret);
                // }
            }
        }

        for (size_t i = 0; i < vpipelines.size(); i++)
        {
            auto &pipelines = vpipelines[i];
            for (size_t j = 0; j < pipelines.size(); j++)
            {
                destory_pipeline(&pipelines[j]);
            }
        }
    }

EXIT_6:

EXIT_5:

EXIT_4:

EXIT_3:
    for (size_t i = 0; i < rtsp_urls.size(); i++)
    {
        axdl_deinit(&g_sample.gModels[i].gModel);
    }

EXIT_2:

EXIT_1:
    COMMON_SYS_DeInit();
    g_sample.Deinit();

    ALOGN("sample end\n");
    return 0;
}
