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
    int s32Ret;

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

static AX_VOID SAMPLE_SetDefaultVencParams(SAMPLE_VENC_CMD_PARA_T *pstPara, AX_U16 width, AX_U16 height)
{
    pstPara->bSaveStrm = AX_FALSE;
    // pstPara->output = "stream.h264";

    pstPara->picW = width;
    pstPara->picH = height;
    pstPara->maxPicW = width;
    pstPara->maxPicH = height;
    pstPara->rcMode = SAMPLE_RC_VBR;
    pstPara->chnNum = 1;
    pstPara->srcFrameRate = 30;
    pstPara->dstFrameRate = 30;
    pstPara->maxIprop = 10;
    pstPara->minIprop = 10;

    pstPara->gopLen = 30;
    pstPara->virILen = pstPara->gopLen / 2;
    pstPara->bitRate = width * height * 3 / 1024;
    pstPara->qpMin = 10;
    pstPara->qpMax = 51;
    pstPara->qpMinI = 10;
    pstPara->qpMaxI = 51;
    pstPara->bLinkMode = AX_TRUE;
    pstPara->startQp = -1;
}

int _create_venc_chn(pipeline_t *pipe)
{
    if (pipe->m_venc_attr.n_venc_chn > MAX_VENC_CHN_COUNT)
    {
        ALOGE("venc_chn must lower than %d, got %d\n", MAX_VENC_CHN_COUNT, pipe->m_venc_attr.n_venc_chn);
        return -1;
    }

    SAMPLE_VENC_CMD_PARA_T stCmdPara;

    memset(&stCmdPara, 0x0, sizeof(SAMPLE_VENC_CMD_PARA_T));
    SAMPLE_SetDefaultVencParams(&stCmdPara, pipe->m_ivps_attr.n_ivps_width, pipe->m_ivps_attr.n_ivps_height);
    auto enRcMode = SAMPLE_RC_VBR;
    AX_PAYLOAD_TYPE_E enType = PT_H264;
    switch (pipe->m_output_type)
    {
    case po_venc_h264:
    case po_rtsp_h264:
        enType = PT_H264;
        break;
    case po_venc_h265:
    case po_rtsp_h265:
        enType = PT_H265;
        break;
    case po_venc_mjpg:
        enType = PT_MJPEG;
        break;
    default:
        ALOGE("pipeline_output_e=%d,should not init venc");
        return -1;
    }

    auto s32Ret = COMMON_VENC_Start(pipe->m_venc_attr.n_venc_chn, enType, enRcMode, &stCmdPara);
    if (AX_SUCCESS != s32Ret)
    {
        ALOGE("VencChn %d: COMMON_VENC_Start failed, s32Ret:0x%x", pipe->m_venc_attr.n_venc_chn, s32Ret);
        return -1;
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
