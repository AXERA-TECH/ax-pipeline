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

#include "../common_pipeline.h"
// #include "../../rtsp/inc/rtsp.h"
#include "../../../third-party/RtspServer/RtspServerWarpper.h"
#include "../../utilities/sample_log.h"

#include "ax_venc_api.h"
#include "ax_ivps_api.h"
extern "C"
{
#include "common_venc.h"
}

bool check_rtsp_session_pipeid(int pipeid);
rtsp_server_t get_rtsp_demo_handle();
rtsp_session_t get_rtsp_session_handle(int pipeid);

void *_venc_get_frame_thread(void *arg)
{
    pipeline_t *pipe = (pipeline_t *)arg;
    AX_S16 syncType = 200;
    AX_VENC_STREAM_T stStream = {0};
    AX_VENC_RECV_PIC_PARAM_T stRecvParam;
    stRecvParam.s32RecvPicNum = 1;
    int s32Ret = AX_VENC_StartRecvFrame(pipe->m_venc_attr.n_venc_chn, &stRecvParam);
    if (AX_SUCCESS != s32Ret)
    {
        ALOGE("AX_VENC_StartRecvFrame failed, s32Ret:0x%x\n", s32Ret);
        return NULL;
    }

    while (!pipe->n_loog_exit)
    {
        s32Ret = AX_VENC_GetStream(pipe->m_venc_attr.n_venc_chn, &stStream, syncType);
        // printf("%d\n",stStream.stPack.u32Len);
        if (AX_SUCCESS == s32Ret)
        {
            switch (pipe->m_output_type)
            {
            case po_rtsp_h264:
            case po_rtsp_h265:
            {
                if (check_rtsp_session_pipeid(pipe->pipeid))
                {
                    rtsp_buffer_t buff = {0};
                    buff.vlen = stStream.stPack.u32Len;
                    buff.vbuff = stStream.stPack.pu8Addr;
                    buff.vts = stStream.stPack.u64PTS;
                    rtsp_push(get_rtsp_demo_handle(), get_rtsp_session_handle(pipe->pipeid), &buff);
                }
            }
            break;
            default:
                break;
            }

            if (pipe->output_func)
            {
                pipeline_buffer_t buf;
                buf.pipeid = pipe->pipeid;
                buf.m_output_type = pipe->m_output_type;
                buf.n_width = 0;
                buf.n_height = 0;
                buf.n_size = stStream.stPack.u32Len;
                buf.n_stride = 0;
                buf.d_type = po_none;
                buf.p_vir = stStream.stPack.pu8Addr;
                buf.p_phy = stStream.stPack.ulPhyAddr;
                buf.p_pipe = pipe;
                pipe->output_func(&buf);
            }

            s32Ret = AX_VENC_ReleaseStream(pipe->m_venc_attr.n_venc_chn, &stStream);
            if (s32Ret)
            {
                ALOGE("VencChn %d: AX_VENC_ReleaseStream failed!s32Ret:0x%x\n", pipe->m_venc_attr.n_venc_chn, s32Ret);
                usleep(30 * 1000);
            }
        }
        else
        {
            ALOGE("VencChn %d: AX_VENC_GetStream failed!s32Ret:0x%x\n", pipe->m_venc_attr.n_venc_chn, s32Ret);
            usleep(30 * 1000);
        }
    }

EXIT:
    ALOGN("VencChn %d: getStream Exit!\n", pipe->m_venc_attr.n_venc_chn);
    return NULL;
}

AX_BOOL set_jpeg_param(pipeline_t *pipe)
{
    AX_S32 s32Ret = 0;
    AX_VENC_JPEG_PARAM_T stJpegParam;
    memset(&stJpegParam, 0, sizeof(stJpegParam));
    s32Ret = AX_VENC_GetJpegParam(pipe->m_venc_attr.n_venc_chn, &stJpegParam);
    if (AX_SUCCESS != s32Ret)
    {
        printf("AX_VENC_GetJpegParam:%d failed, error type 0x%x!\n", pipe->m_venc_attr.n_venc_chn, s32Ret);
        return AX_FALSE;
    }

    stJpegParam.u32Qfactor = 90;
    /* Use user set qtable. Qtable example */
    // if (gS32QTableEnable)
    // {
    //     memcpy(stJpegParam.u8YQt, QTableLuminance, sizeof(QTableLuminance));
    //     memcpy(stJpegParam.u8CbCrQt, QTableChrominance, sizeof(QTableChrominance));
    // }

    s32Ret = AX_VENC_SetJpegParam(pipe->m_venc_attr.n_venc_chn, &stJpegParam);
    if (AX_SUCCESS != s32Ret)
    {
        printf("AX_VENC_SetJpegParam:%d failed, error type 0x%x!\n", pipe->m_venc_attr.n_venc_chn, s32Ret);
        return AX_FALSE;
    }

    return AX_TRUE;
}

AX_BOOL set_rc_param(pipeline_t *pipe, AX_VENC_RC_MODE_E enRcMode)
{
    // #ifdef VIDEO_ENABLE_RC_DYNAMIC
    AX_S32 s32Ret = 0;
    AX_VENC_RC_PARAM_T stRcParam;

    s32Ret = AX_VENC_GetRcParam(pipe->m_venc_attr.n_venc_chn, &stRcParam);
    if (AX_SUCCESS != s32Ret)
    {
        printf("AX_VENC_GetRcParam:%d failed, error type 0x%x!\n", pipe->m_venc_attr.n_venc_chn, s32Ret);
        return AX_FALSE;
    }

    if (enRcMode == AX_VENC_RC_MODE_MJPEGCBR)
    {
        stRcParam.stMjpegCbr.u32BitRate = 4000;
        stRcParam.stMjpegCbr.u32MinQp = 20;
        stRcParam.stMjpegCbr.u32MaxQp = 30;
    }
    else if (enRcMode == AX_VENC_RC_MODE_MJPEGVBR)
    {
        stRcParam.stMjpegVbr.u32MaxBitRate = 4000;
        stRcParam.stMjpegVbr.u32MinQp = 20;
        stRcParam.stMjpegVbr.u32MaxQp = 30;
    }
    else if (enRcMode == AX_VENC_RC_MODE_MJPEGFIXQP)
    {
        stRcParam.stMjpegFixQp.s32FixedQp = 22;
    }

    s32Ret = AX_VENC_SetRcParam(pipe->m_venc_attr.n_venc_chn, &stRcParam);
    if (AX_SUCCESS != s32Ret)
    {
        printf("AX_VENC_SetRcParam:%d failed, error type 0x%x!\n", pipe->m_venc_attr.n_venc_chn, s32Ret);
        return AX_FALSE;
    }

    // #endif
    return AX_TRUE;
}

int _create_venc_chn(pipeline_t *pipe)
{
    if (pipe->m_venc_attr.n_venc_chn > MAX_VENC_CHN_COUNT)
    {
        ALOGE("venc_chn must lower than %d, got %d\n", MAX_VENC_CHN_COUNT, pipe->m_venc_attr.n_venc_chn);
        return -1;
    }
    typedef struct _stRCInfo
    {
        SAMPLE_VENC_RC_E eRCType;
        AX_U32 nMinQp;
        AX_U32 nMaxQp;
        AX_U32 nMinIQp;
        AX_U32 nMaxIQp;
        AX_S32 nIntraQpDelta;
    } RC_INFO_T;
    typedef struct _stVideoConfig
    {
        AX_PAYLOAD_TYPE_E ePayloadType;
        AX_U32 nGOP;
        AX_U32 nSrcFrameRate;
        AX_U32 nDstFrameRate;
        AX_U32 nStride;
        AX_S32 nInWidth;
        AX_S32 nInHeight;
        AX_S32 nOutWidth;
        AX_S32 nOutHeight;
        AX_S32 nOffsetCropX;
        AX_S32 nOffsetCropY;
        AX_S32 nOffsetCropW;
        AX_S32 nOffsetCropH;
        AX_IMG_FORMAT_E eImgFormat;
        RC_INFO_T stRCInfo;
        AX_S32 nBitrate;
    } VIDEO_CONFIG_T;
    AX_VENC_CHN_ATTR_T stVencChnAttr;
    VIDEO_CONFIG_T config;
    memset(&config, 0, sizeof(VIDEO_CONFIG_T));
    // AX_S32 s32Ret = 0;

    config.stRCInfo.eRCType = SAMPLE_RC_VBR;
    config.nGOP = 50;
    config.nBitrate = pipe->m_ivps_attr.n_ivps_width * pipe->m_ivps_attr.n_ivps_height * 3 / 1024;
    config.stRCInfo.nMinQp = 10;
    config.stRCInfo.nMaxQp = 51;
    config.stRCInfo.nMinIQp = 10;
    config.stRCInfo.nMaxIQp = 51;
    config.stRCInfo.nIntraQpDelta = -2;
    config.nOffsetCropX = 0;
    config.nOffsetCropY = 0;
    config.nOffsetCropW = 0;
    config.nOffsetCropH = 0;
    switch (pipe->m_output_type)
    {
    case po_venc_h264:
    case po_rtsp_h264:
        config.ePayloadType = PT_H264;
        break;
    case po_venc_h265:
    case po_rtsp_h265:
        config.ePayloadType = PT_H265;
        break;
    case po_venc_mjpg:
        config.ePayloadType = PT_MJPEG;
        break;
    default:
        // ALOGE("pipeline_output_e=%d,should not init venc");
        return -1;
    }

    config.nInWidth = pipe->m_ivps_attr.n_ivps_width;
    config.nInHeight = pipe->m_ivps_attr.n_ivps_height;
    config.nStride = config.nInWidth;

    switch (pipe->m_ivps_attr.n_ivps_rotate)
    {
    case AX_IVPS_ROTATION_90:
    case AX_IVPS_ROTATION_270:
        config.nInWidth = pipe->m_ivps_attr.n_ivps_height;
        config.nInHeight = pipe->m_ivps_attr.n_ivps_width;
        config.nStride = config.nInWidth;
        break;

    default:
        break;
    }

    config.nSrcFrameRate = pipe->m_ivps_attr.n_ivps_fps;
    config.nDstFrameRate = pipe->m_ivps_attr.n_ivps_fps;

    memset(&stVencChnAttr, 0, sizeof(AX_VENC_CHN_ATTR_T));

    stVencChnAttr.stVencAttr.u32MaxPicWidth = config.nInWidth;
    stVencChnAttr.stVencAttr.u32MaxPicHeight = config.nInHeight;

    stVencChnAttr.stVencAttr.u32PicWidthSrc = config.nInWidth;   /*the picture width*/
    stVencChnAttr.stVencAttr.u32PicHeightSrc = config.nInHeight; /*the picture height*/
    stVencChnAttr.stVencAttr.stCropCfg.bEnable = AX_FALSE;
    // stVencChnAttr.stVencAttr.u32CropOffsetX = config.nOffsetCropX;
    // stVencChnAttr.stVencAttr.u32CropOffsetY = config.nOffsetCropY;
    // stVencChnAttr.stVencAttr.u32CropWidth = config.nOffsetCropW;
    // stVencChnAttr.stVencAttr.u32CropHeight = config.nOffsetCropH;
    stVencChnAttr.stVencAttr.u32VideoRange = 1; /* 0: Narrow Range(NR), Y[16,235], Cb/Cr[16,240]; 1: Full Range(FR), Y/Cb/Cr[0,255] */

    // ALOGN("VencChn %d:w:%d, h:%d, s:%d, Crop:(%d, %d, %d, %d) rcType:%d, payload:%d", gVencChnMapping[VencChn], stVencChnAttr.stVencAttr.u32PicWidthSrc, stVencChnAttr.stVencAttr.u32PicHeightSrc, config.nStride, stVencChnAttr.stVencAttr.u32CropOffsetX, stVencChnAttr.stVencAttr.u32CropOffsetY, stVencChnAttr.stVencAttr.u32CropWidth, stVencChnAttr.stVencAttr.u32CropHeight, config.stRCInfo.eRCType, config.ePayloadType);

    stVencChnAttr.stVencAttr.u32BufSize = config.nStride * config.nInHeight * 3 / 2; /*stream buffer size*/
    // stVencChnAttr.stVencAttr.u32MbLinesPerSlice = 0;                                 /*get stream mode is slice mode or frame mode?*/
    stVencChnAttr.stVencAttr.enLinkMode = AX_VENC_LINK_MODE;
    // stVencChnAttr.stVencAttr.u32GdrDuration = 0;
    /* GOP Setting */
    stVencChnAttr.stGopAttr.enGopMode = AX_VENC_GOPMODE_NORMALP;
    stVencChnAttr.stVencAttr.u8InFifoDepth = 5;
    stVencChnAttr.stVencAttr.u8OutFifoDepth = 5;

    stVencChnAttr.stVencAttr.enType = config.ePayloadType;

#if 0                                                                          // V1.27
    stVencChnAttr.stRcAttr.uFrameRate.tFrmRateCtrl.nSrcFrameRate = AX_FRAME_RATE(config.nSrcFrameRate);  /* input frame rate */
    stVencChnAttr.stRcAttr.uFrameRate.tFrmRateCtrl.nDstFrameRate = AX_FRAME_RATE(config.nDstFrameRate); /* target frame rate */
#else                                                                          // V1.40
    stVencChnAttr.stRcAttr.stFrameRate.fSrcFrameRate = (config.nSrcFrameRate); /* input frame rate */
    stVencChnAttr.stRcAttr.stFrameRate.fDstFrameRate = (config.nDstFrameRate); /* target frame rate */
#endif

    switch (stVencChnAttr.stVencAttr.enType)
    {
    case PT_H265:
    {
        stVencChnAttr.stVencAttr.enProfile = AX_VENC_HEVC_MAIN_PROFILE;
        stVencChnAttr.stVencAttr.enLevel = AX_VENC_HEVC_LEVEL_6;
        stVencChnAttr.stVencAttr.enTier = AX_VENC_HEVC_MAIN_TIER;

        if (config.stRCInfo.eRCType == SAMPLE_RC_CBR)
        {
            AX_VENC_H265_CBR_T stH265Cbr;
            memset(&stH265Cbr, 0, sizeof(stH265Cbr));
            stVencChnAttr.stRcAttr.enRcMode = AX_VENC_RC_MODE_H265CBR;
            stVencChnAttr.stRcAttr.s32FirstFrameStartQp = -1;
            stH265Cbr.u32Gop = config.nGOP;
            // stH265Cbr.u32SrcFrameRate = config.nSrcFrameRate;  /* input frame rate */
            // stH265Cbr.fr32DstFrameRate = config.nDstFrameRate; /* target frame rate */
            stH265Cbr.u32BitRate = config.nBitrate;
            stH265Cbr.u32MinQp = config.stRCInfo.nMinQp;
            stH265Cbr.u32MaxQp = config.stRCInfo.nMaxQp;
            stH265Cbr.u32MinIQp = config.stRCInfo.nMinIQp;
            stH265Cbr.u32MaxIQp = config.stRCInfo.nMaxIQp;
            stH265Cbr.s32IntraQpDelta = config.stRCInfo.nIntraQpDelta;
            memcpy(&stVencChnAttr.stRcAttr.stH265Cbr, &stH265Cbr, sizeof(AX_VENC_H265_CBR_T));
        }
        else if (config.stRCInfo.eRCType == SAMPLE_RC_VBR)
        {
            AX_VENC_H265_VBR_T stH265Vbr;
            memset(&stH265Vbr, 0, sizeof(stH265Vbr));
            stVencChnAttr.stRcAttr.enRcMode = AX_VENC_RC_MODE_H265VBR;
            stVencChnAttr.stRcAttr.s32FirstFrameStartQp = -1;
            stH265Vbr.u32Gop = config.nGOP;
            // stH265Vbr.u32SrcFrameRate = config.nSrcFrameRate;
            // stH265Vbr.fr32DstFrameRate = config.nDstFrameRate;
            stH265Vbr.u32MaxBitRate = config.nBitrate;
            stH265Vbr.u32MinQp = config.stRCInfo.nMinQp;
            stH265Vbr.u32MaxQp = config.stRCInfo.nMaxQp;
            stH265Vbr.u32MinIQp = config.stRCInfo.nMinIQp;
            stH265Vbr.u32MaxIQp = config.stRCInfo.nMaxIQp;
            stH265Vbr.s32IntraQpDelta = config.stRCInfo.nIntraQpDelta;
            memcpy(&stVencChnAttr.stRcAttr.stH265Vbr, &stH265Vbr, sizeof(AX_VENC_H265_VBR_T));
        }
        else if (config.stRCInfo.eRCType == SAMPLE_RC_FIXQP)
        {
            AX_VENC_H265_FIXQP_T stH265FixQp;
            memset(&stH265FixQp, 0, sizeof(stH265FixQp));
            stVencChnAttr.stRcAttr.enRcMode = AX_VENC_RC_MODE_H265FIXQP;
            stH265FixQp.u32Gop = config.nGOP;
            // stH265FixQp.u32SrcFrameRate = config.nSrcFrameRate;
            // stH265FixQp.fr32DstFrameRate = config.nDstFrameRate;
            stH265FixQp.u32IQp = 25;
            stH265FixQp.u32PQp = 30;
            stH265FixQp.u32BQp = 32;
            memcpy(&stVencChnAttr.stRcAttr.stH265FixQp, &stH265FixQp, sizeof(AX_VENC_H265_FIXQP_T));
        }
        break;
    }
    case PT_H264:
    {
        stVencChnAttr.stVencAttr.enProfile = AX_VENC_H264_MAIN_PROFILE;
        stVencChnAttr.stVencAttr.enLevel = AX_VENC_H264_LEVEL_5_2;

        if (config.stRCInfo.eRCType == SAMPLE_RC_CBR)
        {
            AX_VENC_H264_CBR_T stH264Cbr;
            memset(&stH264Cbr, 0, sizeof(stH264Cbr));
            stVencChnAttr.stRcAttr.enRcMode = AX_VENC_RC_MODE_H264CBR;
            stVencChnAttr.stRcAttr.s32FirstFrameStartQp = -1;
            stH264Cbr.u32Gop = config.nGOP;
            // stH264Cbr.u32SrcFrameRate = config.nSrcFrameRate;  /* input frame rate */
            // stH264Cbr.fr32DstFrameRate = config.nDstFrameRate; /* target frame rate */
            stH264Cbr.u32BitRate = config.nBitrate;
            stH264Cbr.u32MinQp = config.stRCInfo.nMinQp;
            stH264Cbr.u32MaxQp = config.stRCInfo.nMaxQp;
            stH264Cbr.u32MinIQp = config.stRCInfo.nMinIQp;
            stH264Cbr.u32MaxIQp = config.stRCInfo.nMaxIQp;
            stH264Cbr.s32IntraQpDelta = config.stRCInfo.nIntraQpDelta;
            memcpy(&stVencChnAttr.stRcAttr.stH264Cbr, &stH264Cbr, sizeof(AX_VENC_H264_CBR_T));
        }
        else if (config.stRCInfo.eRCType == SAMPLE_RC_VBR)
        {
            AX_VENC_H264_VBR_T stH264Vbr;
            memset(&stH264Vbr, 0, sizeof(stH264Vbr));
            stVencChnAttr.stRcAttr.enRcMode = AX_VENC_RC_MODE_H264VBR;
            stVencChnAttr.stRcAttr.s32FirstFrameStartQp = -1;
            stH264Vbr.u32Gop = config.nGOP;
            // stH264Vbr.u32SrcFrameRate = config.nSrcFrameRate;
            // stH264Vbr.fr32DstFrameRate = config.nDstFrameRate;
            stH264Vbr.u32MaxBitRate = config.nBitrate;
            stH264Vbr.u32MinQp = config.stRCInfo.nMinQp;
            stH264Vbr.u32MaxQp = config.stRCInfo.nMaxQp;
            stH264Vbr.u32MinIQp = config.stRCInfo.nMinIQp;
            stH264Vbr.u32MaxIQp = config.stRCInfo.nMaxIQp;
            stH264Vbr.s32IntraQpDelta = config.stRCInfo.nIntraQpDelta;
            memcpy(&stVencChnAttr.stRcAttr.stH264Vbr, &stH264Vbr, sizeof(AX_VENC_H264_VBR_T));
        }
        else if (config.stRCInfo.eRCType == SAMPLE_RC_FIXQP)
        {
            AX_VENC_H264_FIXQP_T stH264FixQp;
            memset(&stH264FixQp, 0, sizeof(stH264FixQp));
            stVencChnAttr.stRcAttr.enRcMode = AX_VENC_RC_MODE_H264FIXQP;
            stH264FixQp.u32Gop = config.nGOP;
            // stH264FixQp.u32SrcFrameRate = config.nSrcFrameRate;
            // stH264FixQp.fr32DstFrameRate = config.nDstFrameRate;
            stH264FixQp.u32IQp = 25;
            stH264FixQp.u32PQp = 30;
            stH264FixQp.u32BQp = 32;
            memcpy(&stVencChnAttr.stRcAttr.stH264FixQp, &stH264FixQp, sizeof(AX_VENC_H264_FIXQP_T));
        }
        break;
    }
    case PT_MJPEG:
    {
        if (config.stRCInfo.eRCType == SAMPLE_RC_CBR)
        {
            AX_VENC_MJPEG_CBR_T stMjpegCbrAttr;
            memset(&stMjpegCbrAttr, 0, sizeof(stMjpegCbrAttr));
            stVencChnAttr.stRcAttr.enRcMode = AX_VENC_RC_MODE_MJPEGCBR;
            // stVencChnAttr.stRcAttr.s32FirstFrameStartQp = -1;

            stMjpegCbrAttr.u32StatTime = 1;
            // stMjpegCbrAttr.u32SrcFrameRate = config.nSrcFrameRate;
            // stMjpegCbrAttr.fr32DstFrameRate = config.nDstFrameRate;
            stMjpegCbrAttr.u32BitRate = 4000;
            stMjpegCbrAttr.u32MinQp = 20;
            stMjpegCbrAttr.u32MaxQp = 30;
            memcpy(&stVencChnAttr.stRcAttr.stMjpegCbr, &stMjpegCbrAttr, sizeof(AX_VENC_MJPEG_CBR_T));
        }
        else if (config.stRCInfo.eRCType == SAMPLE_RC_VBR)
        {
            AX_VENC_MJPEG_VBR_T stMjpegVbrAttr;
            memset(&stMjpegVbrAttr, 0, sizeof(stMjpegVbrAttr));
            stVencChnAttr.stRcAttr.enRcMode = AX_VENC_RC_MODE_MJPEGVBR;
            // stVencChnAttr.stRcAttr.s32FirstFrameStartQp = -1;
            stMjpegVbrAttr.u32StatTime = 1;
            // stMjpegVbrAttr.u32SrcFrameRate = config.nSrcFrameRate;
            // stMjpegVbrAttr.fr32DstFrameRate = config.nDstFrameRate;
            stMjpegVbrAttr.u32MaxBitRate = 4000;
            stMjpegVbrAttr.u32MinQp = 20;
            stMjpegVbrAttr.u32MaxQp = 30;
            memcpy(&stVencChnAttr.stRcAttr.stMjpegVbr, &stMjpegVbrAttr, sizeof(AX_VENC_MJPEG_VBR_T));
        }
        else if (config.stRCInfo.eRCType == SAMPLE_RC_FIXQP)
        {
            AX_VENC_MJPEG_FIXQP_T stMjpegFixQpAttr;
            memset(&stMjpegFixQpAttr, 0, sizeof(stMjpegFixQpAttr));
            stVencChnAttr.stRcAttr.enRcMode = AX_VENC_RC_MODE_MJPEGFIXQP;

            // stMjpegFixQpAttr.u32SrcFrameRate = config.nSrcFrameRate;
            // stMjpegFixQpAttr.fr32DstFrameRate = config.nDstFrameRate;
            stMjpegFixQpAttr.s32FixedQp = 22;
            memcpy(&stVencChnAttr.stRcAttr.stMjpegFixQp, &stMjpegFixQpAttr, sizeof(AX_VENC_MJPEG_FIXQP_T));
        }
        break;
    }
    default:
        ALOGE("VencChn %d:Payload type unrecognized.", pipe->m_venc_attr.n_venc_chn);
        return -1;
    }

    AX_S32 ret = AX_VENC_CreateChn(pipe->m_venc_attr.n_venc_chn, &stVencChnAttr);
    if (AX_SUCCESS != ret)
    {
        ALOGE("VencChn %d: AX_VENC_CreateChn failed, s32Ret:0x%x", pipe->m_venc_attr.n_venc_chn, ret);
        return -1;
    }

    if (pipe->m_output_type == po_venc_mjpg)
    {
        set_rc_param(pipe, stVencChnAttr.stRcAttr.enRcMode);
        set_jpeg_param(pipe);
    }

    if (0 != pthread_create(&pipe->m_venc_attr.tid, NULL, _venc_get_frame_thread, pipe))
    {
        return -1;
    }

    return 0;
}

int _destore_venc_grp(pipeline_t *pipe)
{
    AX_S32 s32Ret = 0;
    if (pipe->m_venc_attr.tid)
    {
        pthread_join(pipe->m_venc_attr.tid, NULL);
    }

    s32Ret = AX_VENC_StopRecvFrame(pipe->m_venc_attr.n_venc_chn);
    if (0 != s32Ret)
    {
        ALOGE("VencChn %d:AX_VENC_StopRecvFrame failed,s32Ret:0x%x.\n", pipe->m_venc_attr.n_venc_chn, s32Ret);
        return s32Ret;
    }

    s32Ret = AX_VENC_DestroyChn(pipe->m_venc_attr.n_venc_chn);
    if (0 != s32Ret)
    {
        ALOGE("VencChn %d:AX_VENC_DestroyChn failed,s32Ret:0x%x.\n", pipe->m_venc_attr.n_venc_chn, s32Ret);
        return s32Ret;
    }
    return 0;
}
