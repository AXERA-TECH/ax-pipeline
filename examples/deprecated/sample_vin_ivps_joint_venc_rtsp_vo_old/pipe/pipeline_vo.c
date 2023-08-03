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

#include "../sample_vin_ivps_joint_venc_rtsp_vo.h"
#include "vo/sample_comm_vo.h"
#include "vo/sample_vo_pattern.h"

#define SAMPLE_VO_DEV0 0

SAMPLE_VO_CONFIG_S stVoConf = {
    /* for device */
    .VoDev = SAMPLE_VO_DEV0,
    .enVoIntfType = VO_INTF_DSI0,
    .enIntfSync = VO_OUTPUT_USER,
    .stReso = {
        .u32Width = 1280,
        .u32Height = 800,
        .u32RefreshRate = 45,
    },
    .u32LayerNr = 1,
    .stVoLayer = {
        {
            // layer0
            .VoLayer = 0,
            .stVoLayerAttr = {
                .stDispRect = {0, 0, 1280, 800},
                .stImageSize = {1280, 800},
                .enPixFmt = AX_YUV420_SEMIPLANAR,
                .enLayerSync = VO_LAYER_SYNC_PRIMARY,
                .u32PrimaryChnId = 0,
                .u32FrameRate = 0,
                .u32FifoDepth = 0,
                .u32ChnNr = 2,
                .u32BkClr = 0,
                .enLayerBuf = VO_LAYER_OUT_BUF_POOL,
                .u32InplaceChnId = 0,
                .u32PoolId = 0,
                .enDispatchMode = VO_LAYER_OUT_TO_FIFO,
                .u64KeepChnPrevFrameBitmap = 0x1,
            },
            .enVoMode = VO_MODE_1MUX,
        },
        {
            // layer1
            .VoLayer = 1,
            .stVoLayerAttr = {
                .stDispRect = {0, 0, 1280, 800},
                .stImageSize = {1280, 800},
                .enPixFmt = AX_YUV420_SEMIPLANAR,
                .enLayerSync = VO_LAYER_SYNC_NORMAL,
                .u32PrimaryChnId = 0,
                .u32FrameRate = 0,
                .u32FifoDepth = 0,
                .u32ChnNr = 16,
                .u32BkClr = 0,
                .enLayerBuf = VO_LAYER_OUT_BUF_POOL,
                .u32InplaceChnId = 0,
                .u32PoolId = 0,
                .enDispatchMode = VO_LAYER_OUT_TO_FIFO,
                .u64KeepChnPrevFrameBitmap = ~0x0,
            },
            .enVoMode = VO_MODE_1MUX,
        },
    },
};

SAMPLE_VO_CHN_THREAD_PARAM_S g_stChnThreadParam = {0};

AX_S32 ParseVoPubAttr(AX_CHAR *pStr, SAMPLE_VO_CONFIG_S *pstVoConf)
{
    AX_CHAR *p, *end;

    if (!pStr || !pstVoConf)
        return -EINVAL;

    p = pStr;

    if (strstr(p, "dpi"))
    {
        SAMPLE_PRT("dpi output\n");
        pstVoConf->enVoIntfType = VO_INTF_DPI;
    }
    else if (strstr(p, "dsi0"))
    {
        SAMPLE_PRT("dsi0 output\n");
        pstVoConf->enVoIntfType = VO_INTF_DSI0;
    }
    else if (strstr(p, "dsi1"))
    {
        SAMPLE_PRT("dsi1 output\n");
        pstVoConf->enVoIntfType = VO_INTF_DSI1;
    }
    else if (strstr(p, "2dsi"))
    {
        SAMPLE_PRT("dsi0 output\n");
        pstVoConf->enVoIntfType = VO_INTF_2DSI;
        pstVoConf->u32LayerNr = 2;
    }
    else if (strstr(p, "bt656"))
    {
        SAMPLE_PRT("bt656 output\n");
        pstVoConf->enVoIntfType = VO_INTF_BT656;
    }
    else if (strstr(p, "bt1120"))
    {
        SAMPLE_PRT("bt1120 output\n");
        pstVoConf->enVoIntfType = VO_INTF_BT1120;
    }
    else
    {
        SAMPLE_PRT("unsupported interface type, %s\n", p);
        return -EINVAL;
    }

    end = strstr(p, "@");
    p = end + 1;
    pstVoConf->stReso.u32Width = strtoul(p, &end, 10);
    end = strstr(p, "x");
    p = end + 1;
    pstVoConf->stReso.u32Height = strtoul(p, &end, 10);
    end = strstr(p, "@");
    p = end + 1;
    pstVoConf->stReso.u32RefreshRate = strtoul(p, &end, 10);

    SAMPLE_PRT("reso: %dx%d-%d\n", pstVoConf->stReso.u32Width,
               pstVoConf->stReso.u32Height, pstVoConf->stReso.u32RefreshRate);

    return 0;
}

static AX_S32 SAMPLE_VO_CREATE_POOL(AX_U32 u32BlkCnt, AX_U64 u64BlkSize, AX_U64 u64MetaSize, AX_U32 *pPoolID)
{
    AX_POOL_CONFIG_T stPoolCfg = {0};

    stPoolCfg.MetaSize = u64MetaSize;
    stPoolCfg.BlkCnt = u32BlkCnt;
    stPoolCfg.BlkSize = u64BlkSize;
    stPoolCfg.CacheMode = POOL_CACHE_MODE_NONCACHE;
    strcpy((char *)stPoolCfg.PartitionName, "anonymous");

    *pPoolID = AX_POOL_CreatePool(&stPoolCfg);
    if (*pPoolID == AX_INVALID_POOLID)
    {
        SAMPLE_PRT("AX_POOL_CreatePool failed, u32BlkCnt = %d, u64BlkSize = 0x%llx, u64MetaSize = 0x%llx\n", u32BlkCnt,
                   u64BlkSize, u64MetaSize);
        return -1;
    }

    SAMPLE_PRT("u32BlkCnt = %d, u64BlkSize = 0x%llx, pPoolID = %d\n", u32BlkCnt, u64BlkSize, *pPoolID);

    return 0;
}

static int SAMPLE_VO_POOL_DESTROY(AX_U32 u32PoolID)
{
    return AX_POOL_MarkDestroyPool(u32PoolID);
}

AX_S32 VoInit(char *pStr)
{
    AX_S32 i, s32Ret;
    AX_U64 u64BlkSize = 0;
    SAMPLE_VO_LAYER_CONFIG_S *pstVoLayer;
    VO_VIDEO_LAYER_ATTR_S *pstVoLayerAttr;
    SAMPLE_VO_CHN_THREAD_PARAM_S *pstChnThreadParam = &g_stChnThreadParam;

    s32Ret = ParseVoPubAttr(pStr, &stVoConf);
    SAMPLE_VO_CONFIG_S *pstVoConf = &stVoConf;

    s32Ret = AX_SYS_Init();
    if (s32Ret)
    {
        SAMPLE_PRT("AX_SYS_Init failed, s32Ret = 0x%x\n", s32Ret);
        return -1;
    }

    s32Ret = AX_VO_Init();
    if (s32Ret)
    {
        SAMPLE_PRT("AX_VO_Init failed, s32Ret = 0x%x\n", s32Ret);
        goto exit0;
    }

    SAMPLE_PRT("u32LayerNr = %d\n", pstVoConf->u32LayerNr);

    for (i = 0; i < pstVoConf->u32LayerNr; i++)
    {
        pstVoLayer = &pstVoConf->stVoLayer[i];
        pstVoLayerAttr = &pstVoLayer->stVoLayerAttr;
        if (i == 0)
        {
            pstVoLayerAttr->stImageSize.u32Width = SAMPLE_MINOR_STREAM_HEIGHT;
            pstVoLayerAttr->stImageSize.u32Height = SAMPLE_MINOR_STREAM_WIDTH;
        }
        else
        {
            // pstVoLayer->enVoMode = VO_MODE_1MUX;
            // pstVoLayerAttr->stImageSize.u32Width = (pstVoConf->stReso.u32Width + 0xF) & (~0xF);
            // pstVoLayerAttr->stImageSize.u32Height = pstVoConf->stReso.u32Height;
            // pstVoLayerAttr->stDispRect.u32X = pstVoConf->stReso.u32Width;
            // pstVoLayerAttr->u32ChnNr = 1;
        }

        pstVoLayerAttr->stDispRect.u32Width = pstVoLayerAttr->stImageSize.u32Width;
        pstVoLayerAttr->stDispRect.u32Height = pstVoLayerAttr->stImageSize.u32Height;

        SAMPLE_PRT("layer%d u32Width = %d, u32Height = %d\n", i, pstVoLayerAttr->stImageSize.u32Width,
                   pstVoLayerAttr->stImageSize.u32Height);

        pstVoLayerAttr->u32PoolId = ~0;
        u64BlkSize = pstVoLayerAttr->stImageSize.u32Width * pstVoLayerAttr->stImageSize.u32Height * 3 / 2;
        s32Ret = SAMPLE_VO_CREATE_POOL(3, u64BlkSize, 512, &pstVoLayerAttr->u32PoolId);
        if (s32Ret)
        {
            SAMPLE_PRT("SAMPLE_VO_CREATE_POOL failed, s32Ret = 0x%x\n", s32Ret);
            goto exit1;
        }

        SAMPLE_PRT("layer pool id = 0x%x\n", pstVoLayerAttr->u32PoolId);
    }

    s32Ret = SAMPLE_COMM_VO_StartVO(pstVoConf);
    if (s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VO_StartVO failed, s32Ret = 0x%x\n", s32Ret);
        goto exit1;
    }

    // for (i = 0; i < pstVoConf->u32LayerNr; i++) {
    //     pstVoLayer = &pstVoConf->stVoLayer[i];
    //     if (i == 0) {
    //         pstVoLayer->stSrcMod.enModId = AX_ID_IVPS;
    //         pstVoLayer->stSrcMod.s32GrpId = u32IvpsGrp;
    //         pstVoLayer->stSrcMod.s32ChnId = 0;

    //         pstVoLayer->stDstMod.enModId = AX_ID_VO;
    //         pstVoLayer->stDstMod.s32GrpId = 0;
    //         pstVoLayer->stDstMod.s32ChnId = 0;
    //         AX_SYS_Link(&pstVoLayer->stSrcMod, &pstVoLayer->stDstMod);
    //     } else {
    //         // pstChnThreadParam->u32LayerID = pstVoLayer->VoLayer;
    //         // pstChnThreadParam->u32ChnID = 0;
    //         // pstChnThreadParam->u32ThreadForceStop = 0;
    //         // pstChnThreadParam->u32UserPoolId = ~0;
    //         // pthread_create(&pstChnThreadParam->ThreadID, NULL, SAMPLE_VO_CHN_THREAD, pstChnThreadParam);
    //     }
    // }

    SAMPLE_PRT("done\n");

    return 0;

exit1:
    for (i = 0; i < pstVoConf->u32LayerNr; i++)
    {
        pstVoLayer = &pstVoConf->stVoLayer[i];
        pstVoLayerAttr = &pstVoLayer->stVoLayerAttr;
        if (pstVoLayerAttr->u32PoolId != ~0)
            SAMPLE_VO_POOL_DESTROY(pstVoLayerAttr->u32PoolId);
    }

    AX_VO_Deinit();

exit0:
    AX_SYS_Deinit();

    SAMPLE_PRT("done, s32Ret = 0x%x\n", s32Ret);

    return s32Ret;
}

AX_VOID VoDeInit()
{
    AX_S32 i;
    SAMPLE_VO_LAYER_CONFIG_S *pstVoLayer;
    VO_VIDEO_LAYER_ATTR_S *pstVoLayerAttr;
    SAMPLE_VO_CHN_THREAD_PARAM_S *pstChnThreadParam = &g_stChnThreadParam;
    SAMPLE_VO_CONFIG_S *pstVoConf = &stVoConf;

    // for (i = 0; i < pstVoConf->u32LayerNr; i++) {
    //     pstVoLayer = &pstVoConf->stVoLayer[i];
    //     if (i == 0) {
    //         AX_SYS_UnLink(&pstVoLayer->stSrcMod, &pstVoLayer->stDstMod);
    //     } else {
    //         if (pstChnThreadParam->ThreadID) {
    //             pstChnThreadParam->u32ThreadForceStop = 1;
    //             pthread_join(pstChnThreadParam->ThreadID, NULL);
    //         }
    //     }
    // }

    SAMPLE_COMM_VO_StopVO(pstVoConf);

    AX_VO_Deinit();

    for (i = 0; i < pstVoConf->u32LayerNr; i++)
    {
        pstVoLayer = &pstVoConf->stVoLayer[i];
        pstVoLayerAttr = &pstVoLayer->stVoLayerAttr;
        if (i != 0)
        {
            if (pstChnThreadParam->u32UserPoolId != ~0)
                SAMPLE_VO_POOL_DESTROY(pstChnThreadParam->u32UserPoolId);
        }

        if (pstVoLayerAttr->u32PoolId != ~0)
            SAMPLE_VO_POOL_DESTROY(pstVoLayerAttr->u32PoolId);
    }

    AX_SYS_Deinit();
}
