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

#include "sample_vin_ivps_joint_venc_rtsp_vo.h"
#include <getopt.h>
#include "../sample_run_joint/sample_run_joint_post_process.h"
#include "../common/common_joint.h"

IVPS_REGION_PARAM_T g_arrRgnThreadParam[SAMPLE_REGION_COUNT];

AX_S32 s_sample_framerate = 30;

CAMERA_T gCams[MAX_CAMERAS] = {0};

volatile AX_S32 gLoopExit = 0;
static AX_S32 g_isp_force_loop_exit = 0;

const AX_S32 gVencChnMapping[SAMPLE_VENC_CHN_NUM] = {0};
rtsp_demo_handle rDemoHandle = NULL;

pthread_mutex_t g_result_mutex;
sample_run_joint_results g_result_disp;

int SAMPLE_MAJOR_STREAM_WIDTH;
int SAMPLE_MAJOR_STREAM_HEIGHT;

sample_run_joint_models gModels = {
    .bRunJoint = AX_FALSE,
    .mMajor.JointAttr = {0},
    .mMinor.JointAttr = {0},
    .mMajor.JointHandle = NULL,
    .mMinor.JointHandle = NULL,
    .mMajor.ModelType = MT_UNKNOWN,
    .mMinor.ModelType = MT_UNKNOWN,
    .ModelType_Main = MT_UNKNOWN,
    .SAMPLE_ALGO_FORMAT = AX_YUV420_SEMIPLANAR,
    .SAMPLE_IVPS_ALGO_WIDTH = 960,
    .SAMPLE_IVPS_ALGO_HEIGHT = 540,
};

int gPipeAi_IvpsGrp; // 1
int gPipeAi_IvpsChn; // 0

/* venc task */
AX_S32 SampleVencInit(COMMON_VENC_CASE_E eVencType);
AX_S32 SampleVencDeInit();

/* osd task */
AX_VOID StartOverlay(AX_VOID);
AX_VOID StopOverlay(AX_VOID);
// osd thread
AX_VOID *RgnThreadFunc(AX_VOID *pArg);
AX_VOID *RgnThreadFunc_V2(AX_VOID *pArg);

/* ivps task */
int SampleIvpsInit();
AX_S32 SampleIvpsDeInit();
AX_S32 SampleLinkInit();
AX_S32 SampleLinkDeInit();

// ai thread
AX_VOID *GetFrameThread(AX_VOID *pArg);

// vo
AX_S32 VoInit(char *pStr);

AX_VOID VoDeInit();

AX_S32 IVPS_ThreadStart(AX_VOID *p)
{
    pthread_t tid = 0;

    pthread_attr_t attr;
    int ret = pthread_attr_init(&attr);
    static uint8_t stack_getframe[64 * 1024]; // stack < 64k will Segmentation fault
    pthread_attr_setstack(&attr, stack_getframe, sizeof(stack_getframe));
    if (0 != pthread_create(&tid, &attr, (void *(*)(void *))GetFrameThread, p))
    // if (0 != pthread_create(&tid, NULL, GetFrameThread, p))
    {
        return -1;
    }

    pthread_detach(tid);

    if (0 != pthread_create(&tid, NULL, RgnThreadFunc_V2, g_arrRgnThreadParam))
    {
        return -1;
    }

    pthread_detach(tid);

    return 0;
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

//允许外部调用
AX_VOID __sigExit(int iSigNo)
{
    // ALOGN("Catch signal %d!\n", iSigNo);
    gLoopExit = 1;
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

    printf("\t-v: Video Encode Type, default is h264\n");
    printf("\t\t0: h264\n");
    printf("\t\t1: h265\n");

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
    COMMON_VENC_CASE_E eVencType = VENC_CASE_H264;
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, __sigExit);

    ALOGN("sample begin\n\n");

    while ((ch = getopt(argc, argv, "p:m:c:e:v:r:h")) != -1)
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
        case 'v':
            eVencType = (COMMON_VENC_CASE_E)atoi(optarg);
            if (eVencType != VENC_CASE_H264 && eVencType != VENC_CASE_H265)
            {
                isExit = 1;
            }
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

    ALOGN("eSysCase=%d,eHdrMode=%d,eVencType=%d\n", eSysCase, eHdrMode, eVencType);

    s32Ret = COMMON_SET_CAM(gCams, eSysCase, eHdrMode, &eSnsType, &tCommonArgs, s_sample_framerate);
    if (s32Ret != 0)
    {
        PrintHelp(argv[0]);
        exit(0);
    }

    SAMPLE_MAJOR_STREAM_WIDTH = gCams[0].stChnAttr.tChnAttr[AX_YUV_SOURCE_ID_MAIN].nWidth;
    SAMPLE_MAJOR_STREAM_HEIGHT = gCams[0].stChnAttr.tChnAttr[AX_YUV_SOURCE_ID_MAIN].nHeight;
    printf("%d %d\n", SAMPLE_MAJOR_STREAM_WIDTH, SAMPLE_MAJOR_STREAM_HEIGHT);

    /*step 1:sys init*/
    s32Ret = COMMON_SYS_Init(&tCommonArgs);
    if (s32Ret)
    {
        ALOGE("COMMON_SYS_Init failed,s32Ret:0x%x\n", s32Ret);
        return -1;
    }

    /*step 2:link init*/
    s32Ret = SampleLinkInit();
    if (s32Ret)
    {
        ALOGE("COMMON_SYS_Init failed,s32Ret:0x%x\n", s32Ret);
        goto EXIT_1;
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

    rDemoHandle = rtsp_new_demo(RTSP_PORT);

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

    /*step 4:ivps init*/
    s32Ret = SampleIvpsInit();
    if (AX_SUCCESS != s32Ret)
    {
        ALOGE("SampleIvpsInit failed,s32Ret:0x%x\n", s32Ret);
        goto EXIT_4;
    }

    /*step 5:venc init*/
    s32Ret = SampleVencInit(eVencType);
    if (AX_SUCCESS != s32Ret)
    {
        ALOGE("SampleVencInit failed,s32Ret:0x%x\n", s32Ret);
        goto EXIT_5;
    }
    gPipeAi_IvpsGrp = 1;
    gPipeAi_IvpsChn = 0;

    s32Ret = VoInit("dsi0@480x854@60");
    if (AX_SUCCESS != s32Ret)
    {
        ALOGE("VoInit failed,s32Ret:0x%x\n", s32Ret);
        goto EXIT_5;
    }

    /*step 6:sys run*/
    IVPS_ThreadStart(NULL);

    s32Ret = SysRun();
    if (AX_SUCCESS != s32Ret)
    {
        ALOGE("SysRun error,s32Ret:0x%x\n", s32Ret);
        goto EXIT_6;
    }

EXIT_6:
    SampleVencDeInit();
    VoDeInit();

EXIT_5:
    StopOverlay();
    SampleIvpsDeInit();

EXIT_4:
    for (i = 0; i < tCommonArgs.nCamCnt; i++)
    {
        if (!gCams[i].bOpen)
            continue;
        COMMON_CAM_Close(&gCams[i]);
    }

    rtsp_del_demo(rDemoHandle);

EXIT_3:
    COMMON_CAM_Deinit();
    COMMON_JOINT_Deinit(&gModels);

EXIT_2:
    SampleLinkDeInit();

EXIT_1:
    COMMON_SYS_DeInit(&gModels);

    ALOGN("sample end\n");
    return 0;
}
