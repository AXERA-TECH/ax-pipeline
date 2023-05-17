#include "common_func.h"
#include "../utilities/sample_log.h"
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

#endif