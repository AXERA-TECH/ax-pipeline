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

#include "sample_vin_ivps_joint_venc_rtsp.h"
#include <getopt.h>
#include "../sample_run_joint/sample_run_joint.h"

static COMMON_SYS_POOL_CFG_T gtSysCommPoolSingleOs04a10Sdr[] = {
    {2688, 1520, 2688, AX_FORMAT_BAYER_RAW_10BPP, 4}, /*vin raw10 use */
    {2688, 1520, 2688, AX_FORMAT_BAYER_RAW_16BPP, 4}, /*vin raw16 use */
    {2688, 1520, 2688, AX_YUV420_SEMIPLANAR, 4},      /*vin nv21/nv21 use */
    {1920, 1080, 1920, AX_YUV420_SEMIPLANAR, 4},
    {1280, 720, 1280, AX_YUV420_SEMIPLANAR, 3},
};
static COMMON_SYS_POOL_CFG_T gtSysCommPoolSingleOs04a10OnlineSdr[] = {
    {2688, 1520, 2688, AX_FORMAT_BAYER_RAW_10BPP, 3}, /*vin raw10 use */
    {2688, 1520, 2688, AX_FORMAT_BAYER_RAW_16BPP, 5}, /*vin raw16 use */
    {2688, 1520, 2688, AX_YUV420_SEMIPLANAR, 6},      /*vin nv21/nv21 use */
    {1920, 1080, 1920, AX_YUV420_SEMIPLANAR, 3},
    {1280, 720, 1280, AX_YUV420_SEMIPLANAR, 3},
};

static COMMON_SYS_POOL_CFG_T gtSysCommPoolSingleOs04a10Hdr[] = {
    {2688, 1520, 2688, AX_FORMAT_BAYER_RAW_10BPP, 4}, /*vin raw10 use */
    {2688, 1520, 2688, AX_FORMAT_BAYER_RAW_16BPP, 4}, /*vin raw16 use */
    {2688, 1520, 2688, AX_YUV420_SEMIPLANAR, 6},      /*vin nv21/nv21 use */
    {1920, 1080, 1920, AX_YUV420_SEMIPLANAR, 4},
    {720, 576, 720, AX_YUV420_SEMIPLANAR, 3},
};

static COMMON_SYS_POOL_CFG_T gtSysCommPoolSingleOs04a10OnlineHdr[] = {
    {2688, 1520, 2688, AX_FORMAT_BAYER_RAW_10BPP, 4}, /*vin raw10 use */
    {2688, 1520, 2688, AX_FORMAT_BAYER_RAW_16BPP, 4}, /*vin raw16 use */
    {2688, 1520, 2688, AX_YUV420_SEMIPLANAR, 6},      /*vin nv21/nv21 use */
    {1920, 1080, 1920, AX_YUV420_SEMIPLANAR, 4},
    {720, 576, 720, AX_YUV420_SEMIPLANAR, 3},
};

static COMMON_SYS_POOL_CFG_T gtSysCommPoolSingleImx334Sdr[] = {

    {3840, 2160, 3840, AX_FORMAT_BAYER_RAW_12BPP, 8}, /*vin raw10 use */
    {3840, 2160, 3840, AX_FORMAT_BAYER_RAW_16BPP, 4}, /*vin raw16 use */
    {3840, 2160, 3840, AX_YUV420_SEMIPLANAR, 6},      /*vin nv21/nv21 use */
    {1920, 1080, 1920, AX_YUV420_SEMIPLANAR, 4},
    {960, 540, 960, AX_YUV420_SEMIPLANAR, 3},

};

static COMMON_SYS_POOL_CFG_T gtSysCommPoolSingleImx334Hdr[] = {

    {3840, 2160, 3840, AX_FORMAT_BAYER_RAW_10BPP, 8}, /*vin raw10 use */
    {3840, 2160, 3840, AX_FORMAT_BAYER_RAW_16BPP, 4}, /*vin raw16 use */
    {3840, 2160, 3840, AX_YUV420_SEMIPLANAR, 6},      /*vin nv21/nv21 use */
    {1920, 1080, 1920, AX_YUV420_SEMIPLANAR, 4},
    {960, 540, 960, AX_YUV420_SEMIPLANAR, 3},

};

static COMMON_SYS_POOL_CFG_T gtSysCommPoolSingleGc4653[] = {

    {2560, 1440, 2560, AX_FORMAT_BAYER_RAW_10BPP, 4}, /*vin raw10 use */
    {2560, 1440, 2560, AX_FORMAT_BAYER_RAW_16BPP, 4}, /*vin raw16 use */
    {2560, 1440, 2560, AX_YUV420_SEMIPLANAR, 4},      /*vin nv21/nv21 use */
    {1280, 720, 1280, AX_YUV420_SEMIPLANAR, 4},
    {640, 360, 640, AX_YUV420_SEMIPLANAR, 3},

};

static COMMON_SYS_POOL_CFG_T gtSysCommPoolSingleOs08a20Sdr[] = {

    {3840, 2160, 3840, AX_FORMAT_BAYER_RAW_12BPP, 4}, /*vin raw10 use */
    {3840, 2160, 3840, AX_FORMAT_BAYER_RAW_16BPP, 4}, /*vin raw16 use */
    {3840, 2160, 3840, AX_YUV420_SEMIPLANAR, 6},      /*vin nv21/nv21 use */
    {1920, 1080, 1920, AX_YUV420_SEMIPLANAR, 4},
    {960, 540, 960, AX_YUV420_SEMIPLANAR, 3},

};

static COMMON_SYS_POOL_CFG_T gtSysCommPoolSingleOs08a20Hdr[] = {

    {3840, 2160, 3840, AX_FORMAT_BAYER_RAW_10BPP, 8}, /*vin raw10 use */
    {3840, 2160, 3840, AX_FORMAT_BAYER_RAW_16BPP, 4}, /*vin raw16 use */
    {3840, 2160, 3840, AX_YUV420_SEMIPLANAR, 6},      /*vin nv21/nv21 use */
    {1920, 1080, 1920, AX_YUV420_SEMIPLANAR, 4},
    {960, 540, 960, AX_YUV420_SEMIPLANAR, 3},

};

IVPS_REGION_PARAM_T g_arrRgnThreadParam[SAMPLE_REGION_COUNT];



CAMERA_T gCams[MAX_CAMERAS] = {0};

volatile AX_S32 gLoopExit = 0;
static AX_S32 g_isp_force_loop_exit = 0;

const AX_S32 gVencChnMapping[SAMPLE_VENC_CHN_NUM] = {0, 2};

sample_run_joint_results g_result_disp;
pthread_mutex_t g_result_mutex;
void *gJointHandle = NULL;
AX_BOOL b_runjoint = AX_FALSE;

int SAMPLE_ALGO_WIDTH = 640;  // 640
int SAMPLE_ALGO_HEIGHT = 640; // 640
int SAMPLE_MAJOR_STREAM_WIDTH;
int SAMPLE_MAJOR_STREAM_HEIGHT;

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

AX_S32 IVPS_ThreadStart(AX_VOID *p)
{
    pthread_t tid = 0;

    if (0 != pthread_create(&tid, NULL, GetFrameThread, p))
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

static AX_VOID __sigint(int iSigNo)
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
    AX_S32 isExit = 0, i, ch;
    AX_S32 s32Ret = 0;
    COMMON_SYS_CASE_E eSysCase = SYS_CASE_SINGLE_GC4653;
    COMMON_SYS_ARGS_T tCommonArgs = {0};
    AX_SNS_HDR_MODE_E eHdrMode = AX_SNS_LINEAR_MODE;
    SAMPLE_SNS_TYPE_E eSnsType = GALAXYCORE_GC4653;
    COMMON_VENC_CASE_E eVencType = VENC_CASE_H264;
    char model_path[256];
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, __sigint);

    ALOGN("sample_vin_ivps_joint_venc_rtsp begin\n\n");

    while ((ch = getopt(argc, argv, "p:m:c:e:v:r:h")) != -1)
    {
        switch (ch)
        {
        case 'm':
            strcpy(model_path, optarg);
            b_runjoint = AX_TRUE;
            break;
        case 'p':
        {
            int ret = sample_parse_yolov5_param(optarg);
            if (ret != 0)
            {
                ALOGE("sample_parse_yolov5_param failed");
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
                s_sample_framerate = 25;
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

    ALOGN("eSysCase=%d,eHdrMode=%d,eVencType=%d\n", eSysCase, eHdrMode, eVencType);

    if (eSysCase == SYS_CASE_SINGLE_OS04A10)
    {
        tCommonArgs.nCamCnt = 1;
        eSnsType = OMNIVISION_OS04A10;
        COMMON_ISP_GetSnsConfig(OMNIVISION_OS04A10, &gCams[0].stSnsAttr, &gCams[0].stSnsClkAttr, &gCams[0].stDevAttr, &gCams[0].stPipeAttr, &gCams[0].stChnAttr);
        if (eHdrMode == AX_SNS_LINEAR_MODE)
        {
            tCommonArgs.nPoolCfgCnt = sizeof(gtSysCommPoolSingleOs04a10Sdr) / sizeof(gtSysCommPoolSingleOs04a10Sdr[0]);
            tCommonArgs.pPoolCfg = gtSysCommPoolSingleOs04a10Sdr;
        }
        else if (eHdrMode == AX_SNS_HDR_2X_MODE)
        {
            tCommonArgs.nPoolCfgCnt = sizeof(gtSysCommPoolSingleOs04a10Hdr) / sizeof(gtSysCommPoolSingleOs04a10Hdr[0]);
            tCommonArgs.pPoolCfg = gtSysCommPoolSingleOs04a10Hdr;
        }
        gCams[0].stPipeAttr.ePipeDataSrc = AX_PIPE_SOURCE_DEV_ONLINE;
        gCams[0].stSnsAttr.nFrameRate = s_sample_framerate;
    }
    else if (eSysCase == SYS_CASE_SINGLE_OS04A10_ONLINE)
    {
        tCommonArgs.nCamCnt = 1;
        eSnsType = OMNIVISION_OS04A10;
        COMMON_ISP_GetSnsConfig(OMNIVISION_OS04A10, &gCams[0].stSnsAttr, &gCams[0].stSnsClkAttr, &gCams[0].stDevAttr, &gCams[0].stPipeAttr, &gCams[0].stChnAttr);
        if (eHdrMode == AX_SNS_LINEAR_MODE)
        {
            tCommonArgs.nPoolCfgCnt = sizeof(gtSysCommPoolSingleOs04a10OnlineSdr) / sizeof(gtSysCommPoolSingleOs04a10OnlineSdr[0]);
            tCommonArgs.pPoolCfg = gtSysCommPoolSingleOs04a10OnlineSdr;
        }
        else if (eHdrMode == AX_SNS_HDR_2X_MODE)
        {
            tCommonArgs.nPoolCfgCnt = sizeof(gtSysCommPoolSingleOs04a10OnlineHdr) / sizeof(gtSysCommPoolSingleOs04a10OnlineHdr[0]);
            tCommonArgs.pPoolCfg = gtSysCommPoolSingleOs04a10OnlineHdr;
        }
        gCams[0].stPipeAttr.ePipeDataSrc = AX_PIPE_SOURCE_DEV_ONLINE;
        gCams[0].stChnAttr.tChnAttr[0].nDepth = 1;
        gCams[0].stChnAttr.tChnAttr[1].nDepth = 1;
        gCams[0].stChnAttr.tChnAttr[2].nDepth = 1;
        gCams[0].stSnsAttr.nFrameRate = s_sample_framerate;
    }
    else if (eSysCase == SYS_CASE_SINGLE_IMX334)
    {
        tCommonArgs.nCamCnt = 1;
        eSnsType = SONY_IMX334;
        COMMON_ISP_GetSnsConfig(SONY_IMX334, &gCams[0].stSnsAttr, &gCams[0].stSnsClkAttr, &gCams[0].stDevAttr, &gCams[0].stPipeAttr,
                                &gCams[0].stChnAttr);
        if (eHdrMode == AX_SNS_LINEAR_MODE)
        {
            tCommonArgs.nPoolCfgCnt = sizeof(gtSysCommPoolSingleImx334Sdr) / sizeof(gtSysCommPoolSingleImx334Sdr[0]);
            tCommonArgs.pPoolCfg = gtSysCommPoolSingleImx334Sdr;
            gCams[0].stSnsAttr.eRawType = AX_RT_RAW12;
            gCams[0].stDevAttr.ePixelFmt = AX_FORMAT_BAYER_RAW_12BPP;
            gCams[0].stPipeAttr.ePixelFmt = AX_FORMAT_BAYER_RAW_12BPP;
        }
        else
        {
            tCommonArgs.nPoolCfgCnt = sizeof(gtSysCommPoolSingleImx334Hdr) / sizeof(gtSysCommPoolSingleImx334Hdr[0]);
            tCommonArgs.pPoolCfg = gtSysCommPoolSingleImx334Hdr;
        }
        gCams[0].stSnsAttr.nFrameRate = s_sample_framerate;
    }
    else if (eSysCase == SYS_CASE_SINGLE_GC4653)
    {
        tCommonArgs.nCamCnt = 1;
        eSnsType = GALAXYCORE_GC4653;
        tCommonArgs.nPoolCfgCnt = sizeof(gtSysCommPoolSingleGc4653) / sizeof(gtSysCommPoolSingleGc4653[0]);
        tCommonArgs.pPoolCfg = gtSysCommPoolSingleGc4653;
        COMMON_ISP_GetSnsConfig(GALAXYCORE_GC4653, &gCams[0].stSnsAttr, &gCams[0].stSnsClkAttr, &gCams[0].stDevAttr, &gCams[0].stPipeAttr,
                                &gCams[0].stChnAttr);
        gCams[0].stSnsAttr.nFrameRate = s_sample_framerate;
    }
    else if (eSysCase == SYS_CASE_SINGLE_OS08A20)
    {
        tCommonArgs.nCamCnt = 1;
        eSnsType = OMNIVISION_OS08A20;
        COMMON_ISP_GetSnsConfig(OMNIVISION_OS08A20, &gCams[0].stSnsAttr, &gCams[0].stSnsClkAttr, &gCams[0].stDevAttr, &gCams[0].stPipeAttr,
                                &gCams[0].stChnAttr);
        if (eHdrMode == AX_SNS_LINEAR_MODE)
        {
            tCommonArgs.nPoolCfgCnt = sizeof(gtSysCommPoolSingleOs08a20Sdr) / sizeof(gtSysCommPoolSingleOs08a20Sdr[0]);
            tCommonArgs.pPoolCfg = gtSysCommPoolSingleOs08a20Sdr;
            gCams[0].stSnsAttr.eRawType = AX_RT_RAW12;
            gCams[0].stDevAttr.ePixelFmt = AX_FORMAT_BAYER_RAW_12BPP;
            gCams[0].stPipeAttr.ePixelFmt = AX_FORMAT_BAYER_RAW_12BPP;
        }
        else
        {
            tCommonArgs.nPoolCfgCnt = sizeof(gtSysCommPoolSingleOs08a20Hdr) / sizeof(gtSysCommPoolSingleOs08a20Hdr[0]);
            tCommonArgs.pPoolCfg = gtSysCommPoolSingleOs08a20Hdr;
        }
        gCams[0].stSnsAttr.nFrameRate = s_sample_framerate;
    }
    else
    {
        ALOGE("Unsupported case eSysCase=%d\n", eSysCase);
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

    /*step 2:link init*/
    s32Ret = SampleLinkInit();
    if (s32Ret)
    {
        ALOGE("COMMON_SYS_Init failed,s32Ret:0x%x\n", s32Ret);
        goto EXIT_1;
    }

    for (i = 0; i < tCommonArgs.nCamCnt; i++)
    {
        gCams[i].eSnsType = eSnsType;
        gCams[i].stSnsAttr.eSnsMode = eHdrMode;
        gCams[i].stDevAttr.eSnsMode = eHdrMode;
        gCams[i].stPipeAttr.eSnsMode = eHdrMode;
        gCams[i].stChnAttr.tChnAttr[0].nDepth = 0;
        gCams[i].stChnAttr.tChnAttr[1].nDepth = 0;
        gCams[i].stChnAttr.tChnAttr[2].nDepth = 0;
        if (i == 0)
        {
            gCams[i].nDevId = 0;
            gCams[i].nRxDev = AX_MIPI_RX_DEV_0;
            gCams[i].nPipeId = 0;
        }
        else if (i == 1)
        {
            gCams[i].nDevId = 2;
            gCams[i].nRxDev = AX_MIPI_RX_DEV_2;
            gCams[i].nPipeId = 2;
        }
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

    /*
        joint 模型初始化会设置 SAMPLE_ALGO_WIDTH/SAMPLE_ALGO_HEIGHT 两个值，这两个值会用作后续设置 IVPS 的 AI 图像通道
        现在默认 IVPS 输出的 AI 图像通道，会将图像置中并填充
        本 SAMPLE 的 AI 只支持 NV12 的输入
     */
    if (b_runjoint == AX_TRUE)
    {
        s32Ret = sample_run_joint_init(model_path, &gJointHandle, &SAMPLE_ALGO_WIDTH, &SAMPLE_ALGO_HEIGHT);
        if (0 != s32Ret)
        {
            ALOGE("sample_run_joint_init failed,s32Ret:0x%x\n", s32Ret);
            goto EXIT_2;
        }
        ALOGN("load model %s success!\n", model_path);
    }
    else
    {
        ALOGN("Not specified model file\n");
    }

    /*step 3:camera init*/
    s32Ret = COMMON_CAM_Init();
    if (0 != s32Ret)
    {
        ALOGE("COMMON_CAM_Init failed,s32Ret:0x%x\n", s32Ret);
        goto EXIT_2;
    }

    AX_Rtsp_Start((AX_S32 *)gVencChnMapping, SAMPLE_VENC_CHN_NUM, eVencType == VENC_CASE_H264 ? AX_TRUE : AX_FALSE);

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

    AX_Rtsp_Stop();

EXIT_3:
    COMMON_CAM_Deinit();
    sample_run_joint_release(gJointHandle);

EXIT_2:
    SampleLinkDeInit();

EXIT_1:
    COMMON_SYS_DeInit();

    ALOGN("sample_vin_ivps_joint_venc_rtsp end\n");
    return 0;
}
