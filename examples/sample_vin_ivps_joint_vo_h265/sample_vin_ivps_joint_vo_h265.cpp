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

#include "../common/common_func.h"
#include "../common/common_pipeline.h"

#include "../utilities/sample_log.h"

#include "ax_ivps_api.h"
#include "npu_cv_kit/ax_npu_imgproc.h"

#include <getopt.h>
#include "unistd.h"
#include "stdlib.h"
#include "string.h"
#include "signal.h"
#include "vector"
#include "map"

#define pipe_count 3

AX_S32 s_sample_framerate = 25;

volatile AX_S32 gLoopExit = 0;

int SAMPLE_MAJOR_STREAM_WIDTH;
int SAMPLE_MAJOR_STREAM_HEIGHT;

int SAMPLE_IVPS_ALGO_WIDTH = 960;
int SAMPLE_IVPS_ALGO_HEIGHT = 540;

static struct _g_sample_
{
    int bRunJoint;
    void *gModels;
    CAMERA_T gCams[MAX_CAMERAS];
    AX_S32 g_isp_force_loop_exit;
    pthread_mutex_t g_result_mutex;
    axdl_results_t g_result_disp;
    pthread_t osd_tid;
    std::vector<pipeline_t *> pipes_need_osd;
    void Init()
    {
        memset(gCams, 0, sizeof(gCams));
        g_isp_force_loop_exit = 0;
        pthread_mutex_init(&g_result_mutex, NULL);
        memset(&g_result_disp, 0, sizeof(axdl_results_t));
        bRunJoint = 0;
        gModels = nullptr;
        ALOGN("g_sample Init\n");
    }
    void Deinit()
    {
        pipes_need_osd.clear();
        pthread_mutex_destroy(&g_result_mutex);
        ALOGN("g_sample Deinit\n");
    }
} g_sample;

void *osd_thread(void *)
{
    std::map<int, axdl_canvas_t> pipes_osd_canvas;
    std::map<int, AX_IVPS_RGN_DISP_GROUP_S> pipes_osd_struct;
    for (size_t i = 0; i < g_sample.pipes_need_osd.size(); i++)
    {
        pipes_osd_canvas[g_sample.pipes_need_osd[i]->pipeid];
        pipes_osd_struct[g_sample.pipes_need_osd[i]->pipeid];
        auto &canvas = pipes_osd_canvas[g_sample.pipes_need_osd[i]->pipeid];
        auto &tDisp = pipes_osd_struct[g_sample.pipes_need_osd[i]->pipeid];
        memset(&tDisp, 0, sizeof(AX_IVPS_RGN_DISP_GROUP_S));
        canvas.channel = 4;
        canvas.data = (unsigned char *)malloc(g_sample.pipes_need_osd[i]->m_ivps_attr.n_ivps_width * g_sample.pipes_need_osd[i]->m_ivps_attr.n_ivps_height * 4);
        canvas.width = g_sample.pipes_need_osd[i]->m_ivps_attr.n_ivps_width;
        canvas.height = g_sample.pipes_need_osd[i]->m_ivps_attr.n_ivps_height;
    }

    axdl_results_t mResults;
    while (!gLoopExit)
    {
        pthread_mutex_lock(&g_sample.g_result_mutex);
        memcpy(&mResults, &g_sample.g_result_disp, sizeof(axdl_results_t));
        pthread_mutex_unlock(&g_sample.g_result_mutex);
        for (size_t i = 0; i < g_sample.pipes_need_osd.size(); i++)
        {
            auto &osd_pipe = g_sample.pipes_need_osd[i];
            if (osd_pipe && osd_pipe->m_ivps_attr.n_osd_rgn)
            {
                axdl_canvas_t &img_overlay = pipes_osd_canvas[osd_pipe->pipeid];
                AX_IVPS_RGN_DISP_GROUP_S &tDisp = pipes_osd_struct[osd_pipe->pipeid];

                memset(img_overlay.data, 0, img_overlay.width * img_overlay.height * img_overlay.channel);

                axdl_draw_results(g_sample.gModels, &img_overlay, &mResults, 0.6, 1.0, 0, 0);

                tDisp.nNum = 1;
                tDisp.tChnAttr.nAlpha = 1024;
                tDisp.tChnAttr.eFormat = AX_FORMAT_RGBA8888;
                tDisp.tChnAttr.nZindex = 1;
                tDisp.tChnAttr.nBitColor.nColor = 0xFF0000;
                tDisp.tChnAttr.nBitColor.bEnable = AX_FALSE;
                tDisp.tChnAttr.nBitColor.nColorInv = 0xFF;
                tDisp.tChnAttr.nBitColor.nColorInvThr = 0xA0A0A0;

                tDisp.arrDisp[0].bShow = AX_TRUE;
                tDisp.arrDisp[0].eType = AX_IVPS_RGN_TYPE_OSD;

                tDisp.arrDisp[0].uDisp.tOSD.bEnable = AX_TRUE;
                tDisp.arrDisp[0].uDisp.tOSD.enRgbFormat = AX_FORMAT_RGBA8888;
                tDisp.arrDisp[0].uDisp.tOSD.u32Zindex = 1;
                tDisp.arrDisp[0].uDisp.tOSD.u32ColorKey = 0x0;
                tDisp.arrDisp[0].uDisp.tOSD.u32BgColorLo = 0xFFFFFFFF;
                tDisp.arrDisp[0].uDisp.tOSD.u32BgColorHi = 0xFFFFFFFF;
                tDisp.arrDisp[0].uDisp.tOSD.u32BmpWidth = img_overlay.width;
                tDisp.arrDisp[0].uDisp.tOSD.u32BmpHeight = img_overlay.height;
                tDisp.arrDisp[0].uDisp.tOSD.u32DstXoffset = 0;
                tDisp.arrDisp[0].uDisp.tOSD.u32DstYoffset = osd_pipe->m_output_type == po_vo_sipeed_maix3_screen ? 32 : 0;
                tDisp.arrDisp[0].uDisp.tOSD.u64PhyAddr = 0;
                tDisp.arrDisp[0].uDisp.tOSD.pBitmap = img_overlay.data;

                int ret = AX_IVPS_RGN_Update(osd_pipe->m_ivps_attr.n_osd_rgn_chn[0], &tDisp);
                if (0 != ret)
                {
                    static int cnt = 0;
                    if (cnt++ % 100 == 0)
                    {
                        ALOGE("AX_IVPS_RGN_Update fail, ret=0x%x, hChnRgn=%d", ret, osd_pipe->m_ivps_attr.n_osd_rgn_chn[0]);
                    }
                    usleep(30 * 1000);
                }
            }
        }
        // freeObjs(&mResults);
        usleep(0);
    }
    for (size_t i = 0; i < g_sample.pipes_need_osd.size(); i++)
    {
        auto &canvas = pipes_osd_canvas[g_sample.pipes_need_osd[i]->pipeid];
        free(canvas.data);
    }

    return NULL;
}

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
        pthread_mutex_lock(&g_sample.g_result_mutex);
        memcpy(&g_sample.g_result_disp, &mResults, sizeof(axdl_results_t));
        pthread_mutex_unlock(&g_sample.g_result_mutex);
    }
}
FILE *h265_file_output = NULL;
void h265_save_func(pipeline_buffer_t *buff)
{
    if (!h265_file_output)
    {
        // 获取系统时间戳
        time_t timeReal;
        time(&timeReal);
        timeReal = timeReal + 8 * 3600;
        tm *t = gmtime(&timeReal);
        char filename[128];
        sprintf(filename, "%d-%02d-%02d_%02d-%02d-%02d.h265", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);

        h265_file_output = fopen(filename, "wb");
        if (h265_file_output)
        {
            ALOGI("start record to %s", filename);
        }
        else
        {
            ALOGE("%s open failed", filename);
        }
    }
    if (h265_file_output)
    {
        fwrite(buff->p_vir, 1, buff->n_size, h265_file_output);
        static int cnt = 0;
        if (cnt++ % 100 == 0)
        {
            ALOGI("recoding");
        }
    }
}

static void *IspRun(void *args)
{
    AX_U32 i = (AX_U32)args;

    ALOGN("cam %d is running...\n", i);

    while (!g_sample.g_isp_force_loop_exit)
    {
        if (!g_sample.gCams[i].bOpen)
        {
            usleep(40 * 1000);
            continue;
        }

        AX_ISP_Run(g_sample.gCams[i].nPipeId);
    }
    return NULL;
}

static AX_S32 SysRun()
{
    AX_S32 s32Ret = 0, i;

    g_sample.g_isp_force_loop_exit = 0;
    for (i = 0; i < MAX_CAMERAS; i++)
    {
        if (g_sample.gCams[i].bOpen)
            pthread_create(&g_sample.gCams[i].tIspProcThread, NULL, IspRun, (AX_VOID *)i);
    }

    while (!gLoopExit)
    {
        sleep(1);
    }

    g_sample.g_isp_force_loop_exit = 1;

    for (i = 0; i < MAX_CAMERAS; i++)
    {
        if (g_sample.gCams[i].bOpen)
        {
            pthread_cancel(g_sample.gCams[i].tIspProcThread);
            s32Ret = pthread_join(g_sample.gCams[i].tIspProcThread, NULL);
            if (s32Ret < 0)
            {
                ALOGE(" isp run thread exit failed,s32Ret:0x%x\n", s32Ret);
            }
        }
    }
    return 0;
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

    printf("\t-c: ISP Test Case:\n");
    printf("\t\t0: Single OS04A10\n");
    printf("\t\t1: Single IMX334\n");
    printf("\t\t2: Single GC4653\n");
    printf("\t\t3: Single OS08A20\n");
    printf("\t\t4: Single OS04A10 Online\n");

    printf("\t-e: SDR/HDR Mode:\n");
    printf("\t\t1: SDR\n");
    printf("\t\t2: HDR 2DOL\n");

    printf("\t-r: Sensor&Video Framerate (framerate need supported by sensor), default is 25\n");

    exit(0);
}

int main(int argc, char *argv[])
{
    optind = 0;
    gLoopExit = 0;
    g_sample.Init();

    AX_S32 isExit = 0, i, ch;
    AX_S32 s32Ret = 0;
    COMMON_SYS_CASE_E eSysCase = SYS_CASE_SINGLE_GC4653;
    COMMON_SYS_ARGS_T tCommonArgs = {0};
    AX_SNS_HDR_MODE_E eHdrMode = AX_SNS_LINEAR_MODE;
    SAMPLE_SNS_TYPE_E eSnsType = GALAXYCORE_GC4653;
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, __sigExit);
    char config_file[256];

    ALOGN("sample begin\n\n");

    while ((ch = getopt(argc, argv, "p:c:e:r:h")) != -1)
    {
        switch (ch)
        {
        case 'p':
        {
            strcpy(config_file, optarg);
            break;
        }
        case 'c':
            eSysCase = (COMMON_SYS_CASE_E)atoi(optarg);
            break;
        case 'e':
            eHdrMode = (AX_SNS_HDR_MODE_E)atoi(optarg);
            break;
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

    ALOGN("eSysCase=%d,eHdrMode=%d\n", eSysCase, eHdrMode);

    s32Ret = COMMON_SET_CAM(g_sample.gCams, eSysCase, eHdrMode, &eSnsType, &tCommonArgs, s_sample_framerate);
    if (s32Ret != 0)
    {
        PrintHelp(argv[0]);
        exit(0);
    }

    SAMPLE_MAJOR_STREAM_WIDTH = g_sample.gCams[0].stChnAttr.tChnAttr[AX_YUV_SOURCE_ID_MAIN].nWidth;
    SAMPLE_MAJOR_STREAM_HEIGHT = g_sample.gCams[0].stChnAttr.tChnAttr[AX_YUV_SOURCE_ID_MAIN].nHeight;

    /*step 1:sys init*/
    s32Ret = COMMON_SYS_Init(&tCommonArgs);
    if (s32Ret)
    {
        ALOGE("COMMON_SYS_Init failed,s32Ret:0x%x\n", s32Ret);
        return -1;
    }

    /*step 3:npu init*/
    AX_NPU_SDK_EX_ATTR_T sNpuAttr;
    sNpuAttr.eHardMode = AX_NPU_VIRTUAL_1_1;
    s32Ret = AX_NPU_SDK_EX_Init_with_attr(&sNpuAttr);
    if (0 != s32Ret)
    {
        ALOGE("AX_NPU_SDK_EX_Init_with_attr failed,s32Ret:0x%x\n", s32Ret);
        goto EXIT_2;
    }

    s32Ret = axdl_parse_param_init(config_file, &g_sample.gModels);
    if (s32Ret != 0)
    {
        ALOGE("sample_parse_param_det failed,run joint skip");
        g_sample.bRunJoint = 0;
    }
    else
    {
        s32Ret = axdl_get_ivps_width_height(g_sample.gModels, config_file, &SAMPLE_IVPS_ALGO_WIDTH, &SAMPLE_IVPS_ALGO_HEIGHT);
        ALOGI("IVPS AI channel width=%d heighr=%d", SAMPLE_IVPS_ALGO_WIDTH, SAMPLE_IVPS_ALGO_HEIGHT);
        g_sample.bRunJoint = 1;
    }

    /*step 3:camera init*/
    s32Ret = COMMON_CAM_Init();
    if (0 != s32Ret)
    {
        ALOGE("COMMON_CAM_Init failed,s32Ret:0x%x\n", s32Ret);
        goto EXIT_2;
    }

    for (i = 0; i < tCommonArgs.nCamCnt; i++)
    {
        s32Ret = COMMON_CAM_Open(&g_sample.gCams[i]);
        if (s32Ret)
        {
            ALOGE("COMMON_CAM_Open failed,s32Ret:0x%x\n", s32Ret);
            goto EXIT_3;
        }
        g_sample.gCams[i].bOpen = AX_TRUE;
        ALOGN("camera %d is open\n", i);
    }

    pipeline_t pipelines[pipe_count];
    memset(&pipelines[0], 0, sizeof(pipelines));
    // 创建pipeline
    {
        int pipeidx = 0;
        pipeline_t &pipe0 = pipelines[pipeidx];
        {
            pipeline_ivps_config_t &config = pipe0.m_ivps_attr;
            config.n_ivps_grp = pipeidx; // 重复的会创建失败
            config.n_ivps_fps = 60;      // 屏幕只能是60gps
            config.n_ivps_rotate = 1;    // 旋转
            config.n_ivps_width = 854;
            config.n_ivps_height = 480;
            config.n_osd_rgn = 1; // osd rgn 的个数，一个rgn可以osd 32个目标
        }
        pipe0.enable = 1;
        pipe0.pipeid = pipeidx++;
        pipe0.m_input_type = pi_vin;
        pipe0.m_output_type = po_vo_sipeed_maix3_screen;
        pipe0.n_loog_exit = 0; // 可以用来控制线程退出（如果有的话）
        pipe0.n_vin_pipe = 0;
        pipe0.n_vin_chn = 0;

        pipeline_t &pipe1 = pipelines[pipeidx];
        {
            pipeline_ivps_config_t &config = pipe1.m_ivps_attr;
            config.n_ivps_grp = pipeidx; // 重复的会创建失败
            config.n_ivps_fps = 60;
            config.n_ivps_width = SAMPLE_IVPS_ALGO_WIDTH;
            config.n_ivps_height = SAMPLE_IVPS_ALGO_HEIGHT;
            if (axdl_get_model_type(g_sample.gModels) != MT_SEG_PPHUMSEG)
            {
                config.b_letterbox = 1;
            }
            config.n_fifo_count = 1; // 如果想要拿到数据并输出到回调 就设为1~4
        }
        pipe1.enable = g_sample.bRunJoint;
        pipe1.pipeid = pipeidx++;
        pipe1.m_input_type = pi_vin;
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
        pipe1.n_vin_pipe = 0;
        pipe1.n_vin_chn = 0;
        pipe1.output_func = ai_inference_func; // 图像输出的回调函数

        pipeline_t &pipe2 = pipelines[pipeidx];
        {
            pipeline_ivps_config_t &config = pipe2.m_ivps_attr;
            config.n_ivps_grp = pipeidx; // 重复的会创建失败
            config.n_ivps_fps = 25;
            config.n_ivps_width = 960;
            config.n_ivps_height = 540;
            config.n_osd_rgn = 1;
            config.n_fifo_count = 1; // 如果想要拿到数据并输出到回调 就设为1~4
        }
        pipe2.enable = 1;
        pipe2.pipeid = pipeidx++;
        pipe2.m_input_type = pi_vin;
        pipe2.m_output_type = po_venc_h265;
        pipe2.n_loog_exit = 0;
        pipe2.n_vin_pipe = 0;
        pipe2.n_vin_chn = 0;
        pipe2.m_venc_attr.n_venc_chn = 0;
        pipe2.output_func = h265_save_func; // 图像输出的回调函数

        for (size_t i = 0; i < pipe_count; i++)
        {
            ALOGN("create pipe-%d", pipelines[i].pipeid);
            create_pipeline(&pipelines[i]);
            if (pipelines[i].m_ivps_attr.n_osd_rgn > 0)
            {
                g_sample.pipes_need_osd.push_back(&pipelines[i]);
            }
        }

        if (g_sample.pipes_need_osd.size() && g_sample.bRunJoint)
        {
            pthread_create(&g_sample.osd_tid, NULL, osd_thread, NULL);
        }
    }

    s32Ret = SysRun();
    if (0 != s32Ret)
    {
        ALOGE("SysRun error,s32Ret:0x%x\n", s32Ret);
        goto EXIT_6;
    }

    // 销毁pipeline
    {
        gLoopExit = 1;
        if (g_sample.pipes_need_osd.size() && g_sample.bRunJoint)
        {
            //            pthread_cancel(g_sample.osd_tid);
            s32Ret = pthread_join(g_sample.osd_tid, NULL);
            if (s32Ret < 0)
            {
                ALOGE(" osd_tid exit failed,s32Ret:0x%x\n", s32Ret);
            }
        }

        for (size_t i = 0; i < pipe_count; i++)
        {
            destory_pipeline(&pipelines[i]);
        }
        if (h265_file_output)
        {
            ALOGI("stop recode");
            fclose(h265_file_output);
            h265_file_output = NULL;
        }
    }

EXIT_6:

    for (i = 0; i < tCommonArgs.nCamCnt; i++)
    {
        if (!g_sample.gCams[i].bOpen)
            continue;
        COMMON_CAM_Close(&g_sample.gCams[i]);
    }

EXIT_3:
    COMMON_CAM_Deinit();
    axdl_deinit(&g_sample.gModels);

EXIT_2:

    COMMON_SYS_DeInit();
    g_sample.Deinit();

    ALOGN("sample end\n");
    return 0;
}
