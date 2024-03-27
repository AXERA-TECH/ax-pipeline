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

typedef struct
{
    SAMPLE_VIN_CASE_E eSysCase;
    COMMON_VIN_MODE_E eSysMode;
    AX_SNS_HDR_MODE_E eHdrMode;
    AX_BOOL bAiispEnable;
    AX_S32 nDumpFrameNum;
} SAMPLE_VIN_PARAM_T;

/* comm pool */
COMMON_SYS_POOL_CFG_T gtSysCommPoolSingleDummySdr[] = {
    {3840, 2160, 3840, AX_FORMAT_YUV420_SEMIPLANAR, 10}, /* vin nv21/nv21 use */
};

COMMON_SYS_POOL_CFG_T gtSysCommPoolSingleOs08a20Sdr[] = {
    {3840, 2160, 3840, AX_FORMAT_YUV420_SEMIPLANAR, 12}, /* vin nv21/nv21 use */
};

COMMON_SYS_POOL_CFG_T gtSysCommPoolDoubleOs08a20Sdr[] = {
    {3840, 2160, 3840, AX_FORMAT_YUV420_SEMIPLANAR, 10 * 2}, /* vin nv21/nv21 use */
};

COMMON_SYS_POOL_CFG_T gtSysCommPoolDoubleOs08a20Hdr[] = {
    {3840, 2160, 3840, AX_FORMAT_YUV420_SEMIPLANAR, 50}, /* vin nv21/nv21 use */
};

COMMON_SYS_POOL_CFG_T gtSysCommPoolDoubleOs08a20MultiplePipeSdr[] = {
    {3840, 2160, 3840, AX_FORMAT_YUV420_SEMIPLANAR, 60}, /* vin nv21/nv21 use */
};

COMMON_SYS_POOL_CFG_T gtSysCommPoolDoubleOs08a20MultiplePipeHdr[] = {
    {3840, 2160, 3840, AX_FORMAT_YUV420_SEMIPLANAR, 60}, /* vin nv21/nv21 use */
};

COMMON_SYS_POOL_CFG_T gtSysCommPoolSingleSc910gsMultiplePipeSdr[] = {
    {3840, 2336, 3840, AX_FORMAT_YUV420_SEMIPLANAR, 80}, /* vin nv21/nv21 use */
};

COMMON_SYS_POOL_CFG_T gtSysCommPoolFourOs08a20Sdr[] = {
    {3840, 2160, 3840, AX_FORMAT_YUV420_SEMIPLANAR, 12 * 4}, /* vin nv21/nv21 use */
};

/* private pool */
COMMON_SYS_POOL_CFG_T gtPrivatePoolSingleDummySdr[] = {
    {3840, 2160, 3840, AX_FORMAT_BAYER_RAW_16BPP, 25}, /* vin raw16 use */
};

COMMON_SYS_POOL_CFG_T gtPrivatePoolSingleDummyUxeSdr[] = {
    {3840 * 3, 2160, 3840 * 3, AX_FORMAT_BAYER_RAW_8BPP, 25}, /* vin yuv420 use */
};

COMMON_SYS_POOL_CFG_T gtPrivatePoolSingleOs08a20Sdr[] = {
    {3840, 2160, 3840, AX_FORMAT_BAYER_RAW_16BPP, 25 * 2}, /* vin raw16 use */
};

COMMON_SYS_POOL_CFG_T gtPrivatePoolDoubleOs08a20Sdr[] = {
    {3840, 2160, 3840, AX_FORMAT_BAYER_RAW_16BPP, 25 * 2}, /* vin raw16 use */
};

COMMON_SYS_POOL_CFG_T gtPrivatePoolDoubleOs08a20Hdr[] = {
    {3840, 2160, 3840, AX_FORMAT_BAYER_RAW_16BPP, 25 * 2}, /* vin raw16 use */
};

COMMON_SYS_POOL_CFG_T gtPrivatePoolDoubleOs08a20MultiplePipeSdr[] = {
    {3840, 2160, 3840, AX_FORMAT_BAYER_RAW_16BPP, 45}, /* vin raw16 use */
};

COMMON_SYS_POOL_CFG_T gtPrivatePoolDoubleOs08a20MultiplePipeHdr[] = {
    {3840, 2160, 3840, AX_FORMAT_BAYER_RAW_16BPP, 65}, /* vin raw16 use */
};

COMMON_SYS_POOL_CFG_T gtPrivatePoolSingleSc910gsMultiplePipeSdr[] = {
    {3840, 2336, 3840, AX_FORMAT_BAYER_RAW_16BPP, 45}, /* vin raw16 use */
};

COMMON_SYS_POOL_CFG_T gtPrivatePoolFourOs08a20Sdr[] = {
    {3840, 2160, 3840, AX_FORMAT_BAYER_RAW_16BPP, 25 * 2}, /* vin raw16 use */
};

static AX_CAMERA_T gCams[MAX_CAMERAS] = {0};
static COMMON_SYS_ARGS_T tCommonArgs = {0};
static COMMON_SYS_ARGS_T tPrivArgs = {0};

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
    else
    {
        pCam->tSnsAttr.eRawType = AX_RT_RAW12;
        pCam->tDevAttr.ePixelFmt = AX_FORMAT_BAYER_RAW_12BPP;
        pCam->tPipeAttr.ePixelFmt = AX_FORMAT_BAYER_RAW_12BPP;
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

    pCam = &pCamList[0];
    pCommonArgs->nCamCnt = 1;

    for (i = 0; i < pCommonArgs->nCamCnt; i++)
    {
        pCam = &pCamList[i];
        COMMON_VIN_GetSnsConfig(eSnsType, &pCam->tMipiRx, &pCam->tSnsAttr,
                                &pCam->tSnsClkAttr, &pCam->tDevAttr,
                                &pCam->tPipeAttr, pCam->tChnAttr);
        for (AX_S32 j = 0; j < AX_VIN_MAX_PIPE_NUM; j++)
        {
            pCam->tPipeInfo[j].ePipeMode = SAMPLE_PIPE_MODE_VIDEO;
            pCam->tPipeInfo[j].bAiispEnable = pVinParam->bAiispEnable;
            strncpy(pCam->tPipeInfo[j].szBinPath, "null.bin", sizeof(pCam->tPipeInfo[j].szBinPath));
        }
    }

    return 0;
}
static AX_U32 __sample_case_single_os08a20(AX_CAMERA_T *pCamList, SAMPLE_SNS_TYPE_E eSnsType,
                                           SAMPLE_VIN_PARAM_T *pVinParam, COMMON_SYS_ARGS_T *pCommonArgs)
{
    AX_CAMERA_T *pCam = NULL;
    COMMON_VIN_MODE_E eSysMode = pVinParam->eSysMode;
    AX_SNS_HDR_MODE_E eHdrMode = pVinParam->eHdrMode;
    AX_S32 j = 0;
    pCommonArgs->nCamCnt = 1;
    pCam = &pCamList[0];
    COMMON_VIN_GetSnsConfig(eSnsType, &pCam->tMipiRx, &pCam->tSnsAttr,
                            &pCam->tSnsClkAttr, &pCam->tDevAttr,
                            &pCam->tPipeAttr, pCam->tChnAttr);
    pCam->nDevId = 0;
    pCam->nRxDev = 0;
    pCam->nPipeId = 0;
    pCam->tSnsClkAttr.nSnsClkIdx = 0;
    pCam->tDevBindPipe.nNum = 1;
    pCam->tDevBindPipe.nPipeId[0] = pCam->nPipeId;
    __set_pipe_hdr_mode(&pCam->tDevBindPipe.nHDRSel[0], eHdrMode);
    __set_vin_attr(pCam, eSnsType, eHdrMode, eSysMode, pVinParam->bAiispEnable);
    for (j = 0; j < pCam->tDevBindPipe.nNum; j++)
    {
        pCam->tPipeInfo[j].ePipeMode = SAMPLE_PIPE_MODE_VIDEO;
        pCam->tPipeInfo[j].bAiispEnable = pVinParam->bAiispEnable;
        if (pCam->tPipeInfo[j].bAiispEnable)
        {
            if (eHdrMode <= AX_SNS_LINEAR_MODE)
            {
                strncpy(pCam->tPipeInfo[j].szBinPath, "/opt/etc/os08a20_sdr_ai3d_t2dnr.bin", sizeof(pCam->tPipeInfo[j].szBinPath));
            }
            else
            {
                strncpy(pCam->tPipeInfo[j].szBinPath, "/opt/etc/os08a20_hdr_2x_ainr.bin", sizeof(pCam->tPipeInfo[j].szBinPath));
            }
        }
        else
        {
            strncpy(pCam->tPipeInfo[j].szBinPath, "null.bin", sizeof(pCam->tPipeInfo[j].szBinPath));
        }
    }
    return 0;
}

static AX_U32 __sample_case_double_os08a20(AX_CAMERA_T *pCamList, SAMPLE_SNS_TYPE_E eSnsType,
                                           SAMPLE_VIN_PARAM_T *pVinParam, COMMON_SYS_ARGS_T *pCommonArgs)
{
    AX_S32 i = 0, j = 0;
    AX_CAMERA_T *pCam = NULL;
    COMMON_VIN_MODE_E eSysMode = pVinParam->eSysMode;
    AX_SNS_HDR_MODE_E eHdrMode = pVinParam->eHdrMode;
    AX_CHAR apd_plate_id[BOARD_ID_LEN] = {0};

    pCommonArgs->nCamCnt = 2;
    COMMON_SYS_GetApdPlateId(apd_plate_id);

    for (i = 0; i < pCommonArgs->nCamCnt; i++)
    {
        pCam = &pCamList[i];
        COMMON_VIN_GetSnsConfig(eSnsType, &pCam->tMipiRx, &pCam->tSnsAttr,
                                &pCam->tSnsClkAttr, &pCam->tDevAttr,
                                &pCam->tPipeAttr, pCam->tChnAttr);
        if (i == 0)
        {
            pCam->nDevId = 0;
            pCam->nRxDev = 0;
            pCam->nPipeId = 0;
            pCam->tSnsClkAttr.nSnsClkIdx = 0;
        }
        else if (i == 1)
        {
            if (!strncmp(apd_plate_id, "ADP_RX_DPHY_2X4LANE", sizeof("ADP_RX_DPHY_2X4LANE") - 1))
            {
                if (!strncmp(apd_plate_id, "ADP_RX_DPHY_2X4LANE_N", sizeof("ADP_RX_DPHY_2X4LANE_N") - 1))
                {
                    pCam->nDevId = 2;
                    pCam->nRxDev = 2;
                    pCam->nPipeId = 1;
                    pCam->tSnsClkAttr.nSnsClkIdx = 1;
                }
                else
                {
                    pCam->nDevId = 4;
                    pCam->nRxDev = 4;
                    pCam->nPipeId = 1;
                    pCam->tSnsClkAttr.nSnsClkIdx = 1;
                }
            }
        }

        pCam->tDevBindPipe.nNum = 1;
        pCam->tDevBindPipe.nPipeId[0] = pCam->nPipeId;
        __set_pipe_hdr_mode(&pCam->tDevBindPipe.nHDRSel[0], eHdrMode);
        __set_vin_attr(pCam, eSnsType, eHdrMode, eSysMode, pVinParam->bAiispEnable);
        for (j = 0; j < pCam->tDevBindPipe.nNum; j++)
        {
            pCam->tPipeInfo[j].ePipeMode = SAMPLE_PIPE_MODE_VIDEO;
            pCam->tPipeInfo[j].bAiispEnable = pVinParam->bAiispEnable;
            if (pCam->tPipeInfo[j].bAiispEnable)
            {
                if (eHdrMode <= AX_SNS_LINEAR_MODE)
                {
                    strncpy(pCam->tPipeInfo[j].szBinPath, "/opt/etc/os08a20_sdr_ai3d_t2dnr.bin", sizeof(pCam->tPipeInfo[j].szBinPath));
                }
                else
                {
                    strncpy(pCam->tPipeInfo[j].szBinPath, "/opt/etc/os08a20_hdr_2x_ainr.bin", sizeof(pCam->tPipeInfo[j].szBinPath));
                }
            }
            else
            {
                strncpy(pCam->tPipeInfo[j].szBinPath, "null.bin", sizeof(pCam->tPipeInfo[j].szBinPath));
            }
            printf("j=%d szBinPath[%s]\n", j, pCam->tPipeInfo[j].szBinPath);
        }
    }

    return 0;
}

static AX_U32 __sample_case_double_os08a20_multiple_pipe(AX_CAMERA_T *pCamList, SAMPLE_SNS_TYPE_E eSnsType,
                                                         SAMPLE_VIN_PARAM_T *pVinParam, COMMON_SYS_ARGS_T *pCommonArgs)
{
    AX_U8 nPipeId = 0;
    AX_S32 i = 0, j = 0;
    AX_CAMERA_T *pCam = NULL;
    COMMON_VIN_MODE_E eSysMode = pVinParam->eSysMode;
    AX_SNS_HDR_MODE_E eHdrMode = pVinParam->eHdrMode;
    AX_CHAR apd_plate_id[BOARD_ID_LEN] = {0};
    SAMPLE_PIPE_INFO_T *pSamplePipeInfo = AX_NULL;

    SAMPLE_PIPE_INFO_T tSamplePipeInfo_sdr[] = {
        {SAMPLE_PIPE_MODE_VIDEO, AX_TRUE, "/opt/etc/os08a20_sdr_ai2dnr.bin"},            /* ai2dnr + t2dnr */
        {SAMPLE_PIPE_MODE_VIDEO, AX_TRUE, "/opt/etc/os08a20_sdr_ai3d_t2dnr.bin"},        /* ai3dnr + t3dnr */
        {SAMPLE_PIPE_MODE_VIDEO, AX_FALSE, "null.bin"},                                  /* t3dnr */
        {SAMPLE_PIPE_MODE_VIDEO, AX_FALSE, "/opt/etc/os08a20_sdr_t2dnr.bin"},            /* t2dnr */
        {SAMPLE_PIPE_MODE_VIDEO, AX_TRUE, "/opt/etc/os08a20_sdr_ai3d_t2dnr.bin"},        /* ai3dnr + t2dnr */
        {SAMPLE_PIPE_MODE_VIDEO, AX_TRUE, "/opt/etc/os08a20_sdr_ai3d-t2d_to_t3dnr.bin"}, /* ai3d_t2dnr to t3dnr */
    };
    SAMPLE_PIPE_INFO_T tSamplePipeInfo_hdr_2x[] = {
        {SAMPLE_PIPE_MODE_VIDEO, AX_FALSE, "null.bin"},                                /* t3dnr */
        {SAMPLE_PIPE_MODE_VIDEO, AX_TRUE, "/opt/etc/os08a20_hdr_2x_aice.bin"},         /* ai3dnr + t2dnr */
        {SAMPLE_PIPE_MODE_VIDEO, AX_TRUE, "/opt/etc/os08a20_hdr_2x_aice_to_ainr.bin"}, /* aice to ainr */
        {SAMPLE_PIPE_MODE_VIDEO, AX_TRUE, "/opt/etc/os08a20_hdr_2x_ainr.bin"},         /* ai3dnr + t2dnr */
        {SAMPLE_PIPE_MODE_VIDEO, AX_TRUE, "/opt/etc/os08a20_hdr_2x_aice.bin"},         /* aice + t2dnr */
        {SAMPLE_PIPE_MODE_VIDEO, AX_FALSE, "null.bin"},                                /* t2dnr */
    };

    if (eHdrMode == AX_SNS_LINEAR_MODE)
    {
        pSamplePipeInfo = &tSamplePipeInfo_sdr[0];
    }
    else
    {
        pSamplePipeInfo = &tSamplePipeInfo_hdr_2x[0];
    }

    pCommonArgs->nCamCnt = 2;
    COMMON_SYS_GetApdPlateId(apd_plate_id);

    for (i = 0; i < pCommonArgs->nCamCnt; i++)
    {
        pCam = &pCamList[i];
        COMMON_VIN_GetSnsConfig(eSnsType, &pCam->tMipiRx, &pCam->tSnsAttr,
                                &pCam->tSnsClkAttr, &pCam->tDevAttr,
                                &pCam->tPipeAttr, pCam->tChnAttr);
        if (i == 0)
        {
            pCam->nDevId = 0;
            pCam->nRxDev = 0;
            pCam->tSnsClkAttr.nSnsClkIdx = 0;

            pCam->tDevBindPipe.nNum = 3;
            pCam->tDevBindPipe.nPipeId[0] = 0;
            pCam->tDevBindPipe.nPipeId[1] = 1;
            pCam->tDevBindPipe.nPipeId[2] = 2;
        }
        else if (i == 1)
        {
            if (!strncmp(apd_plate_id, "ADP_RX_DPHY_2X4LANE", sizeof("ADP_RX_DPHY_2X4LANE") - 1))
            {
                if (!strncmp(apd_plate_id, "ADP_RX_DPHY_2X4LANE_N", sizeof("ADP_RX_DPHY_2X4LANE_N") - 1))
                {
                    pCam->nDevId = 2;
                    pCam->nRxDev = 2;
                    pCam->nPipeId = 1;
                    pCam->tSnsClkAttr.nSnsClkIdx = 1;
                }
                else
                {
                    pCam->nDevId = 4;
                    pCam->nRxDev = 4;
                    pCam->nPipeId = 1;
                    pCam->tSnsClkAttr.nSnsClkIdx = 1;
                }
            }

            pCam->tDevBindPipe.nNum = 3;
            pCam->tDevBindPipe.nPipeId[0] = 3;
            pCam->tDevBindPipe.nPipeId[1] = 4;
            pCam->tDevBindPipe.nPipeId[2] = 5;
        }
        __set_pipe_hdr_mode(&pCam->tDevBindPipe.nHDRSel[0], eHdrMode);
        __set_pipe_hdr_mode(&pCam->tDevBindPipe.nHDRSel[1], eHdrMode);
        __set_pipe_hdr_mode(&pCam->tDevBindPipe.nHDRSel[2], eHdrMode);
        __set_vin_attr(pCam, eSnsType, eHdrMode, eSysMode, pVinParam->bAiispEnable);
        for (j = 0; j < pCam->tDevBindPipe.nNum; j++)
        {
            nPipeId = pCam->tDevBindPipe.nPipeId[j];
            pCam->tPipeInfo[j].ePipeMode = (pSamplePipeInfo + nPipeId)->ePipeMode;
            pCam->tPipeInfo[j].bAiispEnable = (pSamplePipeInfo + nPipeId)->bAiispEnable;
            strncpy(pCam->tPipeInfo[j].szBinPath, (pSamplePipeInfo + nPipeId)->szBinPath, sizeof(pCam->tPipeInfo[j].szBinPath));
        }
    }

    return 0;
}

static AX_U32 __sample_case_single_os08a20_its_capture(AX_CAMERA_T *pCamList, SAMPLE_SNS_TYPE_E eSnsType,
                                                       SAMPLE_VIN_PARAM_T *pVinParam, COMMON_SYS_ARGS_T *pCommonArgs)
{
    AX_S32 i = 0, j = 0;
    AX_CAMERA_T *pCam = NULL;
    COMMON_VIN_MODE_E eSysMode = pVinParam->eSysMode;
    AX_SNS_HDR_MODE_E eHdrMode = pVinParam->eHdrMode;
    pCommonArgs->nCamCnt = 1;
    SAMPLE_PIPE_INFO_T tSamplePipeInfo[] = {
        {SAMPLE_PIPE_MODE_VIDEO, AX_FALSE, "null.bin"},
        {SAMPLE_PIPE_MODE_VIDEO, AX_FALSE, "/opt/etc/os08a20_sdr_t2dnr.bin"},
        {SAMPLE_PIPE_MODE_FLASH_SNAP, AX_TRUE, "/opt/etc/os08a20_sdr_ai2dnr.bin"},
    };

    for (i = 0; i < pCommonArgs->nCamCnt; i++)
    {
        pCam = &pCamList[i];
        COMMON_VIN_GetSnsConfig(eSnsType, &pCam->tMipiRx, &pCam->tSnsAttr,
                                &pCam->tSnsClkAttr, &pCam->tDevAttr,
                                &pCam->tPipeAttr, pCam->tChnAttr);
        if (i == 0)
        {
            pCam->nDevId = 0;
            pCam->nRxDev = 0;
            pCam->tSnsClkAttr.nSnsClkIdx = 0;

            pCam->tDevBindPipe.nNum = 3;
            pCam->tDevBindPipe.nPipeId[0] = 0;
            pCam->tDevBindPipe.nPipeId[1] = 1;
            pCam->tDevBindPipe.nPipeId[2] = 2;
        }

        __set_pipe_hdr_mode(&pCam->tDevBindPipe.nHDRSel[0], eHdrMode);
        __set_pipe_hdr_mode(&pCam->tDevBindPipe.nHDRSel[1], eHdrMode);
        __set_pipe_hdr_mode(&pCam->tDevBindPipe.nHDRSel[2], eHdrMode);
        __set_vin_attr(pCam, eSnsType, eHdrMode, eSysMode, pVinParam->bAiispEnable);
        for (j = 0; j < pCam->tDevBindPipe.nNum; j++)
        {
            pCam->tPipeInfo[j].ePipeMode = tSamplePipeInfo[j].ePipeMode;
            pCam->tPipeInfo[j].bAiispEnable = tSamplePipeInfo[j].bAiispEnable;
            strncpy(pCam->tPipeInfo[j].szBinPath, tSamplePipeInfo[j].szBinPath, sizeof(pCam->tPipeInfo[j].szBinPath));
        }
    }
    return 0;
}

static AX_U32 __sample_case_single_uxe_capture(AX_CAMERA_T *pCamList, SAMPLE_SNS_TYPE_E eSnsType,
                                               SAMPLE_VIN_PARAM_T *pVinParam, COMMON_SYS_ARGS_T *pCommonArgs)
{
    AX_CAMERA_T *pCam = NULL;
    COMMON_VIN_MODE_E eSysMode = pVinParam->eSysMode;
    AX_SNS_HDR_MODE_E eHdrMode = pVinParam->eHdrMode;
    pCommonArgs->nCamCnt = 1;

    pCam = &pCamList[0];
    COMMON_VIN_GetSnsConfig(eSnsType, &pCam->tMipiRx, &pCam->tSnsAttr,
                            &pCam->tSnsClkAttr, &pCam->tDevAttr,
                            &pCam->tPipeAttr, pCam->tChnAttr);
    /*yuv422*/
    pCam->tMipiRx.tMipiAttr.nDataRate = 1900;
    pCam->tMipiRx.tMipiAttr.eLaneNum = 8;
    pCam->tDevAttr.eSnsIntfType = AX_SNS_INTF_TYPE_MIPI_RAW;
    pCam->tDevAttr.ePixelFmt = AX_FORMAT_BAYER_RAW_8BPP;

    pCam->bDevOnly = AX_TRUE;
    pCam->nDevId = 0;
    pCam->nRxDev = 0;
    pCam->nPipeId = 0;
    pCam->tSnsClkAttr.nSnsClkIdx = 0;
    pCam->tSnsClkAttr.eSnsClkRate = AX_SNS_CLK_24M;
    pCam->tDevBindPipe.nNum = 1;
    pCam->tDevBindPipe.nPipeId[0] = pCam->nPipeId;
    __set_pipe_hdr_mode(&pCam->tDevBindPipe.nHDRSel[0], eHdrMode);
    __set_vin_attr(pCam, eSnsType, eHdrMode, eSysMode, pVinParam->bAiispEnable);

    pCam->tSnsAttr.eRawType = AX_RT_RAW8;
    pCam->tDevAttr.ePixelFmt = AX_FORMAT_BAYER_RAW_8BPP;
    pCam->tDevAttr.eDevWorkMode = AX_VIN_DEV_WORK_MODE_4MULTIPLEX; /* hot tip: config 4 ROI */

    /* ROI config */
    pCam->tDevAttr.tDevImgRgn[0].nStartX = 0;
    pCam->tDevAttr.tDevImgRgn[0].nStartY = 0;
    pCam->tDevAttr.tDevImgRgn[0].nWidth = 3840 * 3 / 2;
    pCam->tDevAttr.tDevImgRgn[0].nHeight = 2160 / 2;
    pCam->tDevAttr.nWidthStride[0] = 3840 * 3;

    pCam->tDevAttr.tDevImgRgn[1].nStartX = 3840 * 3 / 2;
    pCam->tDevAttr.tDevImgRgn[1].nStartY = 0;
    pCam->tDevAttr.tDevImgRgn[1].nWidth = 3840 * 3 / 2;
    pCam->tDevAttr.tDevImgRgn[1].nHeight = 2160 / 2;
    pCam->tDevAttr.nWidthStride[1] = 3840 * 3;

    pCam->tDevAttr.tDevImgRgn[2].nStartX = 0;
    pCam->tDevAttr.tDevImgRgn[2].nStartY = 2160 / 2;
    pCam->tDevAttr.tDevImgRgn[2].nWidth = 3840 * 3 / 2;
    pCam->tDevAttr.tDevImgRgn[2].nHeight = 2160 / 2;
    pCam->tDevAttr.nWidthStride[2] = 3840 * 3;

    pCam->tDevAttr.tDevImgRgn[3].nStartX = 3840 * 3 / 2;
    pCam->tDevAttr.tDevImgRgn[3].nStartY = 2160 / 2;
    pCam->tDevAttr.tDevImgRgn[3].nWidth = 3840 * 3 / 2;
    pCam->tDevAttr.tDevImgRgn[3].nHeight = 2160 / 2;
    pCam->tDevAttr.nWidthStride[3] = 3840 * 3;

    /* configure the attribute of early reporting of frame interrupts */
    pCam->tDevFrmIntAttr.bImgRgnIntEn[0] = AX_FALSE;
    pCam->tDevFrmIntAttr.bImgRgnIntEn[1] = AX_FALSE;
    pCam->tDevFrmIntAttr.bImgRgnIntEn[2] = AX_FALSE;
    pCam->tDevFrmIntAttr.bImgRgnIntEn[3] = AX_TRUE;

    pCam->bRegisterSns = AX_FALSE;

    return 0;
}

static AX_U32 __sample_case_single_sc910gs_its_capture(AX_CAMERA_T *pCamList, SAMPLE_SNS_TYPE_E eSnsType,
                                                       SAMPLE_VIN_PARAM_T *pVinParam, COMMON_SYS_ARGS_T *pCommonArgs)
{

    AX_S32 i = 0, j = 0;
    AX_CAMERA_T *pCam = NULL;
    COMMON_VIN_MODE_E eSysMode = pVinParam->eSysMode;
    AX_SNS_HDR_MODE_E eHdrMode = pVinParam->eHdrMode;
    pCommonArgs->nCamCnt = 1;
    SAMPLE_PIPE_INFO_T tSamplePipeInfo[] = {
        {SAMPLE_PIPE_MODE_VIDEO, AX_FALSE, "null.bin"},
        {SAMPLE_PIPE_MODE_VIDEO, AX_FALSE, "/opt/etc/sc910gs_sdr_t2dnr.bin"},
        {SAMPLE_PIPE_MODE_FLASH_SNAP, AX_TRUE, "/opt/etc/sc910gs_sdr_ai2dnr.bin"},
    };

    for (i = 0; i < pCommonArgs->nCamCnt; i++)
    {
        pCam = &pCamList[i];
        COMMON_VIN_GetSnsConfig(eSnsType, &pCam->tMipiRx, &pCam->tSnsAttr,
                                &pCam->tSnsClkAttr, &pCam->tDevAttr,
                                &pCam->tPipeAttr, pCam->tChnAttr);
        pCam->bEnableFlash = AX_TRUE;
        COMMON_VIN_GetOutsideConfig(eSnsType, &pCam->tPowerAttr,
                                    &pCam->tVsyncAttr, &pCam->tHsyncAttr,
                                    &pCam->tLightSyncInfo, &pCam->tSnapStrobeAttr,
                                    &pCam->tSnapFlashAttr);

        if (i == 0)
        {
            pCam->nDevId = 0;
            pCam->nRxDev = 0;
            pCam->tSnsClkAttr.nSnsClkIdx = 0;

            pCam->tDevBindPipe.nNum = 3;
            pCam->tDevBindPipe.nPipeId[0] = 0;
            pCam->tDevBindPipe.nPipeId[1] = 1;
            pCam->tDevBindPipe.nPipeId[2] = 2;
        }

        __set_pipe_hdr_mode(&pCam->tDevBindPipe.nHDRSel[0], eHdrMode);
        __set_pipe_hdr_mode(&pCam->tDevBindPipe.nHDRSel[1], eHdrMode);
        __set_pipe_hdr_mode(&pCam->tDevBindPipe.nHDRSel[2], eHdrMode);
        __set_vin_attr(pCam, eSnsType, eHdrMode, eSysMode, pVinParam->bAiispEnable);
        pCam->tSnsAttr.eRawType = AX_RT_RAW10;
        pCam->tDevAttr.ePixelFmt = AX_FORMAT_BAYER_RAW_10BPP;
        pCam->tPipeAttr.ePixelFmt = AX_FORMAT_BAYER_RAW_10BPP;
        for (j = 0; j < pCam->tDevBindPipe.nNum; j++)
        {
            pCam->tPipeInfo[j].ePipeMode = tSamplePipeInfo[j].ePipeMode;
            pCam->tPipeInfo[j].bAiispEnable = tSamplePipeInfo[j].bAiispEnable;
            strncpy(pCam->tPipeInfo[j].szBinPath, tSamplePipeInfo[j].szBinPath, sizeof(pCam->tPipeInfo[j].szBinPath));
        }
    }

    return 0;
}

static AX_U32 __sample_case_four_os08a20(AX_CAMERA_T *pCamList, SAMPLE_SNS_TYPE_E eSnsType,
                                         SAMPLE_VIN_PARAM_T *pVinParam, COMMON_SYS_ARGS_T *pCommonArgs)
{
    AX_S32 i = 0, j = 0;
    AX_CAMERA_T *pCam = NULL;
    COMMON_VIN_MODE_E eSysMode = pVinParam->eSysMode;
    AX_SNS_HDR_MODE_E eHdrMode = pVinParam->eHdrMode;
    AX_CHAR apd_plate_id[BOARD_ID_LEN] = {0};

    pCommonArgs->nCamCnt = 4;
    COMMON_SYS_GetApdPlateId(apd_plate_id);

    for (i = 0; i < pCommonArgs->nCamCnt; i++)
    {
        pCam = &pCamList[i];
        COMMON_VIN_GetSnsConfig(eSnsType, &pCam->tMipiRx, &pCam->tSnsAttr,
                                &pCam->tSnsClkAttr, &pCam->tDevAttr,
                                &pCam->tPipeAttr, pCam->tChnAttr);

        pCam->nDevId = 0 + (i * 2);
        pCam->nRxDev = 0 + (i * 2);
        pCam->nPipeId = 0 + i;
        pCam->tSnsClkAttr.nSnsClkIdx = 0 + (i / 2);

        pCam->tDevBindPipe.nNum = 1;
        pCam->tDevBindPipe.nPipeId[0] = pCam->nPipeId;
        __set_pipe_hdr_mode(&pCam->tDevBindPipe.nHDRSel[0], eHdrMode);
        __set_vin_attr(pCam, eSnsType, eHdrMode, eSysMode, pVinParam->bAiispEnable);
        for (j = 0; j < pCam->tDevBindPipe.nNum; j++)
        {
            pCam->tPipeInfo[j].ePipeMode = SAMPLE_PIPE_MODE_VIDEO;
            pCam->tPipeInfo[j].bAiispEnable = pVinParam->bAiispEnable;
            if (pCam->tPipeInfo[j].bAiispEnable)
            {
                if (eHdrMode <= AX_SNS_LINEAR_MODE)
                {
                    strncpy(pCam->tPipeInfo[j].szBinPath, "/opt/etc/os08a20_sdr_ai3d_t2dnr.bin", sizeof(pCam->tPipeInfo[j].szBinPath));
                }
                else
                {
                    strncpy(pCam->tPipeInfo[j].szBinPath, "/opt/etc/os08a20_hdr_2x_ainr.bin", sizeof(pCam->tPipeInfo[j].szBinPath));
                }
            }
            else
            {
                strncpy(pCam->tPipeInfo[j].szBinPath, "null.bin", sizeof(pCam->tPipeInfo[j].szBinPath));
            }
        }
    }

    return 0;
}

static AX_U32 __sample_case_config(SAMPLE_VIN_PARAM_T *pVinParam, COMMON_SYS_ARGS_T *pCommonArgs, COMMON_SYS_ARGS_T *pPrivArgs)
{
    AX_CAMERA_T *pCamList = &gCams[0];
    SAMPLE_SNS_TYPE_E eSnsType = OMNIVISION_OS08A20;

    COMM_ISP_PRT("eSysCase %d, eSysMode %d, eHdrMode %d, bAiispEnable %d\n", pVinParam->eSysCase, pVinParam->eSysMode,
                 pVinParam->eHdrMode, pVinParam->bAiispEnable);

    switch (pVinParam->eSysCase)
    {
    case SAMPLE_VIN_SINGLE_OS08A20:
        eSnsType = OMNIVISION_OS08A20;
        /* comm pool config */
        __cal_dump_pool(gtSysCommPoolSingleOs08a20Sdr, pVinParam->eHdrMode, pVinParam->nDumpFrameNum);
        pCommonArgs->nPoolCfgCnt = sizeof(gtSysCommPoolSingleOs08a20Sdr) / sizeof(gtSysCommPoolSingleOs08a20Sdr[0]);
        pCommonArgs->pPoolCfg = gtSysCommPoolSingleOs08a20Sdr;

        /* private pool config */
        __cal_dump_pool(gtPrivatePoolSingleOs08a20Sdr, pVinParam->eHdrMode, pVinParam->nDumpFrameNum);
        pPrivArgs->nPoolCfgCnt = sizeof(gtPrivatePoolSingleOs08a20Sdr) / sizeof(gtPrivatePoolSingleOs08a20Sdr[0]);
        pPrivArgs->pPoolCfg = gtPrivatePoolSingleOs08a20Sdr;

        /* cams config */
        __sample_case_single_os08a20(pCamList, eSnsType, pVinParam, pCommonArgs);
        break;
    case SAMPLE_VIN_DOUBLE_OS08A20:
        eSnsType = OMNIVISION_OS08A20;
        /* pool config */
        if (AX_SNS_LINEAR_MODE == pVinParam->eHdrMode)
        {
            __cal_dump_pool(gtSysCommPoolDoubleOs08a20Sdr, pVinParam->eHdrMode, pVinParam->nDumpFrameNum);
            pCommonArgs->nPoolCfgCnt = sizeof(gtSysCommPoolDoubleOs08a20Sdr) / sizeof(gtSysCommPoolDoubleOs08a20Sdr[0]);
            pCommonArgs->pPoolCfg = gtSysCommPoolDoubleOs08a20Sdr;
        }
        else
        {
            __cal_dump_pool(gtSysCommPoolDoubleOs08a20Hdr, pVinParam->eHdrMode, pVinParam->nDumpFrameNum);
            pCommonArgs->nPoolCfgCnt = sizeof(gtSysCommPoolDoubleOs08a20Hdr) / sizeof(gtSysCommPoolDoubleOs08a20Hdr[0]);
            pCommonArgs->pPoolCfg = gtSysCommPoolDoubleOs08a20Hdr;
        }

        /* private pool config */
        if (AX_SNS_LINEAR_MODE == pVinParam->eHdrMode)
        {
            __cal_dump_pool(gtPrivatePoolDoubleOs08a20Sdr, pVinParam->eHdrMode, pVinParam->nDumpFrameNum);
            pPrivArgs->nPoolCfgCnt = sizeof(gtPrivatePoolDoubleOs08a20Sdr) / sizeof(gtPrivatePoolDoubleOs08a20Sdr[0]);
            pPrivArgs->pPoolCfg = gtPrivatePoolDoubleOs08a20Sdr;
        }
        else
        {
            __cal_dump_pool(gtPrivatePoolDoubleOs08a20Hdr, pVinParam->eHdrMode, pVinParam->nDumpFrameNum);
            pPrivArgs->nPoolCfgCnt = sizeof(gtPrivatePoolDoubleOs08a20Hdr) / sizeof(gtPrivatePoolDoubleOs08a20Hdr[0]);
            pPrivArgs->pPoolCfg = gtPrivatePoolDoubleOs08a20Hdr;
        }

        /* cams config */
        __sample_case_double_os08a20(pCamList, eSnsType, pVinParam, pCommonArgs);
        break;
    case SAMPLE_VIN_DOUBLE_OS08A20_MULTIPLE_PIPE:
        eSnsType = OMNIVISION_OS08A20;
        /* pool config */
        if (AX_SNS_LINEAR_MODE == pVinParam->eHdrMode)
        {
            __cal_dump_pool(gtSysCommPoolDoubleOs08a20MultiplePipeSdr, pVinParam->eHdrMode, pVinParam->nDumpFrameNum);
            pCommonArgs->nPoolCfgCnt = sizeof(gtSysCommPoolDoubleOs08a20MultiplePipeSdr) /
                                       sizeof(gtSysCommPoolDoubleOs08a20MultiplePipeSdr[0]);
            pCommonArgs->pPoolCfg = gtSysCommPoolDoubleOs08a20MultiplePipeSdr;
        }
        else
        {
            __cal_dump_pool(gtSysCommPoolDoubleOs08a20MultiplePipeHdr, pVinParam->eHdrMode, pVinParam->nDumpFrameNum);
            pCommonArgs->nPoolCfgCnt = sizeof(gtSysCommPoolDoubleOs08a20MultiplePipeHdr) /
                                       sizeof(gtSysCommPoolDoubleOs08a20MultiplePipeHdr[0]);
            pCommonArgs->pPoolCfg = gtSysCommPoolDoubleOs08a20MultiplePipeHdr;
        }

        /* private pool config */
        if (AX_SNS_LINEAR_MODE == pVinParam->eHdrMode)
        {
            __cal_dump_pool(gtPrivatePoolDoubleOs08a20MultiplePipeSdr, pVinParam->eHdrMode, pVinParam->nDumpFrameNum);
            pPrivArgs->nPoolCfgCnt = sizeof(gtPrivatePoolDoubleOs08a20MultiplePipeSdr) /
                                     sizeof(gtPrivatePoolDoubleOs08a20MultiplePipeSdr[0]);
            pPrivArgs->pPoolCfg = gtPrivatePoolDoubleOs08a20MultiplePipeSdr;
        }
        else
        {
            __cal_dump_pool(gtPrivatePoolDoubleOs08a20MultiplePipeHdr, pVinParam->eHdrMode, pVinParam->nDumpFrameNum);
            pPrivArgs->nPoolCfgCnt = sizeof(gtPrivatePoolDoubleOs08a20MultiplePipeHdr) /
                                     sizeof(gtPrivatePoolDoubleOs08a20MultiplePipeHdr[0]);
            pPrivArgs->pPoolCfg = gtPrivatePoolDoubleOs08a20MultiplePipeHdr;
        }

        /* cams config */
        __sample_case_double_os08a20_multiple_pipe(pCamList, eSnsType, pVinParam, pCommonArgs);
        break;
    case SAMPLE_VIN_SINGLE_OS08A20_ITS_CAPTURE:
        eSnsType = OMNIVISION_OS08A20;
        /* pool config */
        if (AX_SNS_LINEAR_MODE == pVinParam->eHdrMode)
        {
            __cal_dump_pool(gtSysCommPoolDoubleOs08a20MultiplePipeSdr, pVinParam->eHdrMode, pVinParam->nDumpFrameNum);
            pCommonArgs->nPoolCfgCnt = sizeof(gtSysCommPoolDoubleOs08a20MultiplePipeSdr) /
                                       sizeof(gtSysCommPoolDoubleOs08a20MultiplePipeSdr[0]);
            pCommonArgs->pPoolCfg = gtSysCommPoolDoubleOs08a20MultiplePipeSdr;
        }

        /* private pool config */
        if (AX_SNS_LINEAR_MODE == pVinParam->eHdrMode)
        {
            __cal_dump_pool(gtPrivatePoolDoubleOs08a20MultiplePipeSdr, pVinParam->eHdrMode, pVinParam->nDumpFrameNum);
            pPrivArgs->nPoolCfgCnt = sizeof(gtPrivatePoolDoubleOs08a20MultiplePipeSdr) /
                                     sizeof(gtPrivatePoolDoubleOs08a20MultiplePipeSdr[0]);
            pPrivArgs->pPoolCfg = gtPrivatePoolDoubleOs08a20MultiplePipeSdr;
        }

        /* cams config */
        __sample_case_single_os08a20_its_capture(pCamList, eSnsType, pVinParam, pCommonArgs);
        break;
    case SAMPLE_VIN_SINGLE_YUV422:
        eSnsType = SAMPLE_SNS_DUMMY;

        /* private pool config */
        __cal_dump_pool(gtPrivatePoolSingleDummyUxeSdr, pVinParam->eHdrMode, pVinParam->nDumpFrameNum);
        pPrivArgs->nPoolCfgCnt = sizeof(gtPrivatePoolSingleDummyUxeSdr) / sizeof(gtPrivatePoolSingleDummyUxeSdr[0]);
        pPrivArgs->pPoolCfg = gtPrivatePoolSingleDummyUxeSdr;

        /* cams config */
        __sample_case_single_uxe_capture(pCamList, eSnsType, pVinParam, pCommonArgs);
        break;
    case SAMPLE_VIN_FOUR_OS08A20:
        eSnsType = OMNIVISION_OS08A20;
        /* comm pool config */
        __cal_dump_pool(gtSysCommPoolFourOs08a20Sdr, pVinParam->eHdrMode, pVinParam->nDumpFrameNum);
        pCommonArgs->nPoolCfgCnt = sizeof(gtSysCommPoolFourOs08a20Sdr) / sizeof(gtSysCommPoolFourOs08a20Sdr[0]);
        pCommonArgs->pPoolCfg = gtSysCommPoolFourOs08a20Sdr;

        /* private pool config */
        __cal_dump_pool(gtPrivatePoolFourOs08a20Sdr, pVinParam->eHdrMode, pVinParam->nDumpFrameNum);
        pPrivArgs->nPoolCfgCnt = sizeof(gtPrivatePoolFourOs08a20Sdr) / sizeof(gtPrivatePoolFourOs08a20Sdr[0]);
        pPrivArgs->pPoolCfg = gtPrivatePoolFourOs08a20Sdr;

        /* cams config */
        __sample_case_four_os08a20(pCamList, eSnsType, pVinParam, pCommonArgs);
        break;
    case SAMPLE_VIN_SINGLE_SC910GS_ITS_CAPTURE:
        eSnsType = SMARTSENS_SC910GS;
        /* pool config */
        if (AX_SNS_LINEAR_MODE == pVinParam->eHdrMode)
        {
            __cal_dump_pool(gtSysCommPoolSingleSc910gsMultiplePipeSdr, pVinParam->eHdrMode, pVinParam->nDumpFrameNum);
            pCommonArgs->nPoolCfgCnt = sizeof(gtSysCommPoolSingleSc910gsMultiplePipeSdr) /
                                       sizeof(gtSysCommPoolSingleSc910gsMultiplePipeSdr[0]);
            pCommonArgs->pPoolCfg = gtSysCommPoolSingleSc910gsMultiplePipeSdr;
        }

        /* private pool config */
        if (AX_SNS_LINEAR_MODE == pVinParam->eHdrMode)
        {
            __cal_dump_pool(gtPrivatePoolSingleSc910gsMultiplePipeSdr, pVinParam->eHdrMode, pVinParam->nDumpFrameNum);
            pPrivArgs->nPoolCfgCnt = sizeof(gtPrivatePoolSingleSc910gsMultiplePipeSdr) /
                                     sizeof(gtPrivatePoolSingleSc910gsMultiplePipeSdr[0]);
            pPrivArgs->pPoolCfg = gtPrivatePoolSingleSc910gsMultiplePipeSdr;
        }

        /* cams config */
        __sample_case_single_sc910gs_its_capture(pCamList, eSnsType, pVinParam, pCommonArgs);
        break;
    case SAMPLE_VIN_SINGLE_DUMMY:
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

AX_S32 SAMPLE_VIN_Init()
{
    AX_S32 axRet = 0;

    SAMPLE_VIN_PARAM_T tVinParam = {
        SAMPLE_VIN_SINGLE_OS08A20,
        COMMON_VIN_SENSOR,
        AX_SNS_LINEAR_MODE,
        AX_FALSE,
        0,
    };

    /* Step1: cam config & pool Config */
    __sample_case_config(&tVinParam, &tCommonArgs, &tPrivArgs);

    /* Step2: SYS Init */
    axRet = COMMON_SYS_Init(&tCommonArgs);
    if (axRet)
    {
        COMM_ISP_PRT("COMMON_SYS_Init fail, ret:0x%x", axRet);
        return -1;
    }
    /* Step3: NPU Init */
    axRet = COMMON_NPU_Init(AX_ENGINE_VIRTUAL_NPU_BIG_LITTLE);
    if (axRet)
    {
        COMM_ISP_PRT("COMMON_NPU_Init fail, ret:0x%x", axRet);
        return -1;
    }
    /* Step4: Cam Init */
    axRet = COMMON_CAM_Init();
    if (axRet)
    {
        COMM_ISP_PRT("COMMON_CAM_Init fail, ret:0x%x", axRet);
        return -1;
    }
    axRet = COMMON_CAM_PrivPoolInit(&tPrivArgs);
    if (axRet)
    {
        COMM_ISP_PRT("COMMON_CAM_PrivPoolInit fail, ret:0x%x", axRet);
        return -1;
    }
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
    /* Step7: Cam Run */
    AX_S32 axRet = COMMON_CAM_Run(&gCams[0], tCommonArgs.nCamCnt);
    if (axRet)
    {
        COMM_ISP_PRT("COMMON_CAM_Open fail, ret:0x%x", axRet);
        return -1;
    }
    return 0;
}
AX_S32 SAMPLE_VIN_Deinit()
{
    AX_S32 s32Ret = COMMON_CAM_Stop();
    if (s32Ret)
    {
        ALOGE("COMMON_CAM_Stop fail, ret:0x%x", s32Ret);
        return -1;
    }
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
    AX_S32 nPipeId; /* For VIN */
    AX_S32 nGrpId;  /* For IVPS */
    AX_S32 nOutChnNum;
    char *pFrameInfo;
    AX_VIN_IVPS_MODE_E eMode;
    AX_U32 statDeltaPtsFrmNum;
} SAMPLE_VIN_PARAM_T;

/* comm pool */
COMMON_SYS_POOL_CFG_T gtSysCommPoolSingleDummySdr[] = {
    {2688, 1520, 2688, AX_FORMAT_BAYER_RAW_16BPP, 10},                             /* vin raw16 use */
    {2688, 1520, 2688, AX_FORMAT_YUV420_SEMIPLANAR, 10},                           /* vin nv21/nv21 use */
    {2688, 1520, 2688, AX_FORMAT_YUV420_SEMIPLANAR, 3, AX_COMPRESS_MODE_LOSSY, 4}, /* vin nv21/nv21 use */
};

COMMON_SYS_POOL_CFG_T gtSysCommPoolSingleOs04a10Sdr[] = {
    {2688, 1520, 2688, AX_FORMAT_YUV420_SEMIPLANAR, 3, AX_COMPRESS_MODE_LOSSY, 4}, /* vin nv21/nv21 use */
    {2688, 1520, 2688, AX_FORMAT_YUV420_SEMIPLANAR, 4},                            /* vin nv21/nv21 use */
    {1920, 1080, 1920, AX_FORMAT_YUV420_SEMIPLANAR, 3},                            /* vin nv21/nv21 use */
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
    {2688, 1520, 2688, AX_FORMAT_YUV420_SEMIPLANAR, 3, AX_COMPRESS_MODE_LOSSY, 4}, /* vin nv21/nv21 use */
    {2688, 1520, 2688, AX_FORMAT_YUV420_SEMIPLANAR, 4},                            /* vin nv21/nv21 use */
    {1920, 1080, 1920, AX_FORMAT_YUV420_SEMIPLANAR, 3},                            /* vin nv21/nv21 use */
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
        .statDeltaPtsFrmNum = 0,
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

    AX_ENGINE_NPU_ATTR_T npu_attr;
    memset(&npu_attr, 0, sizeof(npu_attr));
    npu_attr.eHardMode = AX_ENGINE_VIRTUAL_NPU_DISABLE;
    s32Ret = AX_ENGINE_Init(&npu_attr);
    if (0 != s32Ret)
    {
        ALOGE("AX_ENGINE_Init 0x%x", s32Ret);
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