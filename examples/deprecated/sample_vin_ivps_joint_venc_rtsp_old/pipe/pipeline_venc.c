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

#include "../sample_vin_ivps_joint_venc_rtsp.h"
#include "../../utilities/net_utils.h"

VENC_GETSTREAM_PARAM_T gGetStreamPara[SAMPLE_VENC_CHN_NUM];
pthread_t gGetStreamPid[SAMPLE_VENC_CHN_NUM];

static void PrintRtsp(char *rtsp_path)
{
    char ipaddr[64];
    int ret = get_ip("eth0", ipaddr);
    printf("\n");
    if (ret == 0)
    {
        ALOGI("                                    [eth0]  rtsp url >>>>>> rtsp://%s:%d%s <<<<<<", ipaddr, RTSP_PORT, rtsp_path);
    }
    ret = get_ip("wlan0", ipaddr);
    if (ret == 0)
    {
        ALOGI("                                    [wlan0] rtsp url >>>>>> rtsp://%s:%d%s <<<<<<", ipaddr, RTSP_PORT, rtsp_path);
    }
    ret = get_ip("usb0", ipaddr);
    if (ret == 0)
    {
        ALOGI("                                    [usb0]  rtsp url >>>>>> rtsp://%s:%d%s <<<<<<\n", ipaddr, RTSP_PORT, rtsp_path);
    }
}

static void *VencGetStreamProc(void *arg)
{
    AX_S32 s32Ret = -1;
    AX_VENC_RECV_PIC_PARAM_S stRecvParam;
    VENC_GETSTREAM_PARAM_T *pstPara;
    pstPara = (VENC_GETSTREAM_PARAM_T *)arg;
    AX_VENC_STREAM_S stStream;
    AX_S16 syncType = -1;
    AX_U32 totalGetStream = 0;

    s32Ret = AX_VENC_StartRecvFrame(pstPara->VeChn, &stRecvParam);
    if (AX_SUCCESS != s32Ret)
    {
        ALOGE("AX_VENC_StartRecvFrame failed, s32Ret:0x%x\n", s32Ret);
        return NULL;
    }

    memset(&stStream, 0, sizeof(AX_VENC_STREAM_S));

    while (AX_TRUE == pstPara->bThreadStart)
    {
        s32Ret = AX_VENC_GetStream(pstPara->VeChn, &stStream, syncType);

        if (AX_SUCCESS == s32Ret)
        {
            totalGetStream++;

            if (rDemoHandle)
            {
                rtsp_sever_tx_video(rDemoHandle, pstPara->rSessionHandle, stStream.stPack.pu8Addr, stStream.stPack.u32Len, stStream.stPack.u64PTS);
            }

            s32Ret = AX_VENC_ReleaseStream(pstPara->VeChn, &stStream);
            if (AX_SUCCESS != s32Ret)
            {
                // ALOGE("VencChn %d: AX_VENC_ReleaseStream failed!s32Ret:0x%x\n",pstPara->VeChn,s32Ret);
                goto EXIT;
            }
        }
    }

EXIT:
    rtsp_del_session(pstPara->rSessionHandle);
    ALOGN("VencChn %d: Total get %u encoded frames. getStream Exit!\n", pstPara->VeChn, totalGetStream);
    return NULL;
}

AX_S32 SampleVencInit(COMMON_VENC_CASE_E eVencType)
{
    AX_VENC_CHN_ATTR_S stVencChnAttr;
    VIDEO_CONFIG_T config = {0};
    AX_S32 VencChn = 0, s32Ret = 0;

    AX_VENC_MOD_ATTR_S stModAttr;
    stModAttr.enVencType = VENC_MULTI_ENCODER;

    s32Ret = AX_VENC_Init(&stModAttr);
    if (AX_SUCCESS != s32Ret)
    {
        ALOGE("AX_VENC_Init failed, s32Ret:0x%x", s32Ret);
        return s32Ret;
    }

    config.stRCInfo.eRCType = VENC_RC_VBR;
    config.nGOP = 50;
    config.nBitrate = 5000;
    config.stRCInfo.nMinQp = 10;
    config.stRCInfo.nMaxQp = 51;
    config.stRCInfo.nMinIQp = 10;
    config.stRCInfo.nMaxIQp = 51;
    config.stRCInfo.nIntraQpDelta = -2;
    config.nOffsetCropX = 0;
    config.nOffsetCropY = 0;
    config.nOffsetCropW = 0;
    config.nOffsetCropH = 0;

    for (VencChn = 0; VencChn < SAMPLE_VENC_CHN_NUM; VencChn++)
    {
        config.ePayloadType = (eVencType == VENC_CASE_H264) ? PT_H264 : PT_H265;
        switch (VencChn)
        {
        case 0:
            config.nInWidth = gCams[0].stChnAttr.tChnAttr[AX_YUV_SOURCE_ID_MAIN].nWidth;
            config.nInHeight = gCams[0].stChnAttr.tChnAttr[AX_YUV_SOURCE_ID_MAIN].nHeight;
            config.nStride = gCams[0].stChnAttr.tChnAttr[AX_YUV_SOURCE_ID_MAIN].nWidthStride;
            config.nSrcFrameRate = s_sample_framerate;
            config.nDstFrameRate = s_sample_framerate;
            break;
        case 1:
            config.nInWidth = SAMPLE_MINOR_STREAM_WIDTH;
            config.nInHeight = SAMPLE_MINOR_STREAM_HEIGHT;
            config.nStride = SAMPLE_MINOR_STREAM_WIDTH;
            config.nSrcFrameRate = s_sample_framerate;
            config.nDstFrameRate = s_sample_framerate;
            break;
        }

        memset(&stVencChnAttr, 0, sizeof(AX_VENC_CHN_ATTR_S));

        stVencChnAttr.stVencAttr.u32MaxPicWidth = 0;
        stVencChnAttr.stVencAttr.u32MaxPicHeight = 0;

        stVencChnAttr.stVencAttr.u32PicWidthSrc = config.nInWidth;   /*the picture width*/
        stVencChnAttr.stVencAttr.u32PicHeightSrc = config.nInHeight; /*the picture height*/

        stVencChnAttr.stVencAttr.u32CropOffsetX = config.nOffsetCropX;
        stVencChnAttr.stVencAttr.u32CropOffsetY = config.nOffsetCropY;
        stVencChnAttr.stVencAttr.u32CropWidth = config.nOffsetCropW;
        stVencChnAttr.stVencAttr.u32CropHeight = config.nOffsetCropH;
        stVencChnAttr.stVencAttr.u32VideoRange = 1; /* 0: Narrow Range(NR), Y[16,235], Cb/Cr[16,240]; 1: Full Range(FR), Y/Cb/Cr[0,255] */

        ALOGN("VencChn %d:w:%d, h:%d, s:%d, Crop:(%d, %d, %d, %d) rcType:%d, payload:%d", gVencChnMapping[VencChn], stVencChnAttr.stVencAttr.u32PicWidthSrc, stVencChnAttr.stVencAttr.u32PicHeightSrc, config.nStride, stVencChnAttr.stVencAttr.u32CropOffsetX, stVencChnAttr.stVencAttr.u32CropOffsetY, stVencChnAttr.stVencAttr.u32CropWidth, stVencChnAttr.stVencAttr.u32CropHeight, config.stRCInfo.eRCType, config.ePayloadType);

        stVencChnAttr.stVencAttr.u32BufSize = config.nStride * config.nInHeight * 3 / 2; /*stream buffer size*/
        stVencChnAttr.stVencAttr.u32MbLinesPerSlice = 0;                                 /*get stream mode is slice mode or frame mode?*/
        stVencChnAttr.stVencAttr.enLinkMode = AX_LINK_MODE;
        stVencChnAttr.stVencAttr.u32GdrDuration = 0;
        /* GOP Setting */
        stVencChnAttr.stGopAttr.enGopMode = VENC_GOPMODE_NORMALP;

        stVencChnAttr.stVencAttr.enType = config.ePayloadType;
        switch (stVencChnAttr.stVencAttr.enType)
        {
        case PT_H265:
        {
            stVencChnAttr.stVencAttr.enProfile = VENC_HEVC_MAIN_PROFILE;
            stVencChnAttr.stVencAttr.enLevel = VENC_HEVC_LEVEL_6;
            stVencChnAttr.stVencAttr.enTier = VENC_HEVC_MAIN_TIER;

            if (config.stRCInfo.eRCType == VENC_RC_CBR)
            {
                AX_VENC_H265_CBR_S stH265Cbr;
                stVencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_H265CBR;
                stVencChnAttr.stRcAttr.s32FirstFrameStartQp = -1;
                stH265Cbr.u32Gop = config.nGOP;
                stH265Cbr.u32SrcFrameRate = config.nSrcFrameRate;  /* input frame rate */
                stH265Cbr.fr32DstFrameRate = config.nDstFrameRate; /* target frame rate */
                stH265Cbr.u32BitRate = config.nBitrate;
                stH265Cbr.u32MinQp = config.stRCInfo.nMinQp;
                stH265Cbr.u32MaxQp = config.stRCInfo.nMaxQp;
                stH265Cbr.u32MinIQp = config.stRCInfo.nMinIQp;
                stH265Cbr.u32MaxIQp = config.stRCInfo.nMaxIQp;
                stH265Cbr.s32IntraQpDelta = config.stRCInfo.nIntraQpDelta;
                memcpy(&stVencChnAttr.stRcAttr.stH265Cbr, &stH265Cbr, sizeof(AX_VENC_H265_CBR_S));
            }
            else if (config.stRCInfo.eRCType == VENC_RC_VBR)
            {
                AX_VENC_H265_VBR_S stH265Vbr;
                stVencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_H265VBR;
                stVencChnAttr.stRcAttr.s32FirstFrameStartQp = -1;
                stH265Vbr.u32Gop = config.nGOP;
                stH265Vbr.u32SrcFrameRate = config.nSrcFrameRate;
                stH265Vbr.fr32DstFrameRate = config.nDstFrameRate;
                stH265Vbr.u32MaxBitRate = config.nBitrate;
                stH265Vbr.u32MinQp = config.stRCInfo.nMinQp;
                stH265Vbr.u32MaxQp = config.stRCInfo.nMaxQp;
                stH265Vbr.u32MinIQp = config.stRCInfo.nMinIQp;
                stH265Vbr.u32MaxIQp = config.stRCInfo.nMaxIQp;
                stH265Vbr.s32IntraQpDelta = config.stRCInfo.nIntraQpDelta;
                memcpy(&stVencChnAttr.stRcAttr.stH265Vbr, &stH265Vbr, sizeof(AX_VENC_H265_VBR_S));
            }
            else if (config.stRCInfo.eRCType == VENC_RC_FIXQP)
            {
                AX_VENC_H265_FIXQP_S stH265FixQp;
                stVencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_H265FIXQP;
                stH265FixQp.u32Gop = config.nGOP;
                stH265FixQp.u32SrcFrameRate = config.nSrcFrameRate;
                stH265FixQp.fr32DstFrameRate = config.nDstFrameRate;
                stH265FixQp.u32IQp = 25;
                stH265FixQp.u32PQp = 30;
                stH265FixQp.u32BQp = 32;
                memcpy(&stVencChnAttr.stRcAttr.stH265FixQp, &stH265FixQp, sizeof(AX_VENC_H265_FIXQP_S));
            }
            break;
        }
        case PT_H264:
        {
            stVencChnAttr.stVencAttr.enProfile = VENC_H264_MAIN_PROFILE;
            stVencChnAttr.stVencAttr.enLevel = VENC_H264_LEVEL_5_2;

            if (config.stRCInfo.eRCType == VENC_RC_CBR)
            {
                AX_VENC_H264_CBR_S stH264Cbr;
                stVencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_H264CBR;
                stVencChnAttr.stRcAttr.s32FirstFrameStartQp = -1;
                stH264Cbr.u32Gop = config.nGOP;
                stH264Cbr.u32SrcFrameRate = config.nSrcFrameRate;  /* input frame rate */
                stH264Cbr.fr32DstFrameRate = config.nDstFrameRate; /* target frame rate */
                stH264Cbr.u32BitRate = config.nBitrate;
                stH264Cbr.u32MinQp = config.stRCInfo.nMinQp;
                stH264Cbr.u32MaxQp = config.stRCInfo.nMaxQp;
                stH264Cbr.u32MinIQp = config.stRCInfo.nMinIQp;
                stH264Cbr.u32MaxIQp = config.stRCInfo.nMaxIQp;
                stH264Cbr.s32IntraQpDelta = config.stRCInfo.nIntraQpDelta;
                memcpy(&stVencChnAttr.stRcAttr.stH264Cbr, &stH264Cbr, sizeof(AX_VENC_H264_CBR_S));
            }
            else if (config.stRCInfo.eRCType == VENC_RC_VBR)
            {
                AX_VENC_H264_VBR_S stH264Vbr;
                stVencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_H264VBR;
                stVencChnAttr.stRcAttr.s32FirstFrameStartQp = -1;
                stH264Vbr.u32Gop = config.nGOP;
                stH264Vbr.u32SrcFrameRate = config.nSrcFrameRate;
                stH264Vbr.fr32DstFrameRate = config.nDstFrameRate;
                stH264Vbr.u32MaxBitRate = config.nBitrate;
                stH264Vbr.u32MinQp = config.stRCInfo.nMinQp;
                stH264Vbr.u32MaxQp = config.stRCInfo.nMaxQp;
                stH264Vbr.u32MinIQp = config.stRCInfo.nMinIQp;
                stH264Vbr.u32MaxIQp = config.stRCInfo.nMaxIQp;
                stH264Vbr.s32IntraQpDelta = config.stRCInfo.nIntraQpDelta;
                memcpy(&stVencChnAttr.stRcAttr.stH264Vbr, &stH264Vbr, sizeof(AX_VENC_H264_VBR_S));
            }
            else if (config.stRCInfo.eRCType == VENC_RC_FIXQP)
            {
                AX_VENC_H264_FIXQP_S stH264FixQp;
                stVencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_H264FIXQP;
                stH264FixQp.u32Gop = config.nGOP;
                stH264FixQp.u32SrcFrameRate = config.nSrcFrameRate;
                stH264FixQp.fr32DstFrameRate = config.nDstFrameRate;
                stH264FixQp.u32IQp = 25;
                stH264FixQp.u32PQp = 30;
                stH264FixQp.u32BQp = 32;
                memcpy(&stVencChnAttr.stRcAttr.stH264FixQp, &stH264FixQp, sizeof(AX_VENC_H264_FIXQP_S));
            }
            break;
        }
        default:
            ALOGE("VencChn %d:Payload type unrecognized.", VencChn);
            return -1;
        }

        AX_S32 ret = AX_VENC_CreateChn(gVencChnMapping[VencChn], &stVencChnAttr);
        if (AX_SUCCESS != ret)
        {
            ALOGE("VencChn %d: AX_VENC_CreateChn failed, s32Ret:0x%x", VencChn, ret);
            return -1;
        }

        /* create get output stream thread */
        gGetStreamPara[VencChn].VeChn = gVencChnMapping[VencChn];
        gGetStreamPara[VencChn].bThreadStart = AX_TRUE;
        gGetStreamPara[VencChn].ePayloadType = config.ePayloadType;
        char rtsp_path[64];
        sprintf(rtsp_path, "/%s%d", RTSP_END_POINT, VencChn);
        gGetStreamPara[VencChn].rSessionHandle = create_rtsp_session(rDemoHandle, rtsp_path, eVencType);
        PrintRtsp(rtsp_path);
        pthread_create(&gGetStreamPid[VencChn], NULL, VencGetStreamProc, (void *)&gGetStreamPara[VencChn]);
    }

    return 0;
}

AX_S32 SampleVencDeInit()
{
    AX_S32 VencChn = 0, s32Ret = 0;

    for (VencChn = 0; VencChn < SAMPLE_VENC_CHN_NUM; VencChn++)
    {

        s32Ret = AX_VENC_StopRecvFrame(gVencChnMapping[VencChn]);
        if (0 != s32Ret)
        {
            ALOGE("VencChn %d:AX_VENC_StopRecvFrame failed,s32Ret:0x%x.\n", gVencChnMapping[VencChn], s32Ret);
            return s32Ret;
        }

        s32Ret = AX_VENC_DestroyChn(gVencChnMapping[VencChn]);
        if (0 != s32Ret)
        {
            ALOGE("VencChn %d:AX_VENC_DestroyChn failed,s32Ret:0x%x.\n", gVencChnMapping[VencChn], s32Ret);
            return s32Ret;
        }

        if (AX_TRUE == gGetStreamPara[VencChn].bThreadStart)
        {
            gGetStreamPara[VencChn].bThreadStart = AX_FALSE;
            pthread_join(gGetStreamPid[VencChn], NULL);
        }
    }
    s32Ret = AX_VENC_Deinit();
    if (AX_SUCCESS != s32Ret)
    {
        ALOGE("AX_VENC_Deinit failed, s32Ret=0x%x", s32Ret);
        return s32Ret;
    }

    return 0;
}
