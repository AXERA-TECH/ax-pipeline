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

#include "RTSPClient.h"

#include "ax_ivps_api.h"
#include "npu_cv_kit/ax_npu_imgproc.h"

#include "fstream"
#include <getopt.h>
#include "unistd.h"
#include "stdlib.h"
#include "string.h"
#include "signal.h"
#include "vector"
#include "map"

#define pipe_count 2

AX_S32 s_sample_framerate = 25;

volatile AX_S32 gLoopExit = 0;

pthread_mutex_t g_result_mutex;
sample_run_joint_results g_result_disp;

int SAMPLE_MAJOR_STREAM_WIDTH = 1920;
int SAMPLE_MAJOR_STREAM_HEIGHT = 1080;

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

static void frameHandlerFunc(void *arg, RTP_FRAME_TYPE frame_type, int64_t timestamp, unsigned char *buf, int len)
{
    pipeline_t *pipe = (pipeline_t *)arg;
    pipeline_buffer_t buf_h264;

    switch (frame_type)
    {
    case FRAME_TYPE_VIDEO:
        buf_h264.p_vir = buf;
        buf_h264.n_size = len;
        user_input(pipe, 1, &buf_h264);
        printf("\rbuf len : %d", len);
        fflush(stdout);
        break;
    case FRAME_TYPE_AUDIO:
        printf("audio\n");
        break;
    case FRAME_TYPE_ETC:
        printf("etc\n");
        break;
    default:
        break;
    }
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

    printf("\t-f: rtsp url\n");

    printf("\t-r: Sensor&Video Framerate (framerate need supported by sensor), default is 25\n");

    exit(0);
}

int main(int argc, char *argv[])
{
    optind = 0;
    gLoopExit = 0;
    memset(&gModels, 0, sizeof(gModels));
    memset(&g_result_disp, 0, sizeof(g_result_disp));

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
    COMMON_SYS_ARGS_T tCommonArgs = {0};
    char rtsp_url[512];
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, __sigExit);

    ALOGN("sample begin\n\n");

    while ((ch = getopt(argc, argv, "p:m:f:r:h")) != -1)
    {
        switch (ch)
        {
        case 'f':
            strcpy(rtsp_url, optarg);
            ALOGI("rtsp url : %s", rtsp_url);
            break;
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
    COMMON_SYS_POOL_CFG_T poolcfg[] = {
        {1920, 1088, 1920, AX_YUV420_SEMIPLANAR, 10},
    };
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
            config0.n_osd_rgn = 1; // osd rgn 的个数，一个rgn可以osd 32个目标
        }
        pipe0.enable = 1;
        pipe0.pipeid = 0x90015;
        pipe0.m_input_type = pi_vdec_h264;
        pipe0.m_output_type = po_vo_sipeed_maix3_screen;
        pipe0.n_loog_exit = 0;            // 可以用来控制线程退出（如果有的话）
        pipe0.m_vdec_attr.n_vdec_grp = 0; // 可以重复

        pipeline_t &pipe1 = pipelines[1];
        {
            pipeline_ivps_config_t &config1 = pipe1.m_ivps_attr;
            config1.n_ivps_grp = 1; // 重复的会创建失败
            config1.n_ivps_fps = 60;
            config1.n_ivps_width = gModels.SAMPLE_IVPS_ALGO_WIDTH;
            config1.n_ivps_height = gModels.SAMPLE_IVPS_ALGO_HEIGHT;
            config1.b_letterbox = 1;
            config1.n_fifo_count = 1; // 如果想要拿到数据并输出到回调 就设为1~4
        }
        pipe1.enable = 1;
        pipe1.pipeid = 0x90016;
        pipe1.m_input_type = pi_vdec_h264;
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
        pipe1.m_vdec_attr.n_vdec_grp = 0;
        pipe1.output_func = ai_inference_func; // 图像输出的回调函数

        for (size_t i = 0; i < pipe_count; i++)
        {
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

    {
        RTSPClient *rtspClient = new RTSPClient();
        if (rtspClient->openURL(rtsp_url, 1, 2) == 0)
        {
            if (rtspClient->playURL(frameHandlerFunc, &pipelines[0], NULL, NULL) == 0)
            {
                while (!gLoopExit)
                {
                    usleep(1000 * 1000);
                }
            }
        }
        rtspClient->closeURL();
        delete rtspClient;
        gLoopExit = 1;
        sleep(1);
        pipeline_buffer_t end_buf = {0};
        user_input(&pipelines[0], 1, &end_buf);
    }

    // s32Ret = SysRun();
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
    }

EXIT_6:

EXIT_5:

EXIT_4:

EXIT_3:
    COMMON_JOINT_Deinit(&gModels);

EXIT_2:

EXIT_1:
    COMMON_SYS_DeInit();

    ALOGN("sample end\n");
    return 0;
}
