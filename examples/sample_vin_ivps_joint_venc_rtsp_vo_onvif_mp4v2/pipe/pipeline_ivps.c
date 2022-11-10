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

AX_S32 SampleLinkInit()
{
    AX_S32 i = 0;

    /*
    VIN --> IVPS --> VENC
    (ModId   GrpId   ChnId) | (ModId   GrpId   ChnId)
    --------------------------------------------------
    (VIN        0       2) -> (IVPS     2       0)
    (VIN        0       1) -> (IVPS     1       0)
    (VIN        0       0) -> (IVPS     0       0)
    (IVPS       2       0) -> (VENC     0       2)
    //(IVPS       1       0) -> (VENC     0       1)
    (IVPS       0       0) -> (VENC     0       0)

    VIN --> VENC
    (ModId   GrpId   ChnId) | (ModId   GrpId   ChnId)
    --------------------------------------------------
    (VIN        0       2) -> (VENC     0       2)
    (VIN        0       1) -> (VENC     0       1)
    (VIN        0       0) -> (VENC     0       0)
    */

    for (i = 0; i < SAMPLE_IVPS_GROUP_NUM; i++)
    {
        AX_MOD_INFO_S srcMod, dstMod;
        srcMod.enModId = AX_ID_VIN;
        srcMod.s32GrpId = 0;
        srcMod.s32ChnId = i;

        dstMod.enModId = AX_ID_IVPS;
        dstMod.s32GrpId = i;
        dstMod.s32ChnId = 0;
        AX_SYS_Link(&srcMod, &dstMod);

        srcMod.enModId = AX_ID_IVPS;
        srcMod.s32GrpId = i;
        srcMod.s32ChnId = 0;

        if (i == 1)
        {
            // for AI runjoint
            continue;
        }
        else if (i == 2)
        {
            // for vo
            dstMod.enModId = AX_ID_VO;
            dstMod.s32GrpId = 0;
            dstMod.s32ChnId = 0;
            AX_SYS_Link(&srcMod, &dstMod);
        }
        else
        {

            srcMod.enModId = AX_ID_IVPS;
            srcMod.s32GrpId = i;
            srcMod.s32ChnId = 0;

            dstMod.enModId = AX_ID_VENC;
            dstMod.s32GrpId = 0;
            dstMod.s32ChnId = i;
            AX_SYS_Link(&srcMod, &dstMod);

            srcMod.enModId = AX_ID_IVPS;
            srcMod.s32GrpId = i;
            srcMod.s32ChnId = 1;
            
            dstMod.enModId = AX_ID_VENC;
            dstMod.s32GrpId = 0;
            dstMod.s32ChnId = 1;
            AX_SYS_Link(&srcMod, &dstMod);
        }
    }

    return 0;
}

AX_S32 SampleLinkDeInit()
{
    AX_S32 i = 0;

    /*
    VIN --> IVPS --> VENC
    (ModId   GrpId   ChnId) | (ModId   GrpId   ChnId)
    --------------------------------------------------
    (VIN        0       2) -> (IVPS     2       0)
    (VIN        0       1) -> (IVPS     1       0)
    (VIN        0       0) -> (IVPS     0       0)
    (IVPS       2       0) -> (VENC     0       2)
    //(IVPS       1       0) -> (VENC     0       1)
    (IVPS       0       0) -> (VENC     0       0)

    VIN --> VENC
    (ModId   GrpId   ChnId) | (ModId   GrpId   ChnId)
    --------------------------------------------------
    (VIN        0       2) -> (VENC     0       2)
    (VIN        0       1) -> (VENC     0       1)
    (VIN        0       0) -> (VENC     0       0)
    */

    for (i = 0; i < SAMPLE_IVPS_GROUP_NUM; i++)
    {
        AX_MOD_INFO_S srcMod, dstMod;
        srcMod.enModId = AX_ID_VIN;
        srcMod.s32GrpId = 0;
        srcMod.s32ChnId = i;

        dstMod.enModId = AX_ID_IVPS;
        dstMod.s32GrpId = i;
        dstMod.s32ChnId = 0;
        AX_SYS_UnLink(&srcMod, &dstMod);

        srcMod.enModId = AX_ID_IVPS;
        srcMod.s32GrpId = i;
        srcMod.s32ChnId = 0;

        if (i == 1)
        {
            // for AI runjoint
            continue;
        }
        else if (i == 2)
        {
            // for vo
            dstMod.enModId = AX_ID_VO;
            dstMod.s32GrpId = 0;
            dstMod.s32ChnId = 0;
            AX_SYS_UnLink(&srcMod, &dstMod);
        }
        else
        {
            srcMod.enModId = AX_ID_IVPS;
            srcMod.s32GrpId = i;
            srcMod.s32ChnId = 0;

            dstMod.enModId = AX_ID_VENC;
            dstMod.s32GrpId = 0;
            dstMod.s32ChnId = i;
            AX_SYS_UnLink(&srcMod, &dstMod);

            srcMod.enModId = AX_ID_IVPS;
            srcMod.s32GrpId = i;
            srcMod.s32ChnId = 1;

            dstMod.enModId = AX_ID_VENC;
            dstMod.s32GrpId = 0;
            dstMod.s32ChnId = 1;
            AX_SYS_UnLink(&srcMod, &dstMod);
        }
    }

    return 0;
}

int SampleIvpsInit()
{
    AX_S32 s32Ret = 0;
    AX_S32 nGrp = 0, nChn = 0;
    AX_IVPS_GRP_ATTR_S stGrpAttr = {0};
    AX_IVPS_PIPELINE_ATTR_S stPipelineAttr = {0};

    s32Ret = AX_IVPS_Init();
    if (0 != s32Ret)
    {
        ALOGE("AX_IVPS_Init failed,s32Ret:0x%x\n", s32Ret);
        return s32Ret;
    }

    stPipelineAttr.tFbInfo.PoolId = AX_INVALID_POOLID;
    stPipelineAttr.nOutChnNum = 3;

    for (nGrp = 0; nGrp < SAMPLE_IVPS_GROUP_NUM; nGrp++)
    {
        stGrpAttr.nInFifoDepth = 1;
        stGrpAttr.ePipeline = AX_IVPS_PIPELINE_DEFAULT;
        s32Ret = AX_IVPS_CreateGrp(nGrp, &stGrpAttr);
        if (0 != s32Ret)
        {
            ALOGE("AX_IVPS_CreateGrp failed,nGrp %d,s32Ret:0x%x\n", nGrp, s32Ret);
            return s32Ret;
        }

        for (nChn = 0; nChn < SAMPLE_IVPS_CHN_NUM; nChn++)
        {
            memset(&stPipelineAttr.tFilter, 0x00, sizeof(stPipelineAttr.tFilter));

            if (nGrp == 0)
            {
                stPipelineAttr.tFilter[nChn + 1][0].bEnable = AX_TRUE;
                stPipelineAttr.tFilter[nChn + 1][0].tFRC.nSrcFrameRate = s_sample_framerate;
                stPipelineAttr.tFilter[nChn + 1][0].tFRC.nDstFrameRate = s_sample_framerate;
                stPipelineAttr.tFilter[nChn + 1][0].nDstPicOffsetX0 = 0;
                stPipelineAttr.tFilter[nChn + 1][0].nDstPicOffsetY0 = 0;
                stPipelineAttr.tFilter[nChn + 1][0].nDstPicWidth = gCams[0].stChnAttr.tChnAttr[AX_YUV_SOURCE_ID_MAIN].nWidth;
                stPipelineAttr.tFilter[nChn + 1][0].nDstPicHeight = gCams[0].stChnAttr.tChnAttr[AX_YUV_SOURCE_ID_MAIN].nHeight;
                stPipelineAttr.tFilter[nChn + 1][0].nDstPicStride = ALIGN_UP(gCams[0].stChnAttr.tChnAttr[AX_YUV_SOURCE_ID_MAIN].nWidthStride, 64);
                stPipelineAttr.tFilter[nChn + 1][0].nDstFrameWidth = gCams[0].stChnAttr.tChnAttr[AX_YUV_SOURCE_ID_MAIN].nWidth;
                stPipelineAttr.tFilter[nChn + 1][0].nDstFrameHeight = gCams[0].stChnAttr.tChnAttr[AX_YUV_SOURCE_ID_MAIN].nHeight;
                stPipelineAttr.tFilter[nChn + 1][0].eDstPicFormat = AX_YUV420_SEMIPLANAR;
                stPipelineAttr.tFilter[nChn + 1][0].eEngine = AX_IVPS_ENGINE_GDC;
                stPipelineAttr.tFilter[nChn + 1][0].tGdcCfg.eRotation = AX_IVPS_ROTATION_0;
                // nFilter 0x11 for osd
                stPipelineAttr.tFilter[nChn + 1][1].bEnable = AX_TRUE;
                stPipelineAttr.tFilter[nChn + 1][1].tFRC.nSrcFrameRate = s_sample_framerate;
                stPipelineAttr.tFilter[nChn + 1][1].tFRC.nDstFrameRate = s_sample_framerate;
                stPipelineAttr.tFilter[nChn + 1][1].nDstPicOffsetX0 = 0;
                stPipelineAttr.tFilter[nChn + 1][1].nDstPicOffsetY0 = 0;
                stPipelineAttr.tFilter[nChn + 1][1].nDstPicWidth = gCams[0].stChnAttr.tChnAttr[AX_YUV_SOURCE_ID_MAIN].nWidth;
                stPipelineAttr.tFilter[nChn + 1][1].nDstPicHeight = gCams[0].stChnAttr.tChnAttr[AX_YUV_SOURCE_ID_MAIN].nHeight;
                stPipelineAttr.tFilter[nChn + 1][1].nDstPicStride = ALIGN_UP(gCams[0].stChnAttr.tChnAttr[AX_YUV_SOURCE_ID_MAIN].nWidthStride, 64);
                stPipelineAttr.tFilter[nChn + 1][1].nDstFrameWidth = gCams[0].stChnAttr.tChnAttr[AX_YUV_SOURCE_ID_MAIN].nWidth;
                stPipelineAttr.tFilter[nChn + 1][1].nDstFrameHeight = gCams[0].stChnAttr.tChnAttr[AX_YUV_SOURCE_ID_MAIN].nHeight;
                stPipelineAttr.tFilter[nChn + 1][1].eDstPicFormat = AX_YUV420_SEMIPLANAR;
                stPipelineAttr.tFilter[nChn + 1][1].eEngine = AX_IVPS_ENGINE_TDP;
                stPipelineAttr.nOutFifoDepth[nChn] = 0;
                //chn1
                stPipelineAttr.tFilter[nChn + 2][0].bEnable = AX_TRUE;
                stPipelineAttr.tFilter[nChn + 2][0].tFRC.nSrcFrameRate = s_sample_framerate;
                stPipelineAttr.tFilter[nChn + 2][0].tFRC.nDstFrameRate = s_sample_framerate;
                stPipelineAttr.tFilter[nChn + 2][0].nDstPicOffsetX0 = 0;
                stPipelineAttr.tFilter[nChn + 2][0].nDstPicOffsetY0 = 0;
                stPipelineAttr.tFilter[nChn + 2][0].nDstPicWidth = SAMPLE_FHD_STREAM_WIDTH;
                stPipelineAttr.tFilter[nChn + 2][0].nDstPicHeight = SAMPLE_FHD_STREAM_HEIGHT;
                stPipelineAttr.tFilter[nChn + 2][0].nDstPicStride = ALIGN_UP(SAMPLE_FHD_STREAM_WIDTH, 64);
                stPipelineAttr.tFilter[nChn + 2][0].nDstFrameWidth = SAMPLE_FHD_STREAM_WIDTH;
                stPipelineAttr.tFilter[nChn + 2][0].nDstFrameHeight = SAMPLE_FHD_STREAM_HEIGHT;
                stPipelineAttr.tFilter[nChn + 2][0].eDstPicFormat = AX_YUV420_SEMIPLANAR;
                stPipelineAttr.tFilter[nChn + 2][0].eEngine = AX_IVPS_ENGINE_GDC;
                stPipelineAttr.tFilter[nChn + 2][0].tGdcCfg.eRotation = AX_IVPS_ROTATION_0;
                // nFilter 0x21 for osd
                stPipelineAttr.tFilter[nChn + 2][1].bEnable = AX_TRUE;
                stPipelineAttr.tFilter[nChn + 2][1].tFRC.nSrcFrameRate = s_sample_framerate;
                stPipelineAttr.tFilter[nChn + 2][1].tFRC.nDstFrameRate = s_sample_framerate;
                stPipelineAttr.tFilter[nChn + 2][1].nDstPicOffsetX0 = 0;
                stPipelineAttr.tFilter[nChn + 2][1].nDstPicOffsetY0 = 0;
                stPipelineAttr.tFilter[nChn + 2][1].nDstPicWidth = SAMPLE_FHD_STREAM_WIDTH;
                stPipelineAttr.tFilter[nChn + 2][1].nDstPicHeight = SAMPLE_FHD_STREAM_HEIGHT;
                stPipelineAttr.tFilter[nChn + 2][1].nDstPicStride = ALIGN_UP(SAMPLE_FHD_STREAM_WIDTH, 64);
                stPipelineAttr.tFilter[nChn + 2][1].nDstFrameWidth = SAMPLE_FHD_STREAM_WIDTH;
                stPipelineAttr.tFilter[nChn + 2][1].nDstFrameHeight = SAMPLE_FHD_STREAM_HEIGHT;
                stPipelineAttr.tFilter[nChn + 2][1].eDstPicFormat = AX_YUV420_SEMIPLANAR;
                stPipelineAttr.tFilter[nChn + 2][1].eEngine = AX_IVPS_ENGINE_TDP;
                stPipelineAttr.nOutFifoDepth[nChn + 1] = 0;
            }
            else if (nGrp == 1)
            {
                stPipelineAttr.tFilter[nChn + 1][0].bEnable = AX_TRUE;
                stPipelineAttr.tFilter[nChn + 1][0].tFRC.nSrcFrameRate = s_sample_framerate;
                stPipelineAttr.tFilter[nChn + 1][0].tFRC.nDstFrameRate = s_sample_framerate;
                stPipelineAttr.tFilter[nChn + 1][0].nDstPicOffsetX0 = 0;
                stPipelineAttr.tFilter[nChn + 1][0].nDstPicOffsetY0 = 0;
                stPipelineAttr.tFilter[nChn + 1][0].nDstPicWidth = SAMPLE_ALGO_WIDTH;
                stPipelineAttr.tFilter[nChn + 1][0].nDstPicHeight = SAMPLE_ALGO_HEIGHT;
                stPipelineAttr.tFilter[nChn + 1][0].nDstPicStride = ALIGN_UP(stPipelineAttr.tFilter[nChn + 1][0].nDstPicWidth, 64);
                stPipelineAttr.tFilter[nChn + 1][0].nDstFrameWidth = SAMPLE_ALGO_WIDTH;
                stPipelineAttr.tFilter[nChn + 1][0].nDstFrameHeight = SAMPLE_ALGO_HEIGHT;
                stPipelineAttr.tFilter[nChn + 1][0].eDstPicFormat = AX_YUV420_SEMIPLANAR;
                stPipelineAttr.tFilter[nChn + 1][0].eEngine = AX_IVPS_ENGINE_TDP;
                stPipelineAttr.tFilter[nChn + 1][0].tTdpCfg.eRotation = AX_IVPS_ROTATION_0;
                // letterbox filling image
                AX_IVPS_ASPECT_RATIO_S tAspectRatio;
                tAspectRatio.eMode = AX_IVPS_ASPECT_RATIO_AUTO;
                tAspectRatio.eAligns[0] = AX_IVPS_ASPECT_RATIO_HORIZONTAL_CENTER;
                tAspectRatio.eAligns[1] = AX_IVPS_ASPECT_RATIO_VERTICAL_CENTER;
                tAspectRatio.nBgColor = 0x0000FF;
                stPipelineAttr.tFilter[nChn + 1][0].tTdpCfg.tAspectRatio = tAspectRatio;

                stPipelineAttr.nOutFifoDepth[nChn] = 1; // for AI runjoint
            }
            else
            {
                stPipelineAttr.tFilter[nChn + 1][0].bEnable = AX_TRUE;
                stPipelineAttr.tFilter[nChn + 1][0].tFRC.nSrcFrameRate = 60;
                stPipelineAttr.tFilter[nChn + 1][0].tFRC.nDstFrameRate = 60;
                stPipelineAttr.tFilter[nChn + 1][0].nDstPicOffsetX0 = 0;
                stPipelineAttr.tFilter[nChn + 1][0].nDstPicOffsetY0 = 0;
                stPipelineAttr.tFilter[nChn + 1][0].nDstPicWidth = SAMPLE_MINOR_STREAM_WIDTH;
                stPipelineAttr.tFilter[nChn + 1][0].nDstPicHeight = SAMPLE_MINOR_STREAM_HEIGHT;
                stPipelineAttr.tFilter[nChn + 1][0].nDstPicStride = ALIGN_UP(stPipelineAttr.tFilter[nChn + 1][0].nDstPicWidth, 64);
                stPipelineAttr.tFilter[nChn + 1][0].nDstFrameWidth = SAMPLE_MINOR_STREAM_WIDTH;
                stPipelineAttr.tFilter[nChn + 1][0].nDstFrameHeight = SAMPLE_MINOR_STREAM_HEIGHT;
                stPipelineAttr.tFilter[nChn + 1][0].eDstPicFormat = AX_YUV420_SEMIPLANAR;
                stPipelineAttr.tFilter[nChn + 1][0].eEngine = AX_IVPS_ENGINE_TDP;
                // nFilter 0x11 for osd
                stPipelineAttr.tFilter[nChn + 1][1].bEnable = AX_TRUE;
                stPipelineAttr.tFilter[nChn + 1][1].tFRC.nSrcFrameRate = 60;
                stPipelineAttr.tFilter[nChn + 1][1].tFRC.nDstFrameRate = 60;
                stPipelineAttr.tFilter[nChn + 1][1].nDstPicOffsetX0 = 0;
                stPipelineAttr.tFilter[nChn + 1][1].nDstPicOffsetY0 = 0;
                stPipelineAttr.tFilter[nChn + 1][1].nDstPicWidth = SAMPLE_MINOR_STREAM_HEIGHT;
                stPipelineAttr.tFilter[nChn + 1][1].nDstPicHeight = SAMPLE_MINOR_STREAM_WIDTH;
                stPipelineAttr.tFilter[nChn + 1][1].nDstPicStride = ALIGN_UP(stPipelineAttr.tFilter[nChn + 1][0].nDstPicWidth, 64);
                stPipelineAttr.tFilter[nChn + 1][1].nDstFrameWidth = SAMPLE_MINOR_STREAM_HEIGHT;
                stPipelineAttr.tFilter[nChn + 1][1].nDstFrameHeight = SAMPLE_MINOR_STREAM_WIDTH;
                stPipelineAttr.tFilter[nChn + 1][1].eDstPicFormat = AX_YUV420_SEMIPLANAR;
                stPipelineAttr.tFilter[nChn + 1][1].eEngine = AX_IVPS_ENGINE_TDP;
                stPipelineAttr.tFilter[nChn + 1][1].tTdpCfg.eRotation = AX_IVPS_ROTATION_90;
                stPipelineAttr.nOutFifoDepth[nChn] = 0;
            }
            
            s32Ret = AX_IVPS_SetPipelineAttr(nGrp, &stPipelineAttr);
            if (0 != s32Ret)
            {
                ALOGE("AX_IVPS_SetPipelineAttr failed,nGrp %d,s32Ret:0x%x\n", nGrp, s32Ret);
                return s32Ret;
            }

            s32Ret = AX_IVPS_EnableChn(nGrp, nChn);
            if (0 != s32Ret)
            {
                ALOGE("AX_IVPS_EnableChn failed,nGrp %d,nChn %d,s32Ret:0x%x\n", nGrp, nChn, s32Ret);
                return s32Ret;
            }
            if(nGrp == 0)
                s32Ret = AX_IVPS_EnableChn(nGrp, 1);
            if (0 != s32Ret)
            {
                ALOGE("AX_IVPS_EnableChn failed,nGrp %d,nChn %d,s32Ret:0x%x\n", nGrp, nChn, s32Ret);
                return s32Ret;
            }
        }

        s32Ret = AX_IVPS_StartGrp(nGrp);
        if (0 != s32Ret)
        {
            ALOGE("AX_IVPS_StartGrp failed,nGrp %d,s32Ret:0x%x\n", nGrp, s32Ret);
            return s32Ret;
        }
    }

    StartOverlay();

    return 0;
}

AX_S32 SampleIvpsDeInit()
{
    AX_S32 s32Ret = 0, nGrp = 0, nChn = 0;

    for (nGrp = 0; nGrp < SAMPLE_IVPS_GROUP_NUM; nGrp++)
    {
        s32Ret = AX_IVPS_StopGrp(nGrp);
        if (0 != s32Ret)
        {
            ALOGE("AX_IVPS_StopGrp failed,nGrp %d,s32Ret:0x%x\n", nGrp, s32Ret);
            return s32Ret;
        }

        for (nChn = 0; nChn < SAMPLE_IVPS_CHN_NUM; nChn++)
        {
            
            s32Ret = AX_IVPS_DisableChn(nGrp, nChn);
            if (0 != s32Ret)
            {
                ALOGE("AX_IVPS_DisableChn failed,nGrp %d,nChn %d,s32Ret:0x%x\n", nGrp, nChn, s32Ret);
                return s32Ret;
            }
            if(nGrp == 0)
            {
                s32Ret = AX_IVPS_DisableChn(nGrp, 1);
                if (0 != s32Ret)
                {
                    ALOGE("AX_IVPS_DisableChn failed,nGrp %d,nChn %d,s32Ret:0x%x\n", nGrp, nChn, s32Ret);
                    return s32Ret;
                }
            }
            
        }

        s32Ret = AX_IVPS_DestoryGrp(nGrp);
        if (0 != s32Ret)
        {
            ALOGE("AX_IVPS_DestoryGrp failed,nGrp %d,s32Ret:0x%x", nGrp, s32Ret);
            return s32Ret;
        }
    }

    s32Ret = AX_IVPS_Deinit();
    if (0 != s32Ret)
    {
        ALOGE("AX_IVPS_Deinit failed,s32Ret:0x%x\n", s32Ret);
        return s32Ret;
    }

    return 0;
}
