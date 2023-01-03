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

#include "../sample_run_joint/sample_run_joint_post_process.h"
#include "../common/common_joint.h"
#include "../common/common_func.h"
#include "../common/common_pipeline.h"
#include "../utilities/osd_utils.h"
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

CAMERA_T gCams[MAX_CAMERAS] = {0};

volatile AX_S32 gLoopExit = 0;
static AX_S32 g_isp_force_loop_exit = 0;

pthread_mutex_t g_result_mutex;
sample_run_joint_results g_result_disp;

int SAMPLE_MAJOR_STREAM_WIDTH;
int SAMPLE_MAJOR_STREAM_HEIGHT;

sample_run_joint_models gModels;

std::vector<pipeline_t *> pipes_need_osd;
std::map<int, osd_utils_img> pipes_osd_canvas;
std::map<int, AX_IVPS_RGN_DISP_GROUP_S> pipes_osd_struct;

void *osd_thread(void *)
{
    sample_run_joint_results mResults;
    while (!gLoopExit)
    {
        pthread_mutex_lock(&g_result_mutex);
        memcpy(&mResults, &g_result_disp, sizeof(sample_run_joint_results));
        pthread_mutex_unlock(&g_result_mutex);
        for (size_t i = 0; i < pipes_need_osd.size(); i++)
        {
            auto &osd_pipe = pipes_need_osd[i];
            if (osd_pipe && osd_pipe->m_ivps_attr.n_osd_rgn)
            {
                osd_utils_img &img_overlay = pipes_osd_canvas[osd_pipe->pipeid];
                AX_IVPS_RGN_DISP_GROUP_S &tDisp = pipes_osd_struct[osd_pipe->pipeid];

                memset(img_overlay.data, 0, img_overlay.width * img_overlay.height * img_overlay.channel);

                drawResults(&img_overlay, 0.6, 1.0, &mResults, 0, 0);

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
    return NULL;
}

void ai_inference_func(pipeline_buffer_t *buff)
{
    if (gModels.bRunJoint)
    {
        static sample_run_joint_results mResults;
        AX_NPU_CV_Image tSrcFrame = {0};

        tSrcFrame.eDtype = (AX_NPU_CV_FrameDataType)buff->d_type;
        tSrcFrame.nWidth = buff->n_width;
        tSrcFrame.nHeight = buff->n_height;
        tSrcFrame.pVir = (unsigned char *)buff->p_vir;
        tSrcFrame.pPhy = buff->p_phy;
        tSrcFrame.tStride.nW = buff->n_stride;
        tSrcFrame.nSize = buff->n_size;

        sample_run_joint_inference_single_func(&gModels, &tSrcFrame, &mResults);
        pthread_mutex_lock(&g_result_mutex);
        memcpy(&g_result_disp, &mResults, sizeof(sample_run_joint_results));
        pthread_mutex_unlock(&g_result_mutex);
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
            ALOGI("start recode to %s", filename);
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
            ALOGI("偷拍中");
        }
    }
}

static void *IspRun(void *args)
{
    AX_U32 i = (AX_U32)args;

    ALOGN("cam %d is running...\n", i);

    while (!g_isp_force_loop_exit)
    {
        if (!gCams[i].bOpen)
        {
            usleep(40 * 1000);
            continue;
        }

        AX_ISP_Run(gCams[i].nPipeId);
    }
    return NULL;
}

static AX_S32 SysRun()
{
    AX_S32 s32Ret = 0, i;

    g_isp_force_loop_exit = 0;
    for (i = 0; i < MAX_CAMERAS; i++)
    {
        if (gCams[i].bOpen)
            pthread_create(&gCams[i].tIspProcThread, NULL, IspRun, (AX_VOID *)i);
    }

    while (!gLoopExit)
    {
        sleep(1);
    }

    g_isp_force_loop_exit = 1;

    for (i = 0; i < MAX_CAMERAS; i++)
    {
        if (gCams[i].bOpen)
        {
            pthread_cancel(gCams[i].tIspProcThread);
            s32Ret = pthread_join(gCams[i].tIspProcThread, NULL);
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
    printf("\t-p: yolov5 param file path\n");
    printf("\t-m: Joint model path\n");

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
    g_isp_force_loop_exit = 0;
    memset(&gModels, 0, sizeof(gModels));
    memset(&g_result_disp, 0, sizeof(g_result_disp));
    memset(&gCams, 0, sizeof(gCams));

    gModels.bRunJoint = AX_FALSE;
    gModels.mMajor.JointHandle = NULL;
    gModels.mMinor.JointHandle = NULL;
    gModels.mMajor.ModelType = MT_UNKNOWN;
    gModels.mMinor.ModelType = MT_UNKNOWN;
    gModels.ModelType_Main = MT_UNKNOWN;
    gModels.SAMPLE_ALGO_FORMAT = AX_YUV420_SEMIPLANAR;
    gModels.SAMPLE_IVPS_ALGO_WIDTH = 960;
    gModels.SAMPLE_IVPS_ALGO_HEIGHT = 540;

    AX_S32 isExit = 0, i, ch;
    AX_S32 s32Ret = 0;
    COMMON_SYS_CASE_E eSysCase = SYS_CASE_SINGLE_GC4653;
    COMMON_SYS_ARGS_T tCommonArgs = {0};
    AX_SNS_HDR_MODE_E eHdrMode = AX_SNS_LINEAR_MODE;
    SAMPLE_SNS_TYPE_E eSnsType = GALAXYCORE_GC4653;
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, __sigExit);

    ALOGN("sample begin\n\n");

    while ((ch = getopt(argc, argv, "p:m:c:e:r:h")) != -1)
    {
        switch (ch)
        {
        case 'm':
            strcpy(gModels.MODEL_PATH, optarg);
            gModels.bRunJoint = AX_TRUE;
            break;
        case 'p':
        {
            int ret = sample_run_joint_parse_param(optarg, &gModels);
            if (ret != 0)
            {
                ALOGE("sample_parse_param_det failed");
                isExit = 1;
            }
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

    if (gModels.ModelType_Main == MT_UNKNOWN)
    {
        ALOGI("got MT_UNKNOWN");
        gModels.bRunJoint = AX_FALSE;
    }

    ALOGN("eSysCase=%d,eHdrMode=%d\n", eSysCase, eHdrMode);

    s32Ret = COMMON_SET_CAM(gCams, eSysCase, eHdrMode, &eSnsType, &tCommonArgs, s_sample_framerate);
    if (s32Ret != 0)
    {
        PrintHelp(argv[0]);
        exit(0);
    }

    SAMPLE_MAJOR_STREAM_WIDTH = gCams[0].stChnAttr.tChnAttr[AX_YUV_SOURCE_ID_MAIN].nWidth;
    SAMPLE_MAJOR_STREAM_HEIGHT = gCams[0].stChnAttr.tChnAttr[AX_YUV_SOURCE_ID_MAIN].nHeight;

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

    s32Ret = COMMON_JOINT_Init(&gModels, SAMPLE_MAJOR_STREAM_WIDTH, SAMPLE_MAJOR_STREAM_HEIGHT);
    if (0 != s32Ret)
    {
        ALOGE("COMMON_JOINT_Init failed,s32Ret:0x%x\n", s32Ret);
        goto EXIT_2;
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
        s32Ret = COMMON_CAM_Open(&gCams[i]);
        if (s32Ret)
        {
            ALOGE("COMMON_CAM_Open failed,s32Ret:0x%x\n", s32Ret);
            goto EXIT_3;
        }
        gCams[i].bOpen = AX_TRUE;
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
            config.n_ivps_width = gModels.SAMPLE_IVPS_ALGO_WIDTH;
            config.n_ivps_height = gModels.SAMPLE_IVPS_ALGO_HEIGHT;
            config.b_letterbox = 1;
            config.n_fifo_count = 1; // 如果想要拿到数据并输出到回调 就设为1~4
        }
        pipe1.enable = 1;
        pipe1.pipeid = pipeidx++;
        pipe1.m_input_type = pi_vin;
        switch (gModels.SAMPLE_ALGO_FORMAT)
        {
        case AX_FORMAT_RGB888:
            pipe1.m_output_type = po_buff_rgb;
            break;
        case AX_FORMAT_BGR888:
            pipe1.m_output_type = po_buff_bgr;
            break;
        case AX_YUV420_SEMIPLANAR:
        default:
            pipe1.m_output_type = po_buff_nv12;
            break;
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
            ALOGN("create pipe-%d",pipelines[i].pipeid);
            create_pipeline(&pipelines[i]);
            if (pipelines[i].m_ivps_attr.n_osd_rgn > 0)
            {
                pipes_need_osd.push_back(&pipelines[i]);
            }
        }

        for (size_t i = 0; i < pipes_need_osd.size(); i++)
        {
            pipes_osd_canvas[pipes_need_osd[i]->pipeid];
            pipes_osd_struct[pipes_need_osd[i]->pipeid];
            auto &canvas = pipes_osd_canvas[pipes_need_osd[i]->pipeid];
            auto &tDisp = pipes_osd_struct[pipes_need_osd[i]->pipeid];
            memset(&tDisp, 0, sizeof(AX_IVPS_RGN_DISP_GROUP_S));
            canvas.channel = 4;
            canvas.data = (unsigned char *)malloc(pipes_need_osd[i]->m_ivps_attr.n_ivps_width * pipes_need_osd[i]->m_ivps_attr.n_ivps_height * 4);
            canvas.width = pipes_need_osd[i]->m_ivps_attr.n_ivps_width;
            canvas.height = pipes_need_osd[i]->m_ivps_attr.n_ivps_height;
        }
        if (pipes_need_osd.size() && gModels.bRunJoint)
        {
            pthread_t osd_tid;
            pthread_create(&osd_tid, NULL, osd_thread, NULL);
            pthread_detach(osd_tid);
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
        for (size_t i = 0; i < pipes_need_osd.size(); i++)
        {
            auto &canvas = pipes_osd_canvas[pipes_need_osd[i]->pipeid];
            free(canvas.data);
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
        if (!gCams[i].bOpen)
            continue;
        COMMON_CAM_Close(&gCams[i]);
    }

EXIT_3:
    COMMON_CAM_Deinit();
    COMMON_JOINT_Deinit(&gModels);

EXIT_2:

    COMMON_SYS_DeInit();

    ALOGN("sample end\n");
    return 0;
}
