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
#include "../../utilities/sample_log.h"

#include "ax_ivps_api.h"
// #include "c_api.h"
// #include "npu_cv_kit/ax_npu_imgproc.h"

#include "unistd.h"
#include "string.h"
#include "pthread.h"

#ifndef ALIGN_UP
#define ALIGN_UP(x, align) ((((x) + ((align)-1)) / (align)) * (align))
#endif

void *_ivps_get_frame_thread(void *arg)
{
    pipeline_t *pipe = (pipeline_t *)arg;

    AX_S32 nMilliSec = 200;

    // prctl(PR_SET_NAME, "SAMPLE_IVPS_GET");

    // ALOGN("SAMPLE_RUN_JOINT +++");

    while (!pipe->n_loog_exit)
    {
        AX_VIDEO_FRAME_S tVideoFrame;

        AX_S32 ret = AX_IVPS_GetChnFrame(pipe->m_ivps_attr.n_ivps_grp, 0, &tVideoFrame, nMilliSec);

        if (0 != ret)
        {
            if (AX_ERR_IVPS_BUF_EMPTY == ret)
            {
                usleep(1000);
                continue;
            }
            usleep(1000);
            continue;
        }

        tVideoFrame.u64VirAddr[0] = (AX_U32)AX_POOL_GetBlockVirAddr(tVideoFrame.u32BlkId[0]);
        tVideoFrame.u64PhyAddr[0] = AX_POOL_Handle2PhysAddr(tVideoFrame.u32BlkId[0]);

        // ax_runner_image_t tSrcFrame = {0};
        // tSrcFrame.nWidth = tVideoFrame.u32Width;
        // tSrcFrame.nHeight = tVideoFrame.u32Height;
        // switch (tVideoFrame.enImgFormat)
        // {
        // case AX_YUV420_SEMIPLANAR:
        //     tSrcFrame.eDtype = AX_NPU_CV_FDT_NV12;
        //     tVideoFrame.u32FrameSize = tVideoFrame.u32PicStride[0] * tVideoFrame.u32Height * 3 / 2;
        //     break;
        // case AX_FORMAT_RGB888:
        //     tSrcFrame.eDtype = AX_NPU_CV_FDT_RGB;
        //     tVideoFrame.u32FrameSize = tVideoFrame.u32PicStride[0] * tVideoFrame.u32Height * 3;
        //     break;
        // case AX_FORMAT_BGR888:
        //     tSrcFrame.eDtype = AX_NPU_CV_FDT_BGR;
        //     tVideoFrame.u32FrameSize = tVideoFrame.u32PicStride[0] * tVideoFrame.u32Height * 3;
        //     break;
        // default:
        //     tSrcFrame.eDtype = AX_NPU_CV_FDT_UNKNOWN;
        //     break;
        // }
        // tSrcFrame.tStride.nW = (0 == tVideoFrame.u32PicStride[0]) ? tSrcFrame.nWidth : tVideoFrame.u32PicStride[0];
        // tSrcFrame.nSize = tVideoFrame.u32FrameSize; // t.tStride.nW * t.nHeight * 3 / 2;
        // tSrcFrame.pPhy = tVideoFrame.u64PhyAddr[0];
        // tSrcFrame.pVir = (AX_U8 *)tVideoFrame.u64VirAddr[0];

        if (pipe->output_func)
        {
            // printf("%d %d\n", tSrcFrame.nWidth, tSrcFrame.nHeight);
            pipeline_buffer_t buf;
            buf.pipeid = pipe->pipeid;
            buf.m_output_type = pipe->m_output_type;
            buf.n_width = tVideoFrame.u32Width;
            buf.n_height = tVideoFrame.u32Height;

            buf.n_stride = (0 == tVideoFrame.u32PicStride[0]) ? tVideoFrame.u32Width : tVideoFrame.u32PicStride[0];
            switch (tVideoFrame.enImgFormat)
            {
            case AX_YUV420_SEMIPLANAR:
                buf.n_size = tVideoFrame.u32PicStride[0] * tVideoFrame.u32Height * 3 / 2;
                buf.d_type = po_buff_nv12;
                break;
            case AX_FORMAT_RGB888:
                buf.n_size = tVideoFrame.u32PicStride[0] * tVideoFrame.u32Height * 3;
                buf.d_type = po_buff_rgb;
                break;
            case AX_FORMAT_BGR888:
                buf.n_size = tVideoFrame.u32PicStride[0] * tVideoFrame.u32Height * 3;
                buf.d_type = po_buff_bgr;
                break;
            default:
                buf.d_type = po_none;
                break;
            }
            buf.p_vir = (AX_U8 *)tVideoFrame.u64VirAddr[0];
            buf.p_phy = tVideoFrame.u64PhyAddr[0];
            buf.p_pipe = pipe;
            pipe->output_func(&buf);
        }

        ret = AX_IVPS_ReleaseChnFrame(pipe->m_ivps_attr.n_ivps_grp, 0, &tVideoFrame);
    }
    ALOGN("SAMPLE_RUN_JOINT ---");
    return (AX_VOID *)0;
}

int _create_ivps_grp(pipeline_t *pipe)
{
    if (pipe->m_ivps_attr.n_ivps_grp > MAX_IVPS_GRP_COUNT)
    {
        ALOGE("ivps_grp must lower than %d, got %d\n", MAX_IVPS_GRP_COUNT, pipe->m_ivps_attr.n_ivps_grp);
        return -1;
    }
    AX_S32 s32Ret = 0;
    AX_S32 nGrp = pipe->m_ivps_attr.n_ivps_grp, nChn = 0;
    AX_IVPS_GRP_ATTR_S stGrpAttr = {0};
    AX_IVPS_PIPELINE_ATTR_S stPipelineAttr = {0};

    stPipelineAttr.tFbInfo.PoolId = AX_INVALID_POOLID;
    stPipelineAttr.nOutChnNum = 1;

    stGrpAttr.nInFifoDepth = 1;
    stGrpAttr.ePipeline = AX_IVPS_PIPELINE_DEFAULT;
    s32Ret = AX_IVPS_CreateGrp(nGrp, &stGrpAttr);
    if (0 != s32Ret)
    {
        ALOGE("AX_IVPS_CreateGrp failed,nGrp %d,s32Ret:0x%x\n", nGrp, s32Ret);
        return s32Ret;
    }

    memset(&stPipelineAttr.tFilter, 0x00, sizeof(stPipelineAttr.tFilter));

    stPipelineAttr.tFilter[nChn][0].bEnable = AX_TRUE;
    stPipelineAttr.tFilter[nChn][0].tFRC.nSrcFrameRate = pipe->m_ivps_attr.n_ivps_fps;
    stPipelineAttr.tFilter[nChn][0].tFRC.nDstFrameRate = pipe->m_ivps_attr.n_ivps_fps;
    stPipelineAttr.tFilter[nChn][0].nDstPicOffsetX0 = 0;
    stPipelineAttr.tFilter[nChn][0].nDstPicOffsetY0 = 0;
    stPipelineAttr.tFilter[nChn][0].nDstPicWidth = pipe->m_ivps_attr.n_ivps_width;
    stPipelineAttr.tFilter[nChn][0].nDstPicHeight = pipe->m_ivps_attr.n_ivps_height;
    stPipelineAttr.tFilter[nChn][0].nDstPicStride = ALIGN_UP(stPipelineAttr.tFilter[nChn][0].nDstPicWidth, 64);
    stPipelineAttr.tFilter[nChn][0].nDstFrameWidth = pipe->m_ivps_attr.n_ivps_width;
    stPipelineAttr.tFilter[nChn][0].nDstFrameHeight = pipe->m_ivps_attr.n_ivps_height;
    stPipelineAttr.tFilter[nChn][0].eDstPicFormat = AX_YUV420_SEMIPLANAR;
    stPipelineAttr.tFilter[nChn][0].eEngine = AX_IVPS_ENGINE_TDP;

    if (pipe->m_ivps_attr.b_letterbox)
    {
        // letterbox filling image
        AX_IVPS_ASPECT_RATIO_S tAspectRatio;
        tAspectRatio.eMode = AX_IVPS_ASPECT_RATIO_AUTO;
        tAspectRatio.eAligns[0] = AX_IVPS_ASPECT_RATIO_HORIZONTAL_CENTER;
        tAspectRatio.eAligns[1] = AX_IVPS_ASPECT_RATIO_VERTICAL_CENTER;
        tAspectRatio.nBgColor = 0x0000FF;
        stPipelineAttr.tFilter[nChn][0].tTdpCfg.tAspectRatio = tAspectRatio;
    }

    stPipelineAttr.tFilter[nChn][0].tTdpCfg.bFlip = pipe->m_ivps_attr.b_ivps_flip > 0 ? AX_TRUE : AX_FALSE;
    stPipelineAttr.tFilter[nChn][0].tTdpCfg.bMirror = pipe->m_ivps_attr.b_ivps_mirror > 0 ? AX_TRUE : AX_FALSE;
    stPipelineAttr.tFilter[nChn][0].tTdpCfg.eRotation = (AX_IVPS_ROTATION_E)pipe->m_ivps_attr.n_ivps_rotate;

    switch (stPipelineAttr.tFilter[nChn][0].tTdpCfg.eRotation)
    {
    case AX_IVPS_ROTATION_90:
    case AX_IVPS_ROTATION_270:
        stPipelineAttr.tFilter[nChn][0].nDstPicWidth = pipe->m_ivps_attr.n_ivps_height;
        stPipelineAttr.tFilter[nChn][0].nDstPicHeight = pipe->m_ivps_attr.n_ivps_width;
        stPipelineAttr.tFilter[nChn][0].nDstPicStride = ALIGN_UP(stPipelineAttr.tFilter[nChn][0].nDstPicWidth, 64);
        stPipelineAttr.tFilter[nChn][0].nDstFrameWidth = pipe->m_ivps_attr.n_ivps_height;
        stPipelineAttr.tFilter[nChn][0].nDstFrameHeight = pipe->m_ivps_attr.n_ivps_width;
        break;

    default:
        break;
    }

    switch (pipe->m_output_type)
    {
    case po_buff_rgb:
        stPipelineAttr.tFilter[nChn][0].eDstPicFormat = AX_FORMAT_RGB888;
        break;
    case po_buff_bgr:
        stPipelineAttr.tFilter[nChn][0].eDstPicFormat = AX_FORMAT_BGR888;
        break;
    case po_buff_nv21:
        stPipelineAttr.tFilter[nChn][0].eDstPicFormat = AX_YUV420_SEMIPLANAR_VU;
        break;
    case po_buff_nv12:
    case po_venc_mjpg:
    case po_venc_h264:
    case po_venc_h265:
    case po_rtsp_h264:
    case po_rtsp_h265:
    case po_vo_sipeed_maix3_screen:
        stPipelineAttr.tFilter[nChn][0].eDstPicFormat = AX_YUV420_SEMIPLANAR;
        break;
    default:
        break;
    }

    stPipelineAttr.nOutFifoDepth[nChn] = pipe->m_ivps_attr.n_fifo_count;
    if (stPipelineAttr.nOutFifoDepth[nChn] < 0)
        stPipelineAttr.nOutFifoDepth[nChn] = 0;
    if (stPipelineAttr.nOutFifoDepth[nChn] > 4)
        stPipelineAttr.nOutFifoDepth[nChn] = 4;

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

    s32Ret = AX_IVPS_StartGrp(nGrp);
    if (0 != s32Ret)
    {
        ALOGE("AX_IVPS_StartGrp failed,nGrp %d,s32Ret:0x%x\n", nGrp, s32Ret);
        return s32Ret;
    }

    for (int i = 0; i < pipe->m_ivps_attr.n_osd_rgn && i < MAX_OSD_RGN_COUNT; i++)
    {
        IVPS_RGN_HANDLE hChnRgn = AX_IVPS_RGN_Create();
        if (AX_IVPS_INVALID_REGION_HANDLE != hChnRgn)
        {
            AX_S32 nFilter = 0x00;
            int nRet = AX_IVPS_RGN_AttachToFilter(hChnRgn, pipe->m_ivps_attr.n_ivps_grp, nFilter);
            if (0 != nRet)
            {
                ALOGE("AX_IVPS_RGN_AttachToFilter(Grp: %d, Filter: 0x%x) failed, ret=0x%x", pipe->m_ivps_attr.n_ivps_grp, nFilter, nRet);
                pipe->m_ivps_attr.n_osd_rgn = i;
                break;
            }
            pipe->m_ivps_attr.n_osd_rgn_chn[i] = hChnRgn;
            continue;
        }
        pipe->m_ivps_attr.n_osd_rgn = i;
        break;
    }

    switch (pipe->m_output_type)
    {
    case po_buff_rgb:
    case po_buff_bgr:
    case po_buff_nv12:
    case po_buff_nv21:
        if (stPipelineAttr.nOutFifoDepth[nChn] > 0)
        {
            if (0 != pthread_create(&pipe->m_ivps_attr.tid, NULL, _ivps_get_frame_thread, pipe))
            {
                return -1;
            }
        }
        else
        {

            ALOGW("m_output_type set po_buff mode,but pipe->m_ivps_attr.n_fifo_count got %d", pipe->m_ivps_attr.n_fifo_count);
        }
        break;
    default:
        break;
    }

    return 0;
}

int _destore_ivps_grp(pipeline_t *pipe)
{
    AX_S32 s32Ret = 0;
    pthread_join(pipe->m_ivps_attr.tid, NULL);

    s32Ret = AX_IVPS_StopGrp(pipe->m_ivps_attr.n_ivps_grp);
    if (0 != s32Ret)
    {
        ALOGE("AX_IVPS_StopGrp failed,nGrp %d,s32Ret:0x%x\n", pipe->m_ivps_attr.n_ivps_grp, s32Ret);
        return s32Ret;
    }

    s32Ret = AX_IVPS_DisableChn(pipe->m_ivps_attr.n_ivps_grp, 0);
    if (0 != s32Ret)
    {
        ALOGE("AX_IVPS_DisableChn failed,nGrp %d,nChn %d,s32Ret:0x%x\n", pipe->m_ivps_attr.n_ivps_grp, 0, s32Ret);
        return s32Ret;
    }

    s32Ret = AX_IVPS_DestoryGrp(pipe->m_ivps_attr.n_ivps_grp);
    if (0 != s32Ret)
    {
        ALOGE("AX_IVPS_DestoryGrp failed,nGrp %d,s32Ret:0x%x", pipe->m_ivps_attr.n_ivps_grp, s32Ret);
        return s32Ret;
    }
    return 0;
}
