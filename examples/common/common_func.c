#include "common_func.h"
#include "../utilities/sample_log.h"
#include "string.h"
#ifdef AXERA_TARGET_CHIP_AX620
static COMMON_SYS_POOL_CFG_T gtSysCommPoolSingleOs04a10Sdr[] = {
    {2688, 1520, 2688, AX_FORMAT_BAYER_RAW_10BPP, 15}, /*vin raw10 use */
    {2688, 1520, 2688, AX_FORMAT_BAYER_RAW_16BPP, 4},  /*vin raw16 use */
    {2688, 1520, 2688, AX_YUV420_SEMIPLANAR, 5},       /*vin nv21/nv21 use */
    {1920, 1080, 1920, AX_YUV420_SEMIPLANAR, 4},
    {1280, 720, 1280, AX_YUV420_SEMIPLANAR, 4},
};

static COMMON_SYS_POOL_CFG_T gtSysCommPoolSingleOs04a10OnlineSdr[] = {
    {2688, 1520, 2688, AX_FORMAT_BAYER_RAW_10BPP, 3}, /*vin raw10 use */
    {2688, 1520, 2688, AX_FORMAT_BAYER_RAW_16BPP, 4}, /*vin raw16 use */
    {2688, 1520, 2688, AX_YUV420_SEMIPLANAR, 3},      /*vin nv21/nv21 use */
    {1920, 1080, 1920, AX_YUV420_SEMIPLANAR, 2},
    {1280, 720, 1280, AX_YUV420_SEMIPLANAR, 2},
};

static COMMON_SYS_POOL_CFG_T gtSysCommPoolSingleOs04a10Hdr[] = {
    {2688, 1520, 2688, AX_FORMAT_BAYER_RAW_10BPP, 17}, /*vin raw10 use */
    {2688, 1520, 2688, AX_FORMAT_BAYER_RAW_16BPP, 5},  /*vin raw16 use */
    {2688, 1520, 2688, AX_YUV420_SEMIPLANAR, 6},       /*vin nv21/nv21 use */
    {1920, 1080, 1920, AX_YUV420_SEMIPLANAR, 6},
    {720, 576, 720, AX_YUV420_SEMIPLANAR, 6},
};

static COMMON_SYS_POOL_CFG_T gtSysCommPoolSingleOs04a10OnlineHdr[] = {
    {2688, 1520, 2688, AX_FORMAT_BAYER_RAW_10BPP, 6}, /*vin raw10 use */
    {2688, 1520, 2688, AX_FORMAT_BAYER_RAW_16BPP, 4}, /*vin raw16 use */
    {2688, 1520, 2688, AX_YUV420_SEMIPLANAR, 3},      /*vin nv21/nv21 use */
    {1920, 1080, 1920, AX_YUV420_SEMIPLANAR, 2},
    {720, 576, 720, AX_YUV420_SEMIPLANAR, 2},
};

COMMON_SYS_POOL_CFG_T gtSysCommPoolSingleImx334Sdr[] = {

    {3840, 2160, 3840, AX_FORMAT_BAYER_RAW_12BPP, 15}, /*vin raw10 use */
    {3840, 2160, 3840, AX_FORMAT_BAYER_RAW_16BPP, 5},  /*vin raw16 use */
    {3840, 2160, 3840, AX_YUV420_SEMIPLANAR, 6},       /*vin nv21/nv21 use */
    {1920, 1080, 1920, AX_YUV420_SEMIPLANAR, 3},
    {960, 540, 960, AX_YUV420_SEMIPLANAR, 3},

};

COMMON_SYS_POOL_CFG_T gtSysCommPoolSingleImx334Hdr[] = {

    {3840, 2160, 3840, AX_FORMAT_BAYER_RAW_10BPP, 17}, /*vin raw10 use */
    {3840, 2160, 3840, AX_FORMAT_BAYER_RAW_16BPP, 5},  /*vin raw16 use */
    {3840, 2160, 3840, AX_YUV420_SEMIPLANAR, 6},       /*vin nv21/nv21 use */
    {1920, 1080, 1920, AX_YUV420_SEMIPLANAR, 6},
    {960, 540, 960, AX_YUV420_SEMIPLANAR, 6},

};

static COMMON_SYS_POOL_CFG_T gtSysCommPoolSingleGc4653[] = {

    {2560, 1440, 2560, AX_FORMAT_BAYER_RAW_10BPP, 3}, /*vin raw10 use */
    {2560, 1440, 2560, AX_FORMAT_BAYER_RAW_16BPP, 4}, /*vin raw16 use */
    {2560, 1440, 2560, AX_YUV420_SEMIPLANAR, 2},      /*vin nv21/nv21 use */
    {1280, 720, 1280, AX_YUV420_SEMIPLANAR, 2},
    {640, 360, 640, AX_YUV420_SEMIPLANAR, 2},

};

static COMMON_SYS_POOL_CFG_T gtSysCommPoolDoubleOs04a10[] = {
    {2688, 1520, 2688, AX_FORMAT_BAYER_RAW_10BPP, 15 * 2}, /*vin raw10 use */
    {2688, 1520, 2688, AX_FORMAT_BAYER_RAW_16BPP, 5 * 2},  /*vin raw16 use */
    {2688, 1520, 2688, AX_YUV420_SEMIPLANAR, 6 * 2},       /*vin nv21/nv21 use */
    {1344, 760, 1344, AX_YUV420_SEMIPLANAR, 3 * 2},
    {1344, 760, 1344, AX_YUV420_SEMIPLANAR, 3 * 2},
};

COMMON_SYS_POOL_CFG_T gtSysCommPoolSingleOs08a20Sdr[] = {

    {3840, 2160, 3840, AX_FORMAT_BAYER_RAW_12BPP, 15}, /*vin raw10 use */
    {3840, 2160, 3840, AX_FORMAT_BAYER_RAW_16BPP, 5},  /*vin raw16 use */
    {3840, 2160, 3840, AX_YUV420_SEMIPLANAR, 6},       /*vin nv21/nv21 use */
    {1920, 1080, 1920, AX_YUV420_SEMIPLANAR, 3},
    {960, 540, 960, AX_YUV420_SEMIPLANAR, 3},

};

COMMON_SYS_POOL_CFG_T gtSysCommPoolSingleOs08a20Hdr[] = {

    {3840, 2160, 3840, AX_FORMAT_BAYER_RAW_10BPP, 17}, /*vin raw10 use */
    {3840, 2160, 3840, AX_FORMAT_BAYER_RAW_16BPP, 5},  /*vin raw16 use */
    {3840, 2160, 3840, AX_YUV420_SEMIPLANAR, 6},       /*vin nv21/nv21 use */
    {1920, 1080, 1920, AX_YUV420_SEMIPLANAR, 6},
    {960, 540, 960, AX_YUV420_SEMIPLANAR, 6},

};

static COMMON_SYS_POOL_CFG_T gtSysCommPoolSingleDVP[] = {
    {1600, 300, 1600, AX_FORMAT_BAYER_RAW_8BPP, 40},  /*vin raw8 use */
    {1600, 300, 1600, AX_FORMAT_BAYER_RAW_16BPP, 5},  /*vin raw16 use */
    {1600, 300, 1600, AX_YUV422_INTERLEAVED_UYVY, 6}, /*vin nv21/nv21 use */
};

static COMMON_SYS_POOL_CFG_T gtSysCommPoolBT601[] = {
    {2688, 1520, 2688, AX_FORMAT_BAYER_RAW_10BPP, 40}, /*vin raw10 use */
    {2688, 1520, 2688, AX_FORMAT_BAYER_RAW_16BPP, 5},  /*vin raw16 use */
    {2688, 1520, 2688, AX_YUV422_INTERLEAVED_YUYV, 6}, /*vin nv21/nv21 use */
    {1920, 1080, 1920, AX_YUV422_INTERLEAVED_YUYV, 3},
    {1280, 720, 1280, AX_YUV422_INTERLEAVED_YUYV, 3},
};
static COMMON_SYS_POOL_CFG_T gtSysCommPoolBT656[] = {
    {2688, 1520, 2688, AX_FORMAT_BAYER_RAW_10BPP, 40}, /*vin raw10 use */
    {2688, 1520, 2688, AX_FORMAT_BAYER_RAW_16BPP, 5},  /*vin raw16 use */
    {2688, 1520, 2688, AX_YUV422_INTERLEAVED_YUYV, 6}, /*vin nv21/nv21 use */
    {1920, 1080, 1920, AX_YUV422_INTERLEAVED_YUYV, 3},
    {1280, 720, 1280, AX_YUV422_INTERLEAVED_YUYV, 3},
};
static COMMON_SYS_POOL_CFG_T gtSysCommPoolBT1120[] = {
    {2688, 1520, 2688, AX_FORMAT_BAYER_RAW_10BPP, 40}, /*vin raw10 use */
    {2688, 1520, 2688, AX_FORMAT_BAYER_RAW_16BPP, 5},  /*vin raw16 use */
    {2688, 1520, 2688, AX_YUV422_INTERLEAVED_YUYV, 6}, /*vin nv21/nv21 use */
    {1920, 1080, 1920, AX_YUV422_INTERLEAVED_YUYV, 3},
    {1280, 720, 1280, AX_YUV422_INTERLEAVED_YUYV, 3},
};

static COMMON_SYS_POOL_CFG_T gtSysCommPoolMIPI_YUV[] = {
    {3840, 2160, 3840, AX_FORMAT_BAYER_RAW_16BPP, 40}, /*vin raw16 use */
};

int COMMON_SET_CAM(CAMERA_T gCams[MAX_CAMERAS], COMMON_SYS_CASE_E eSysCase, AX_SNS_HDR_MODE_E eHdrMode, SAMPLE_SNS_TYPE_E *eSnsType, COMMON_SYS_ARGS_T *tCommonArgs, int s_sample_framerate)
{
    if (eSysCase >= SYS_CASE_BUTT || eSysCase <= SYS_CASE_NONE)
    {
        ALOGE("error case type\n");
        return -1;
    }

    if (eSysCase == SYS_CASE_SINGLE_OS04A10)
    {
        tCommonArgs->nCamCnt = 1;
        *eSnsType = OMNIVISION_OS04A10;
        COMMON_ISP_GetSnsConfig(OMNIVISION_OS04A10, &gCams[0].stSnsAttr, &gCams[0].stSnsClkAttr, &gCams[0].stDevAttr, &gCams[0].stPipeAttr, &gCams[0].stChnAttr);
        if (eHdrMode == AX_SNS_LINEAR_MODE)
        {
            tCommonArgs->nPoolCfgCnt = sizeof(gtSysCommPoolSingleOs04a10Sdr) / sizeof(gtSysCommPoolSingleOs04a10Sdr[0]);
            tCommonArgs->pPoolCfg = gtSysCommPoolSingleOs04a10Sdr;
        }
        else if (eHdrMode == AX_SNS_HDR_2X_MODE)
        {
            tCommonArgs->nPoolCfgCnt = sizeof(gtSysCommPoolSingleOs04a10Hdr) / sizeof(gtSysCommPoolSingleOs04a10Hdr[0]);
            tCommonArgs->pPoolCfg = gtSysCommPoolSingleOs04a10Hdr;
        }
        gCams[0].stPipeAttr.ePipeDataSrc = AX_PIPE_SOURCE_DEV_ONLINE;
        gCams[0].stSnsAttr.nFrameRate = s_sample_framerate;
    }
    else if (eSysCase == SYS_CASE_SINGLE_OS04A10_ONLINE)
    {
        tCommonArgs->nCamCnt = 1;
        *eSnsType = OMNIVISION_OS04A10;
        COMMON_ISP_GetSnsConfig(OMNIVISION_OS04A10, &gCams[0].stSnsAttr, &gCams[0].stSnsClkAttr, &gCams[0].stDevAttr, &gCams[0].stPipeAttr, &gCams[0].stChnAttr);
        if (eHdrMode == AX_SNS_LINEAR_MODE)
        {
            tCommonArgs->nPoolCfgCnt = sizeof(gtSysCommPoolSingleOs04a10OnlineSdr) / sizeof(gtSysCommPoolSingleOs04a10OnlineSdr[0]);
            tCommonArgs->pPoolCfg = gtSysCommPoolSingleOs04a10OnlineSdr;
        }
        else if (eHdrMode == AX_SNS_HDR_2X_MODE)
        {
            tCommonArgs->nPoolCfgCnt = sizeof(gtSysCommPoolSingleOs04a10OnlineHdr) / sizeof(gtSysCommPoolSingleOs04a10OnlineHdr[0]);
            tCommonArgs->pPoolCfg = gtSysCommPoolSingleOs04a10OnlineHdr;
        }
        gCams[0].stPipeAttr.ePipeDataSrc = AX_PIPE_SOURCE_DEV_ONLINE;
        gCams[0].stChnAttr.tChnAttr[0].nDepth = 1;
        gCams[0].stChnAttr.tChnAttr[1].nDepth = 1;
        gCams[0].stChnAttr.tChnAttr[2].nDepth = 1;
        gCams[0].stSnsAttr.nFrameRate = s_sample_framerate;
    }
    else if (eSysCase == SYS_CASE_SINGLE_IMX334)
    {
        tCommonArgs->nCamCnt = 1;
        *eSnsType = SONY_IMX334;
        COMMON_ISP_GetSnsConfig(SONY_IMX334, &gCams[0].stSnsAttr, &gCams[0].stSnsClkAttr, &gCams[0].stDevAttr, &gCams[0].stPipeAttr,
                                &gCams[0].stChnAttr);
        if (eHdrMode == AX_SNS_LINEAR_MODE)
        {
            tCommonArgs->nPoolCfgCnt = sizeof(gtSysCommPoolSingleImx334Sdr) / sizeof(gtSysCommPoolSingleImx334Sdr[0]);
            tCommonArgs->pPoolCfg = gtSysCommPoolSingleImx334Sdr;
            gCams[0].stSnsAttr.eRawType = AX_RT_RAW12;
            gCams[0].stDevAttr.ePixelFmt = AX_FORMAT_BAYER_RAW_12BPP;
            gCams[0].stPipeAttr.ePixelFmt = AX_FORMAT_BAYER_RAW_12BPP;
        }
        else
        {
            tCommonArgs->nPoolCfgCnt = sizeof(gtSysCommPoolSingleImx334Hdr) / sizeof(gtSysCommPoolSingleImx334Hdr[0]);
            tCommonArgs->pPoolCfg = gtSysCommPoolSingleImx334Hdr;
        }
        gCams[0].stSnsAttr.nFrameRate = s_sample_framerate;
    }
    else if (eSysCase == SYS_CASE_SINGLE_GC4653)
    {
        tCommonArgs->nCamCnt = 1;
        *eSnsType = GALAXYCORE_GC4653;
        tCommonArgs->nPoolCfgCnt = sizeof(gtSysCommPoolSingleGc4653) / sizeof(gtSysCommPoolSingleGc4653[0]);
        tCommonArgs->pPoolCfg = gtSysCommPoolSingleGc4653;
        COMMON_ISP_GetSnsConfig(GALAXYCORE_GC4653, &gCams[0].stSnsAttr, &gCams[0].stSnsClkAttr, &gCams[0].stDevAttr, &gCams[0].stPipeAttr,
                                &gCams[0].stChnAttr);
        gCams[0].stSnsAttr.nFrameRate = s_sample_framerate;
    }
    else if (eSysCase == SYS_CASE_DUAL_OS04A10)
    {
        tCommonArgs->nCamCnt = 2;
        *eSnsType = OMNIVISION_OS04A10;
        COMMON_ISP_GetSnsConfig(OMNIVISION_OS04A10, &gCams[0].stSnsAttr, &gCams[0].stSnsClkAttr, &gCams[0].stDevAttr, &gCams[0].stPipeAttr,
                                &gCams[0].stChnAttr);
        COMMON_ISP_GetSnsConfig(OMNIVISION_OS04A10, &gCams[1].stSnsAttr, &gCams[1].stSnsClkAttr, &gCams[1].stDevAttr, &gCams[1].stPipeAttr,
                                &gCams[1].stChnAttr);
        tCommonArgs->nPoolCfgCnt = sizeof(gtSysCommPoolDoubleOs04a10) / sizeof(gtSysCommPoolDoubleOs04a10[0]);
        tCommonArgs->pPoolCfg = gtSysCommPoolDoubleOs04a10;

        gCams[0].stSnsClkAttr.nSnsClkIdx = 0; /* mclk0 only by AX DEMO board, User defined */
        gCams[1].stSnsClkAttr.nSnsClkIdx = 2; /* mclk2 only by AX DEMO board, User defined */
    }
    else if (eSysCase == SYS_CASE_SINGLE_OS08A20)
    {
        tCommonArgs->nCamCnt = 1;
        *eSnsType = OMNIVISION_OS08A20;
        COMMON_ISP_GetSnsConfig(OMNIVISION_OS08A20, &gCams[0].stSnsAttr, &gCams[0].stSnsClkAttr, &gCams[0].stDevAttr, &gCams[0].stPipeAttr,
                                &gCams[0].stChnAttr);
        if (eHdrMode == AX_SNS_LINEAR_MODE)
        {
            tCommonArgs->nPoolCfgCnt = sizeof(gtSysCommPoolSingleOs08a20Sdr) / sizeof(gtSysCommPoolSingleOs08a20Sdr[0]);
            tCommonArgs->pPoolCfg = gtSysCommPoolSingleOs08a20Sdr;
            gCams[0].stSnsAttr.eRawType = AX_RT_RAW12;
            gCams[0].stDevAttr.ePixelFmt = AX_FORMAT_BAYER_RAW_12BPP;
            gCams[0].stPipeAttr.ePixelFmt = AX_FORMAT_BAYER_RAW_12BPP;
        }
        else
        {
            tCommonArgs->nPoolCfgCnt = sizeof(gtSysCommPoolSingleOs08a20Hdr) / sizeof(gtSysCommPoolSingleOs08a20Hdr[0]);
            tCommonArgs->pPoolCfg = gtSysCommPoolSingleOs08a20Hdr;
        }
        gCams[0].stSnsAttr.nFrameRate = s_sample_framerate;
    }
    else if (eSysCase == SYS_CASE_SINGLE_DVP)
    {
        tCommonArgs->nCamCnt = 1;
        gCams[0].eSnsType = SENSOR_DVP;
        COMMON_ISP_GetSnsConfig(SENSOR_DVP, &gCams[0].stSnsAttr, &gCams[0].stSnsClkAttr, &gCams[0].stDevAttr,
                                &gCams[0].stPipeAttr, &gCams[0].stChnAttr);

        tCommonArgs->nPoolCfgCnt = sizeof(gtSysCommPoolSingleDVP) / sizeof(gtSysCommPoolSingleDVP[0]);
        tCommonArgs->pPoolCfg = gtSysCommPoolSingleDVP;
    }
    else if (eSysCase == SYS_CASE_SINGLE_BT601)
    {
        tCommonArgs->nCamCnt = 1;
        gCams[0].eSnsType = SENSOR_BT601;
        COMMON_ISP_GetSnsConfig(SENSOR_BT601, &gCams[0].stSnsAttr, &gCams[0].stSnsClkAttr, &gCams[0].stDevAttr, &gCams[0].stPipeAttr,
                                &gCams[0].stChnAttr);

        tCommonArgs->nPoolCfgCnt = sizeof(gtSysCommPoolBT601) / sizeof(gtSysCommPoolBT601[0]);
        tCommonArgs->pPoolCfg = gtSysCommPoolBT601;
    }
    else if (eSysCase == SYS_CASE_SINGLE_BT656)
    {
        tCommonArgs->nCamCnt = 1;
        gCams[0].eSnsType = SENSOR_BT656;
        COMMON_ISP_GetSnsConfig(SENSOR_BT656, &gCams[0].stSnsAttr, &gCams[0].stSnsClkAttr, &gCams[0].stDevAttr, &gCams[0].stPipeAttr,
                                &gCams[0].stChnAttr);

        tCommonArgs->nPoolCfgCnt = sizeof(gtSysCommPoolBT656) / sizeof(gtSysCommPoolBT656[0]);
        tCommonArgs->pPoolCfg = gtSysCommPoolBT656;
    }
    else if (eSysCase == SYS_CASE_SINGLE_BT1120)
    {
        tCommonArgs->nCamCnt = 1;
        gCams[0].eSnsType = SENSOR_BT1120;
        COMMON_ISP_GetSnsConfig(SENSOR_BT1120, &gCams[0].stSnsAttr, &gCams[0].stSnsClkAttr, &gCams[0].stDevAttr, &gCams[0].stPipeAttr,
                                &gCams[0].stChnAttr);

        tCommonArgs->nPoolCfgCnt = sizeof(gtSysCommPoolBT1120) / sizeof(gtSysCommPoolBT1120[0]);
        tCommonArgs->pPoolCfg = gtSysCommPoolBT1120;
    }
    else if (eSysCase == SYS_CASE_MIPI_YUV)
    {
        tCommonArgs->nCamCnt = 1;
        *eSnsType = MIPI_YUV;
        COMMON_ISP_GetSnsConfig(MIPI_YUV, &gCams[0].stSnsAttr, &gCams[0].stSnsClkAttr, &gCams[0].stDevAttr, &gCams[0].stPipeAttr,
                                &gCams[0].stChnAttr);
        tCommonArgs->nPoolCfgCnt = sizeof(gtSysCommPoolMIPI_YUV) / sizeof(gtSysCommPoolMIPI_YUV[0]);
        tCommonArgs->pPoolCfg = gtSysCommPoolMIPI_YUV;
    }

    for (int i = 0; i < tCommonArgs->nCamCnt; i++)
    {
        gCams[i].eSnsType = *eSnsType;
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
    return 0;
}
#elif defined(AXERA_TARGET_CHIP_AX650)

typedef enum
{
    SAMPLE_VIN_HAL_CASE_NONE = -1,
    SAMPLE_VIN_HAL_CASE_SINGLE_DUMMY = 0,
    SAMPLE_VIN_HAL_CASE_SINGLE_OS08A20 = 1,
    SAMPLE_VIN_HAL_CASE_BUTT
} SAMPLE_VIN_HAL_CASE_E;

typedef struct
{
    SAMPLE_VIN_HAL_CASE_E eVinCase;
    COMMON_VIN_MODE_E eSysMode;
    AX_SNS_HDR_MODE_E eHdrMode;
    AX_BOOL bAiispEnable;
    AX_S32 nDumpFrameNum;
} SAMPLE_VIN_PARAM_T;

/* comm pool */
COMMON_SYS_POOL_CFG_T gtSysCommPoolSingleOs08a20[] = {
    {3840, 2160, 3840, AX_FORMAT_YUV420_SEMIPLANAR, 20}, /* vin nv21/nv21 use */
};

/* priv pool */
COMMON_SYS_POOL_CFG_T gtPrivPoolSingleOs08a20[] = {
    {3840, 2160, 3840, AX_FORMAT_BAYER_RAW_16BPP, 25 * 2}, /* vin raw16 use */
};

static AX_CAMERA_T gCams[MAX_CAMERAS] = {0};
static COMMON_SYS_ARGS_T tCommonArgs = {0};
static COMMON_SYS_ARGS_T tPrivArgs = {0};

static AX_VOID cal_dump_pool(COMMON_SYS_POOL_CFG_T pool[], AX_SNS_HDR_MODE_E eHdrMode, AX_S32 nFrameNum)
{
    if (NULL == pool)
    {
        return;
    }
    if (nFrameNum > 0)
    {
        switch (eHdrMode)
        {
        case AX_SNS_LINEAR_MODE:
            pool[0].nBlkCnt += nFrameNum;
            break;

        case AX_SNS_HDR_2X_MODE:
            pool[0].nBlkCnt += nFrameNum * 2;
            break;

        case AX_SNS_HDR_3X_MODE:
            pool[0].nBlkCnt += nFrameNum * 3;
            break;

        case AX_SNS_HDR_4X_MODE:
            pool[0].nBlkCnt += nFrameNum * 4;
            break;

        default:
            pool[0].nBlkCnt += nFrameNum;
            break;
        }
    }
}

static AX_VOID set_pipe_hdr_mode(AX_U32 *pHdrSel, AX_SNS_HDR_MODE_E eHdrMode)
{
    if (NULL == pHdrSel)
    {
        return;
    }

    switch (eHdrMode)
    {
    case AX_SNS_LINEAR_MODE:
        *pHdrSel = 0x1;
        break;

    case AX_SNS_HDR_2X_MODE:
        *pHdrSel = 0x1 | 0x2;
        break;

    case AX_SNS_HDR_3X_MODE:
        *pHdrSel = 0x1 | 0x2 | 0x4;
        break;

    case AX_SNS_HDR_4X_MODE:
        *pHdrSel = 0x1 | 0x2 | 0x4 | 0x8;
        break;

    default:
        *pHdrSel = 0x1;
        break;
    }
}

static AX_U32 __vin_cfg_cams_params(SAMPLE_VIN_HAL_CASE_E eVinCase,
                                    COMMON_VIN_MODE_E eSysMode,
                                    AX_SNS_HDR_MODE_E eHdrMode,
                                    AX_BOOL bAiispEnable,
                                    COMMON_SYS_ARGS_T *pCommonArgs)
{
    AX_S32 i = 0;
    SAMPLE_SNS_TYPE_E eSnsType = OMNIVISION_OS08A20;
    AX_CHAR board_id[BOARD_ID_LEN] = {0};
    AX_U8 nPipeId = gCams[0].nPipeId;

    COMM_ISP_PRT("eVinCase %d, eSysMode %d, eHdrMode %d\n", eVinCase, eSysMode, eHdrMode);

    if (eVinCase == SAMPLE_VIN_HAL_CASE_SINGLE_DUMMY)
    {
        pCommonArgs->nCamCnt = 1;
        eSnsType = SAMPLE_SNS_DUMMY;
        COMMON_VIN_GetSnsConfig(SAMPLE_SNS_DUMMY,
                                &gCams[0].tMipiRx,
                                &gCams[0].tSnsAttr,
                                &gCams[0].tSnsClkAttr,
                                &gCams[0].tDevAttr);
        COMMON_VIN_GetPipeConfig(SAMPLE_SNS_DUMMY,
                                 &gCams[0].tPipeAttr[nPipeId], gCams[0].tChnAttr[nPipeId], gCams[0].bChnEnable[nPipeId]);

        gCams[0].tPipeInfo[0].eFrameSrcType = AX_VIN_FRAME_SOURCE_TYPE_USER;
    }
    else if (eVinCase == SAMPLE_VIN_HAL_CASE_SINGLE_OS08A20)
    {
        pCommonArgs->nCamCnt = 1;
        eSnsType = OMNIVISION_OS08A20;
        COMMON_VIN_GetSnsConfig(OMNIVISION_OS08A20,
                                &gCams[0].tMipiRx,
                                &gCams[0].tSnsAttr,
                                &gCams[0].tSnsClkAttr,
                                &gCams[0].tDevAttr);
        COMMON_VIN_GetPipeConfig(OMNIVISION_OS08A20,
                                 &gCams[0].tPipeAttr[nPipeId], gCams[0].tChnAttr[nPipeId], gCams[0].bChnEnable[nPipeId]);

        gCams[0].nDevId = 0;
        gCams[0].nRxDev = 0;
        gCams[0].nPipeId = 0;
        gCams[0].tSnsClkAttr.nSnsClkIdx = 0;
        gCams[0].tDevBindPipe.nNum = 1;
        gCams[0].tDevBindPipe.nPipeId[0] = gCams[0].nPipeId;
        gCams[0].tChnAttr[0]->tCompressInfo.enCompressMode = AX_COMPRESS_MODE_NONE;
        set_pipe_hdr_mode(&gCams[0].tDevBindPipe.nHDRSel[0], eHdrMode);
        gCams[0].tPipeInfo[0].eFrameSrcType = AX_VIN_FRAME_SOURCE_TYPE_USER;
    }

    for (i = 0; i < pCommonArgs->nCamCnt; i++)
    {
        gCams[i].eSnsType = eSnsType;
        gCams[i].tSnsAttr.eSnsMode = eHdrMode;
        gCams[i].tDevAttr.eSnsMode = eHdrMode;
        gCams[i].tPipeAttr[nPipeId].eSnsMode = eHdrMode;
        gCams[i].eHdrMode = eHdrMode;
        gCams[i].eSysMode = eSysMode;
        gCams[i].tPipeAttr[nPipeId].bAiIspEnable = bAiispEnable;
        gCams[i].ptSnsHdl[nPipeId] = COMMON_ISP_GetSnsObj(eSnsType);
        gCams[i].eBusType = COMMON_ISP_GetSnsBusType(eSnsType);
        for (AX_S32 j = 0; j < AX_VIN_MAX_PIPE_NUM; j++)
        {
            gCams[i].tPipeInfo[j].ePipeMode = SAMPLE_PIPE_MODE_VIDEO;
            gCams[i].tPipeInfo[j].bAiispEnable = bAiispEnable;
            strncpy(gCams[i].tPipeInfo[j].szBinPath, "null.bin", sizeof(gCams[i].tPipeInfo[j].szBinPath));
        }

        if (eHdrMode > AX_SNS_LINEAR_MODE)
        {
            gCams[i].tDevAttr.eSnsOutputMode = AX_SNS_DOL_HDR;
        }
        else
        {
            gCams[i].tSnsAttr.eRawType = AX_RT_RAW12;
            gCams[i].tDevAttr.ePixelFmt = AX_FORMAT_BAYER_RAW_12BPP;
            gCams[i].tPipeAttr[nPipeId].ePixelFmt = AX_FORMAT_BAYER_RAW_12BPP;
        }
        gCams[i].bRegisterSns = AX_TRUE;
        if (COMMON_VIN_LOADRAW == eSysMode)
        {
            gCams[i].bEnableDev = AX_FALSE;
        }
        else
        {
            gCams[i].bEnableDev = AX_TRUE;
        }
    }

    return 0;
}

AX_S32 SAMPLE_VIN_Init()
{

    tCommonArgs.nPoolCfgCnt = sizeof(gtSysCommPoolSingleOs08a20) / sizeof(gtSysCommPoolSingleOs08a20[0]);
    tCommonArgs.pPoolCfg = gtSysCommPoolSingleOs08a20;
    tPrivArgs.nPoolCfgCnt = sizeof(gtPrivPoolSingleOs08a20) / sizeof(gtPrivPoolSingleOs08a20[0]);
    tPrivArgs.pPoolCfg = gtPrivPoolSingleOs08a20;

    AX_S32 nRet = 0;

    SAMPLE_VIN_PARAM_T tVinParam = {
        SAMPLE_VIN_HAL_CASE_SINGLE_OS08A20,
        COMMON_VIN_SENSOR,
        AX_SNS_LINEAR_MODE,
        AX_FALSE,
        0,
    };

    nRet = COMMON_SYS_Init(&tCommonArgs);
    if (nRet)
    {
        COMM_ISP_PRT("COMMON_SYS_Init fail, ret:0x%x", nRet);
        return -1;
    }

    if (AX_TRUE == tVinParam.bAiispEnable)
    {
        if (tVinParam.eVinCase != SAMPLE_VIN_HAL_CASE_SINGLE_OS08A20)
        {
            COMM_ISP_PRT("when parameter -a 1,parameter -c: ISP Pipeline Case must be 1\n");
            return -1;
        }
    }
    if (tVinParam.eVinCase >= SAMPLE_VIN_HAL_CASE_BUTT || tVinParam.eVinCase <= SAMPLE_VIN_HAL_CASE_NONE)
    {
        COMM_ISP_PRT("error sys case : %d\n", tVinParam.eVinCase);
        return -1;
    }
    if (tVinParam.eSysMode >= COMMON_VIN_BUTT || tVinParam.eSysMode <= COMMON_VIN_NONE)
    {
        COMM_ISP_PRT("error sys mode : %d\n", tVinParam.eSysMode);
        return -1;
    }

    __vin_cfg_cams_params(tVinParam.eVinCase, tVinParam.eSysMode, tVinParam.eHdrMode,
                          tVinParam.bAiispEnable, &tCommonArgs);
    nRet = COMMON_NPU_Init(AX_ENGINE_VIRTUAL_NPU_BIG_LITTLE);
    if (nRet)
    {
        return nRet;
    }
    nRet = COMMON_CAM_Init();
    if (nRet)
    {
        return nRet;
    }

    nRet = COMMON_CAM_PrivPoolInit(&tPrivArgs);
    if (nRet)
    {
        return nRet;
    }

    // /* Step1: cam config & pool Config */
    // __sample_case_config(&tVinParam, &tCommonArgs, &tPrivArgs);

    // /* Step2: SYS Init */
    // axRet = COMMON_SYS_Init(&tCommonArgs);
    // if (axRet)
    // {
    //     COMM_ISP_PRT("COMMON_SYS_Init fail, ret:0x%x", axRet);
    //     return -1;
    // }
    // /* Step3: NPU Init */
    // axRet = COMMON_NPU_Init(AX_ENGINE_VIRTUAL_NPU_BIG_LITTLE);
    // if (axRet)
    // {
    //     COMM_ISP_PRT("COMMON_NPU_Init fail, ret:0x%x", axRet);
    //     return -1;
    // }
    // /* Step4: Cam Init */
    // axRet = COMMON_CAM_Init();
    // if (axRet)
    // {
    //     COMM_ISP_PRT("COMMON_CAM_Init fail, ret:0x%x", axRet);
    //     return -1;
    // }
    // axRet = COMMON_CAM_PrivPoolInit(&tPrivArgs);
    // if (axRet)
    // {
    //     COMM_ISP_PRT("COMMON_CAM_PrivPoolInit fail, ret:0x%x", axRet);
    //     return -1;
    // }
    return 0;
}

AX_S32 SAMPLE_VIN_Open()
{
    /* Step5: Cam Open */
    AX_S32 axRet = COMMON_CAM_Open(&gCams[0], tCommonArgs.nCamCnt);
    if (axRet)
    {
        COMM_ISP_PRT("COMMON_CAM_Open fail, ret:0x%x", axRet);
        return -1;
    }
    return 0;
}

AX_S32 SAMPLE_VIN_Start()
{
    // /* Step7: Cam Run */
    // AX_S32 axRet = COMMON_CAM_Run(&gCams[0], tCommonArgs.nCamCnt);
    // if (axRet)
    // {
    //     COMM_ISP_PRT("COMMON_CAM_Open fail, ret:0x%x", axRet);
    //     return -1;
    // }
    return 0;
}
AX_S32 SAMPLE_VIN_Deinit()
{
    // AX_S32 s32Ret = COMMON_CAM_Stop();
    // if (s32Ret)
    // {
    //     ALOGE("COMMON_CAM_Stop fail, ret:0x%x", s32Ret);
    //     return -1;
    // }
    AX_S32 s32Ret = COMMON_CAM_Close(&gCams[0], tCommonArgs.nCamCnt);
    if (s32Ret)
    {
        ALOGE("COMMON_CAM_Close fail, ret:0x%x", s32Ret);
        return -1;
    }
    s32Ret = COMMON_CAM_Deinit();
    if (s32Ret)
    {
        ALOGE("COMMON_CAM_Deinit fail, ret:0x%x", s32Ret);
        return -1;
    }
    return 0;
}

#elif defined(AXERA_TARGET_CHIP_AX620E)
#include "string.h"

typedef struct
{
    SAMPLE_VIN_CASE_E eSysCase;
    COMMON_VIN_MODE_E eSysMode;
    AX_SNS_HDR_MODE_E eHdrMode;
    SAMPLE_LOAD_RAW_NODE_E eLoadRawNode;
    AX_BOOL bAiispEnable;
    AX_S32 nDumpFrameNum;
} SAMPLE_VIN_PARAM_T;

/* comm pool */
COMMON_SYS_POOL_CFG_T gtSysCommPoolSingleDummySdr[] = {
    {2688, 1520, 2688, AX_FORMAT_BAYER_RAW_16BPP, 10},                             /* vin raw16 use */
    {2688, 1520, 2688, AX_FORMAT_YUV420_SEMIPLANAR, 10},                           /* vin nv21/nv21 use */
    {2688, 1520, 2688, AX_FORMAT_YUV420_SEMIPLANAR, 3, AX_COMPRESS_MODE_LOSSY, 4}, /* vin nv21/nv21 use */
};

COMMON_SYS_POOL_CFG_T gtSysCommPoolSingleOs04a10Sdr[] = {
    {2688, 1520, 2688, AX_FORMAT_YUV420_SEMIPLANAR, 6, AX_COMPRESS_MODE_LOSSY, 4}, /* vin nv21/nv21 use */
    {1920, 1080, 1920, AX_FORMAT_YUV420_SEMIPLANAR, 3, AX_COMPRESS_MODE_LOSSY, 4}, /* vin nv21/nv21 use */
    {720, 576, 720, AX_FORMAT_YUV420_SEMIPLANAR, 3},                               /* vin nv21/nv21 use */
};
/* private pool */
COMMON_SYS_POOL_CFG_T gtPrivatePoolSingleDummySdr[] = {
    {2688, 1520, 2688, AX_FORMAT_BAYER_RAW_16BPP, 10},
};

COMMON_SYS_POOL_CFG_T gtPrivatePoolSingleOs04a10Sdr[] = {
    {2688, 1520, 2688, AX_FORMAT_BAYER_RAW_10BPP_PACKED, 12, AX_COMPRESS_MODE_LOSSY, 4}, /* vin raw16 use */
};

// SC450AI
COMMON_SYS_POOL_CFG_T gtSysCommPoolSingleOs450aiSdr[] = {
    {2688, 1520, 2688, AX_FORMAT_YUV420_SEMIPLANAR, 4, AX_COMPRESS_MODE_LOSSY, 4}, /* vin nv21/nv21 use */
    {1920, 1080, 1920, AX_FORMAT_YUV420_SEMIPLANAR, 3, AX_COMPRESS_MODE_LOSSY, 4}, /* vin nv21/nv21 use */
    {720, 576, 720, AX_FORMAT_YUV420_SEMIPLANAR, 3},                               /* vin nv21/nv21 use */
};

COMMON_SYS_POOL_CFG_T gtPrivatePoolSingleOs450aiSdr[] = {
    {2688, 1520, 2688, AX_FORMAT_BAYER_RAW_10BPP_PACKED, 8, AX_COMPRESS_MODE_LOSSY, 4}, /* vin raw10 use */
};

static AX_CAMERA_T gCams[MAX_CAMERAS] = {0};
// static volatile AX_S32 gLoopExit = 0;
static COMMON_SYS_ARGS_T tCommonArgs = {0};
static COMMON_SYS_ARGS_T tPrivArgs = {0};
extern AX_CHIP_TYPE_E AX_SYS_GetChipType(AX_VOID);

static AX_VOID __cal_dump_pool(COMMON_SYS_POOL_CFG_T pool[], AX_SNS_HDR_MODE_E eHdrMode, AX_S32 nFrameNum)
{
    if (NULL == pool)
    {
        return;
    }
    if (nFrameNum > 0)
    {
        switch (eHdrMode)
        {
        case AX_SNS_LINEAR_MODE:
            pool[0].nBlkCnt += nFrameNum;
            break;

        case AX_SNS_HDR_2X_MODE:
            pool[0].nBlkCnt += nFrameNum * 2;
            break;

        case AX_SNS_HDR_3X_MODE:
            pool[0].nBlkCnt += nFrameNum * 3;
            break;

        case AX_SNS_HDR_4X_MODE:
            pool[0].nBlkCnt += nFrameNum * 4;
            break;

        default:
            pool[0].nBlkCnt += nFrameNum;
            break;
        }
    }
}

static AX_VOID __set_pipe_hdr_mode(AX_U32 *pHdrSel, AX_SNS_HDR_MODE_E eHdrMode)
{
    if (NULL == pHdrSel)
    {
        return;
    }

    switch (eHdrMode)
    {
    case AX_SNS_LINEAR_MODE:
        *pHdrSel = 0x1;
        break;

    case AX_SNS_HDR_2X_MODE:
        *pHdrSel = 0x1 | 0x2;
        break;

    case AX_SNS_HDR_3X_MODE:
        *pHdrSel = 0x1 | 0x2 | 0x4;
        break;

    case AX_SNS_HDR_4X_MODE:
        *pHdrSel = 0x1 | 0x2 | 0x4 | 0x8;
        break;

    default:
        *pHdrSel = 0x1;
        break;
    }
}

static AX_VOID __set_vin_attr(AX_CAMERA_T *pCam, SAMPLE_SNS_TYPE_E eSnsType, AX_SNS_HDR_MODE_E eHdrMode,
                              COMMON_VIN_MODE_E eSysMode, AX_BOOL bAiispEnable)
{
    pCam->eSnsType = eSnsType;
    pCam->tSnsAttr.eSnsMode = eHdrMode;
    pCam->tDevAttr.eSnsMode = eHdrMode;
    pCam->eHdrMode = eHdrMode;
    pCam->eSysMode = eSysMode;
    pCam->tPipeAttr.eSnsMode = eHdrMode;
    pCam->tPipeAttr.bAiIspEnable = bAiispEnable;
    if (eHdrMode > AX_SNS_LINEAR_MODE)
    {
        pCam->tDevAttr.eSnsOutputMode = AX_SNS_DOL_HDR;
    }

    if (COMMON_VIN_TPG == eSysMode)
    {
        pCam->tDevAttr.eSnsIntfType = AX_SNS_INTF_TYPE_TPG;
    }

    if (COMMON_VIN_LOADRAW == eSysMode)
    {
        pCam->bEnableDev = AX_FALSE;
    }
    else
    {
        pCam->bEnableDev = AX_TRUE;
    }

    pCam->bRegisterSns = AX_TRUE;

    return;
}

static AX_U32 __sample_case_single_dummy(AX_CAMERA_T *pCamList, SAMPLE_SNS_TYPE_E eSnsType,
                                         SAMPLE_VIN_PARAM_T *pVinParam, COMMON_SYS_ARGS_T *pCommonArgs)
{
    AX_S32 i = 0;
    AX_CAMERA_T *pCam = NULL;
    COMMON_VIN_MODE_E eSysMode = pVinParam->eSysMode;
    AX_SNS_HDR_MODE_E eHdrMode = pVinParam->eHdrMode;
    SAMPLE_LOAD_RAW_NODE_E eLoadRawNode = pVinParam->eLoadRawNode;
    pCam = &pCamList[0];
    pCommonArgs->nCamCnt = 1;

    for (i = 0; i < pCommonArgs->nCamCnt; i++)
    {
        pCam = &pCamList[i];
        COMMON_VIN_GetSnsConfig(eSnsType, &pCam->tMipiAttr, &pCam->tSnsAttr,
                                &pCam->tSnsClkAttr, &pCam->tDevAttr,
                                &pCam->tPipeAttr, pCam->tChnAttr);

        pCam->nDevId = 0;
        pCam->nRxDev = 0;
        pCam->nPipeId = 0;
        pCam->tSnsClkAttr.nSnsClkIdx = 0;
        pCam->tDevBindPipe.nNum = 1;
        pCam->tDevBindPipe.nPipeId[0] = pCam->nPipeId;
        pCam->ptSnsHdl = COMMON_ISP_GetSnsObj(eSnsType);
        pCam->eBusType = COMMON_ISP_GetSnsBusType(eSnsType);
        pCam->eLoadRawNode = eLoadRawNode;
        __set_pipe_hdr_mode(&pCam->tDevBindPipe.nHDRSel[0], eHdrMode);
        __set_vin_attr(pCam, eSnsType, eHdrMode, eSysMode, pVinParam->bAiispEnable);
        for (AX_S32 j = 0; j < AX_VIN_MAX_PIPE_NUM; j++)
        {
            pCam->tPipeInfo[j].ePipeMode = SAMPLE_PIPE_MODE_VIDEO;
            pCam->tPipeInfo[j].bAiispEnable = pVinParam->bAiispEnable;
            strncpy(pCam->tPipeInfo[j].szBinPath, "null.bin", sizeof(pCam->tPipeInfo[j].szBinPath));
        }
    }

    return 0;
}
#if 0
static AX_U32 __sample_case_single_os08a20(AX_CAMERA_T *pCamList, SAMPLE_SNS_TYPE_E eSnsType,
        SAMPLE_VIN_PARAM_T *pVinParam, COMMON_SYS_ARGS_T *pCommonArgs)
{
    AX_CAMERA_T *pCam = NULL;
    COMMON_VIN_MODE_E eSysMode = pVinParam->eSysMode;
    AX_SNS_HDR_MODE_E eHdrMode = pVinParam->eHdrMode;
    AX_S32 j = 0;
    pCommonArgs->nCamCnt = 1;
    pCam = &pCamList[0];
    COMMON_VIN_GetSnsConfig(eSnsType, &pCam->tMipiAttr, &pCam->tSnsAttr,
                            &pCam->tSnsClkAttr, &pCam->tDevAttr,
                            &pCam->tPipeAttr, pCam->tChnAttr);

    pCam->nDevId = 0;
    pCam->nRxDev = 0;
    pCam->nPipeId = 0;
    pCam->tSnsClkAttr.nSnsClkIdx = 0;
    pCam->tDevBindPipe.nNum =  1;
    pCam->tDevBindPipe.nPipeId[0] = pCam->nPipeId;
    pCam->ptSnsHdl = COMMON_ISP_GetSnsObj(eSnsType);
    pCam->eBusType = COMMON_ISP_GetSnsBusType(eSnsType);
    __set_pipe_hdr_mode(&pCam->tDevBindPipe.nHDRSel[0], eHdrMode);
    __set_vin_attr(pCam, eSnsType, eHdrMode, eSysMode, pVinParam->bAiispEnable);
    for (j = 0; j < pCam->tDevBindPipe.nNum; j++) {
        pCam->tPipeInfo[j].ePipeMode = SAMPLE_PIPE_MODE_VIDEO;
        pCam->tPipeInfo[j].bAiispEnable = pVinParam->bAiispEnable;
        if (pCam->tPipeInfo[j].bAiispEnable) {
            if (eHdrMode <= AX_SNS_LINEAR_MODE) {
                strncpy(pCam->tPipeInfo[j].szBinPath, "/opt/etc/os08a20_sdr_dual3dnr.bin", sizeof(pCam->tPipeInfo[j].szBinPath));
            } else {
                strncpy(pCam->tPipeInfo[j].szBinPath, "/opt/etc/os08a20_hdr_2x_ainr.bin", sizeof(pCam->tPipeInfo[j].szBinPath));
            }
        } else {
            strncpy(pCam->tPipeInfo[j].szBinPath, "null.bin", sizeof(pCam->tPipeInfo[j].szBinPath));
        }
    }
    return 0;
}
#endif
static AX_U32 __sample_case_single_os04a10(AX_CAMERA_T *pCamList, SAMPLE_SNS_TYPE_E eSnsType,
                                           SAMPLE_VIN_PARAM_T *pVinParam, COMMON_SYS_ARGS_T *pCommonArgs)
{
    AX_CAMERA_T *pCam = NULL;
    COMMON_VIN_MODE_E eSysMode = pVinParam->eSysMode;
    AX_SNS_HDR_MODE_E eHdrMode = pVinParam->eHdrMode;
    AX_S32 j = 0;
    pCommonArgs->nCamCnt = 1;
    pCam = &pCamList[0];
    COMMON_VIN_GetSnsConfig(eSnsType, &pCam->tMipiAttr, &pCam->tSnsAttr,
                            &pCam->tSnsClkAttr, &pCam->tDevAttr,
                            &pCam->tPipeAttr, pCam->tChnAttr);
    pCam->nDevId = 0;
    pCam->nRxDev = 0;
    pCam->nPipeId = 0;
    pCam->tSnsClkAttr.nSnsClkIdx = 0;
    pCam->tDevBindPipe.nNum = 1;
    pCam->tDevBindPipe.nPipeId[0] = pCam->nPipeId;
    pCam->ptSnsHdl = COMMON_ISP_GetSnsObj(eSnsType);
    pCam->eBusType = COMMON_ISP_GetSnsBusType(eSnsType);
    __set_pipe_hdr_mode(&pCam->tDevBindPipe.nHDRSel[0], eHdrMode);
    __set_vin_attr(pCam, eSnsType, eHdrMode, eSysMode, pVinParam->bAiispEnable);
    for (j = 0; j < pCam->tDevBindPipe.nNum; j++)
    {
        pCam->tPipeInfo[j].ePipeMode = SAMPLE_PIPE_MODE_VIDEO;
        pCam->tPipeInfo[j].bAiispEnable = pVinParam->bAiispEnable;
        strncpy(pCam->tPipeInfo[j].szBinPath, "null.bin", sizeof(pCam->tPipeInfo[j].szBinPath));
    }
    return 0;
}

static AX_U32 __sample_case_single_sc450ai(AX_CAMERA_T *pCamList, SAMPLE_SNS_TYPE_E eSnsType,
                                           SAMPLE_VIN_PARAM_T *pVinParam, COMMON_SYS_ARGS_T *pCommonArgs)
{
    AX_CAMERA_T *pCam = NULL;
    COMMON_VIN_MODE_E eSysMode = pVinParam->eSysMode;
    AX_SNS_HDR_MODE_E eHdrMode = pVinParam->eHdrMode;
    AX_S32 j = 0;
    SAMPLE_LOAD_RAW_NODE_E eLoadRawNode = pVinParam->eLoadRawNode;
    pCommonArgs->nCamCnt = 1;

    pCam = &pCamList[0];
    COMMON_VIN_GetSnsConfig(eSnsType, &pCam->tMipiAttr, &pCam->tSnsAttr,
                            &pCam->tSnsClkAttr, &pCam->tDevAttr,
                            &pCam->tPipeAttr, pCam->tChnAttr);
    pCam->nDevId = 0;
    pCam->nRxDev = 0;
    pCam->nPipeId = 0;
    pCam->tSnsClkAttr.nSnsClkIdx = 0;
    pCam->tDevBindPipe.nNum = 1;
    pCam->eLoadRawNode = eLoadRawNode;
    pCam->tDevBindPipe.nPipeId[0] = pCam->nPipeId;
    pCam->ptSnsHdl = COMMON_ISP_GetSnsObj(eSnsType);
    pCam->eBusType = COMMON_ISP_GetSnsBusType(eSnsType);
    pCam->eInputMode = AX_INPUT_MODE_MIPI;
    __set_pipe_hdr_mode(&pCam->tDevBindPipe.nHDRSel[0], eHdrMode);
    __set_vin_attr(pCam, eSnsType, eHdrMode, eSysMode, pVinParam->bAiispEnable);
    for (j = 0; j < pCam->tDevBindPipe.nNum; j++)
    {
        pCam->tPipeInfo[j].ePipeMode = SAMPLE_PIPE_MODE_VIDEO;
        pCam->tPipeInfo[j].bAiispEnable = pVinParam->bAiispEnable;
        strncpy(pCam->tPipeInfo[j].szBinPath, "null.bin", sizeof(pCam->tPipeInfo[j].szBinPath));
    }
    return 0;
}

static AX_U32 __sample_case_config(SAMPLE_VIN_PARAM_T *pVinParam, COMMON_SYS_ARGS_T *pCommonArgs,
                                   COMMON_SYS_ARGS_T *pPrivArgs)
{
    AX_CAMERA_T *pCamList = &gCams[0];
    SAMPLE_SNS_TYPE_E eSnsType = OMNIVISION_OS04A10;

    ALOGI("eSysCase %d, eSysMode %d, eLoadRawNode %d, eHdrMode %d, bAiispEnable %d", pVinParam->eSysCase,
          pVinParam->eSysMode,
          pVinParam->eLoadRawNode, pVinParam->eHdrMode, pVinParam->bAiispEnable);

    switch (pVinParam->eSysCase)
    {
    case SAMPLE_VIN_SINGLE_OS04A10:
        eSnsType = OMNIVISION_OS04A10;
        /* comm pool config */
        __cal_dump_pool(gtSysCommPoolSingleOs04a10Sdr, pVinParam->eHdrMode, pVinParam->nDumpFrameNum);
        pCommonArgs->nPoolCfgCnt = sizeof(gtSysCommPoolSingleOs04a10Sdr) / sizeof(gtSysCommPoolSingleOs04a10Sdr[0]);
        pCommonArgs->pPoolCfg = gtSysCommPoolSingleOs04a10Sdr;

        /* private pool config */
        __cal_dump_pool(gtPrivatePoolSingleOs04a10Sdr, pVinParam->eHdrMode, pVinParam->nDumpFrameNum);
        pPrivArgs->nPoolCfgCnt = sizeof(gtPrivatePoolSingleOs04a10Sdr) / sizeof(gtPrivatePoolSingleOs04a10Sdr[0]);
        pPrivArgs->pPoolCfg = gtPrivatePoolSingleOs04a10Sdr;

        /* cams config */
        __sample_case_single_os04a10(pCamList, eSnsType, pVinParam, pCommonArgs);
        break;
    case SAMPLE_VIN_SINGLE_SC450AI:
        eSnsType = SMARTSENS_SC450AI;
        /* comm pool config */
        __cal_dump_pool(gtSysCommPoolSingleOs450aiSdr, pVinParam->eHdrMode, pVinParam->nDumpFrameNum);
        pCommonArgs->nPoolCfgCnt = sizeof(gtSysCommPoolSingleOs450aiSdr) / sizeof(gtSysCommPoolSingleOs450aiSdr[0]);
        pCommonArgs->pPoolCfg = gtSysCommPoolSingleOs450aiSdr;

        /* private pool config */
        __cal_dump_pool(gtPrivatePoolSingleOs450aiSdr, pVinParam->eHdrMode, pVinParam->nDumpFrameNum);
        pPrivArgs->nPoolCfgCnt = sizeof(gtPrivatePoolSingleOs450aiSdr) / sizeof(gtPrivatePoolSingleOs450aiSdr[0]);
        pPrivArgs->pPoolCfg = gtPrivatePoolSingleOs450aiSdr;

        /* cams config */
        __sample_case_single_sc450ai(pCamList, eSnsType, pVinParam, pCommonArgs);
        break;
    // case SAMPLE_VIN_SINGLE_DUMMY:
    default:
        eSnsType = SAMPLE_SNS_DUMMY;
        /* pool config */
        pCommonArgs->nPoolCfgCnt = sizeof(gtSysCommPoolSingleDummySdr) / sizeof(gtSysCommPoolSingleDummySdr[0]);
        pCommonArgs->pPoolCfg = gtSysCommPoolSingleDummySdr;

        /* private pool config */
        pPrivArgs->nPoolCfgCnt = sizeof(gtPrivatePoolSingleDummySdr) / sizeof(gtPrivatePoolSingleDummySdr[0]);
        pPrivArgs->pPoolCfg = gtPrivatePoolSingleDummySdr;

        /* cams config */
        __sample_case_single_dummy(pCamList, eSnsType, pVinParam, pCommonArgs);
        break;
    }

    return 0;
}

AX_S32 SAMPLE_VIN_Init(SAMPLE_VIN_CASE_E eCase, int bAIISP_enable)
{
    AX_S32 s32Ret = 0;

    SAMPLE_VIN_PARAM_T tVinParam = {
        eCase,
        COMMON_VIN_SENSOR,
        AX_SNS_LINEAR_MODE,
        LOAD_RAW_NONE,
        bAIISP_enable,
        0,
    };

    /* Step1: cam config & pool Config */
    __sample_case_config(&tVinParam, &tCommonArgs, &tPrivArgs);
    // gCams[0].tSnsAttr.fFrameRate = 25.0f;
    /* Step2: SYS Init */
    s32Ret = COMMON_SYS_Init(&tCommonArgs);
    if (s32Ret)
    {
        ALOGE("COMMON_SYS_Init fail, ret:0x%x", s32Ret);
        return -1;
    }
    /* Step4: NPU Init */
    s32Ret = COMMON_NPU_Init();
    if (s32Ret)
    {
        ALOGE("COMMON_NPU_Init fail, ret:0x%x", s32Ret);
        return -1;
    }

    /* Step7: Cam Init */
    s32Ret = COMMON_CAM_Init();
    if (s32Ret)
    {
        ALOGE("COMMON_CAM_Init fail, ret:0x%x", s32Ret);
        return -1;
    }

    s32Ret = COMMON_CAM_PrivPoolInit(&tPrivArgs);
    if (s32Ret)
    {
        ALOGE("COMMON_CAM_PrivPoolInit fail, ret:0x%x", s32Ret);
        return -1;
    }
    return 0;
}
AX_S32 SAMPLE_VIN_Open()
{
    /* Step8: Cam Open */
    AX_S32 s32Ret = COMMON_CAM_Open(&gCams[0], tCommonArgs.nCamCnt);
    if (s32Ret)
    {
        ALOGE("COMMON_CAM_Open fail, ret:0x%x", s32Ret);
        return -1;
    }
    return 0;
}
AX_S32 SAMPLE_VIN_Start()
{
    // return COMMON_CAM_Run(&gCams[0], tCommonArgs.nCamCnt);
    return 0;
}
AX_S32 SAMPLE_VIN_Deinit()
{
    AX_S32 s32Ret = 0;
    // s32Ret = COMMON_CAM_Stop();
    // if (s32Ret)
    // {
    //     ALOGE("COMMON_CAM_Stop fail, ret:0x%x", s32Ret);
    //     return -1;
    // }
    s32Ret = COMMON_CAM_Close(&gCams[0], tCommonArgs.nCamCnt);
    if (s32Ret)
    {
        ALOGE("COMMON_CAM_Close fail, ret:0x%x", s32Ret);
        return -1;
    }
    s32Ret = COMMON_CAM_Deinit();
    if (s32Ret)
    {
        ALOGE("COMMON_CAM_Deinit fail, ret:0x%x", s32Ret);
        return -1;
    }
    return 0;
}
#endif