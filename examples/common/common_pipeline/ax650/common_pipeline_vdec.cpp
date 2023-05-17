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
#include "ax_vdec_api.h"
#include "ax_buffer_tool.h"

#include "unistd.h"
#include "string.h"
#include "pthread.h"

extern "C"
{
    // #include "common_vdec_utils.h"
}

#define AX_DEC_RET_STR_CASE(s32Ret) \
    case (s32Ret):                  \
        return (#s32Ret)

const char *AX_VdecRetStr(AX_S32 s32Ret)
{
    switch (s32Ret)
    {
        AX_DEC_RET_STR_CASE(AX_SUCCESS);
        AX_DEC_RET_STR_CASE(AX_ERR_VDEC_INVALID_GRPID);
        AX_DEC_RET_STR_CASE(AX_ERR_VDEC_INVALID_CHNID);
        AX_DEC_RET_STR_CASE(AX_ERR_VDEC_ILLEGAL_PARAM);
        AX_DEC_RET_STR_CASE(AX_ERR_VDEC_NULL_PTR);
        AX_DEC_RET_STR_CASE(AX_ERR_VDEC_BAD_ADDR);
        AX_DEC_RET_STR_CASE(AX_ERR_VDEC_SYS_NOTREADY);
        AX_DEC_RET_STR_CASE(AX_ERR_VDEC_BUSY);
        AX_DEC_RET_STR_CASE(AX_ERR_VDEC_NOT_INIT);
        AX_DEC_RET_STR_CASE(AX_ERR_VDEC_NOT_CONFIG);
        AX_DEC_RET_STR_CASE(AX_ERR_VDEC_NOT_SUPPORT);
        AX_DEC_RET_STR_CASE(AX_ERR_VDEC_NOT_PERM);
        AX_DEC_RET_STR_CASE(AX_ERR_VDEC_EXIST);
        AX_DEC_RET_STR_CASE(AX_ERR_VDEC_UNEXIST);
        AX_DEC_RET_STR_CASE(AX_ERR_VDEC_NOMEM);
        AX_DEC_RET_STR_CASE(AX_ERR_VDEC_NOBUF);
        AX_DEC_RET_STR_CASE(AX_ERR_VDEC_BUF_EMPTY);
        AX_DEC_RET_STR_CASE(AX_ERR_VDEC_BUF_FULL);
        AX_DEC_RET_STR_CASE(AX_ERR_VDEC_QUEUE_EMPTY);
        AX_DEC_RET_STR_CASE(AX_ERR_VDEC_QUEUE_FULL);
        AX_DEC_RET_STR_CASE(AX_ERR_VDEC_TIMED_OUT);
        AX_DEC_RET_STR_CASE(AX_ERR_VDEC_FLOW_END);
        AX_DEC_RET_STR_CASE(AX_ERR_VDEC_UNKNOWN);
        AX_DEC_RET_STR_CASE(AX_ERR_VDEC_RUN_ERROR);
        AX_DEC_RET_STR_CASE(AX_ERR_VDEC_STRM_ERROR);
        AX_DEC_RET_STR_CASE(AX_ERR_POOL_UNEXIST);
        AX_DEC_RET_STR_CASE(AX_ERR_POOL_ILLEGAL_PARAM);
        AX_DEC_RET_STR_CASE(AX_ERR_POOL_NOT_SUPPORT);
        AX_DEC_RET_STR_CASE(AX_ERR_POOL_NOT_PERM);
        AX_DEC_RET_STR_CASE(AX_ERR_VDEC_NEED_REALLOC_BUF);
    default:
        ALOGE("Unknown return code. 0x%x", s32Ret);
        return ("unknown code.");
    }
}

#if !VDEC_LINK_MODE
void *_vdec_get_frame_thread(void *arg)
{
    pipeline_t *pipe = (pipeline_t *)arg;
    AX_VIDEO_FRAME_INFO_S frameInfo = {0};
    // AX_VIDEO_FRAME_S frame;
    while (!pipe->n_loog_exit)
    {
        AX_S32 ret = AX_VDEC_GetFrame(pipe->n_vdec_grp, &frameInfo, 200);
        if (ret != 0)
        {
            ALOGE("AX_VDEC_GetFrame 0x%x", ret);
            usleep(50 * 1000);
            continue;
        }
        // else
        // {

        //     frameInfo.stVFrame.u64VirAddr[0] = (AX_U32)AX_POOL_GetBlockVirAddr(frameInfo.stVFrame.u32BlkId[0]);
        //     frameInfo.stVFrame.u64PhyAddr[0] = AX_POOL_Handle2PhysAddr(frameInfo.stVFrame.u32BlkId[0]);
        ALOGI("AX_VDEC_GetFrame success %d %d %d", frameInfo.stVFrame.u32Width, frameInfo.stVFrame.u32Height, frameInfo.stVFrame.enImgFormat);
        // }

        ret = AX_IVPS_SendFrame(pipe->m_ivps_attr.n_ivps_grp, &frameInfo.stVFrame, 200);
        if (ret != 0)
        {
            ALOGE("AX_IVPS_SendFrame 0x%x", ret);
        }

        AX_VDEC_ReleaseFrame(pipe->n_vdec_grp, &frameInfo);
    }
}
#endif

AX_S32 FramePoolInit(AX_VDEC_GRP VdGrp, AX_U32 FrameSize, AX_POOL *PoolId)
{
    AX_S32 s32Ret = AX_SUCCESS;
    /* vdec use pool to alloc output buffer */
    AX_POOL_CONFIG_T stPoolConfig = {0};
    AX_POOL s32PoolId;

    memset(&stPoolConfig, 0, sizeof(AX_POOL_CONFIG_T));
    stPoolConfig.MetaSize = 512;
    stPoolConfig.BlkCnt = 10;
    stPoolConfig.BlkSize = FrameSize;
    stPoolConfig.CacheMode = POOL_CACHE_MODE_NONCACHE;
    memset(stPoolConfig.PartitionName, 0, sizeof(stPoolConfig.PartitionName));
    strcpy((AX_CHAR *)stPoolConfig.PartitionName, "anonymous");

    s32PoolId = AX_POOL_CreatePool(&stPoolConfig);
    if (AX_INVALID_POOLID == s32PoolId)
    {
        ALOGE("Create pool err.\n");
        return AX_ERR_VDEC_NULL_PTR;
    }

    *PoolId = s32PoolId;

    s32Ret = AX_VDEC_AttachPool(VdGrp, 0, s32PoolId);
    if (s32Ret != AX_SUCCESS)
    {
        AX_POOL_DestroyPool(s32PoolId);
        ALOGE("Attach pool err. 0x%x\n", s32Ret);
    }

    printf("FramePoolInit successfully! %d\n", s32PoolId);

    return s32Ret;
}

int _create_vdec_grp(pipeline_t *pipe)
{
    if (pipe->m_vdec_attr.n_vdec_grp > MAX_VDEC_GRP_COUNT)
    {
        ALOGE("vdec_grp must lower than %d, got %d\n", MAX_VDEC_GRP_COUNT, pipe->m_vdec_attr.n_vdec_grp);
        return -1;
    }
    AX_VDEC_GRP_ATTR_T gGrpAttr;
    memset(&gGrpAttr, 0, sizeof(AX_VDEC_GRP_ATTR_T));
    switch (pipe->m_input_type)
    {
    case pi_vdec_h264:
    {
        gGrpAttr.enCodecType = PT_H264;
        gGrpAttr.enInputMode = AX_VDEC_INPUT_MODE_FRAME;
        gGrpAttr.u32MaxPicWidth = 1920;
        gGrpAttr.u32MaxPicHeight = 1080;
        gGrpAttr.u32StreamBufSize = 8 * 1024 * 1024;
        // gGrpAttr.u32MaxFrameBufCnt = 10;
        // gGrpAttr.u32InputFifoDepth = 100;
        // gGrpAttr.bSdkAutoFramePool = AX_TRUE;
        gGrpAttr.bSdkAutoFramePool = AX_FALSE;
#if VDEC_LINK_MODE
        // gGrpAttr.enLinkMode = AX_LINK_MODE;
#endif
        AX_POOL s32PoolId = 0;
        AX_U32 FrameSize = 0;

        AX_S32 ret = AX_VDEC_CreateGrp(pipe->m_vdec_attr.n_vdec_grp, &gGrpAttr);
        if (ret != AX_SUCCESS)
        {
            ALOGE("AX_VDEC_CreateGrp error: 0x%x\n", ret);
            return -1;
        }

        {
            AX_VDEC_CHN_ATTR_T stVdChnAttr = {0};

            stVdChnAttr.enImgFormat = AX_FORMAT_YUV420_SEMIPLANAR;
            stVdChnAttr.enOutputMode = AX_VDEC_OUTPUT_ORIGINAL;
            stVdChnAttr.u32PicWidth = 1920;
            stVdChnAttr.u32PicHeight = 1080;
            stVdChnAttr.u32FrameBufCnt = 5;
            AX_U32 uPixBits = 8;
#define AX_SHIFT_LEFT_ALIGN(a) (1 << (a))
#define AX_VDEC_WIDTH_ALIGN AX_SHIFT_LEFT_ALIGN(8)
            stVdChnAttr.u32FrameStride = AX_COMM_ALIGN(AX_COMM_ALIGN(stVdChnAttr.u32PicWidth, 128) * uPixBits,
                                                       AX_VDEC_WIDTH_ALIGN * 8) /
                                         8;
            // stVdChnAttr.u32FrameStride = AX_COMM_ALIGN(stVdChnAttr.u32PicWidth * 8, AX_VDEC_WIDTH_ALIGN * 8) / 8;
            stVdChnAttr.u32FramePadding = 0;
            stVdChnAttr.u32CropX = 0;
            stVdChnAttr.u32CropY = 0;
            stVdChnAttr.u32ScaleRatioX = 1;
            stVdChnAttr.u32ScaleRatioY = 1;

            stVdChnAttr.u32OutputFifoDepth = 0;

            ret = AX_VDEC_SetChnAttr(pipe->m_vdec_attr.n_vdec_grp, 0, &stVdChnAttr);
            if (ret != AX_SUCCESS)
            {
                ALOGE("AX_VDEC_SetChnAttr failed! Error:%x %s\n", ret, AX_VdecRetStr(ret));
                return -1;
            }
            ret = AX_VDEC_EnableChn(pipe->m_vdec_attr.n_vdec_grp, 0);
            if (ret != AX_SUCCESS)
            {
                ALOGE("AX_VDEC_EnableChn failed! Error:%x %s\n", ret, AX_VdecRetStr(ret));
                return -1;
            }
        }

        // AX_FRAME_COMPRESS_INFO_T stCompressInfo = {0};
        if (gGrpAttr.bSdkAutoFramePool != AX_TRUE)
        {
            FrameSize = AX_VDEC_GetPicBufferSize(1920, 1080, AX_FORMAT_YUV420_SEMIPLANAR, NULL, PT_H264); // 3655712;
            printf("Get pool mem size is %d\n", FrameSize);
            ret = FramePoolInit(pipe->m_vdec_attr.n_vdec_grp, FrameSize, &s32PoolId);
            if (ret != AX_SUCCESS)
            {
                ALOGE("FramePoolInit failed! Error:%x\n", ret);
                return -1;
            }
        }

        pipe->m_vdec_attr.poolid = s32PoolId;
        AX_VDEC_RECV_PIC_PARAM_T stRecvParam = {0};
        stRecvParam.s32RecvPicNum = -1;
        ret = AX_VDEC_StartRecvStream(pipe->m_vdec_attr.n_vdec_grp, &stRecvParam);
        if (ret != AX_SUCCESS)
        {
            ALOGE("AX_VDEC_StartRecvStream error: 0x%x\n", ret);
            AX_VDEC_DestroyGrp(pipe->m_vdec_attr.n_vdec_grp);
            return -1;
        }
#if !VDEC_LINK_MODE
        if (0 != pthread_create(&pipe->m_vdec_attr.tid, NULL, _vdec_get_frame_thread, pipe))
        {
            return -1;
        }
#endif
    }
    break;
    case pi_vdec_jpeg:
    {
        AX_U32 FrameSize = 0;
        FrameSize = AX_VDEC_GetPicBufferSize(4096, 4096, AX_FORMAT_RGB888, NULL, PT_JPEG); // 3655712;
        printf("Get pool mem size is %d\n", FrameSize);

        // AX_S32 s32Ret = AX_SUCCESS;
        /* vdec use pool to alloc output buffer */
        AX_POOL_CONFIG_T stPoolConfig = {0};
        AX_POOL s32PoolId;

        memset(&stPoolConfig, 0, sizeof(AX_POOL_CONFIG_T));
        stPoolConfig.MetaSize = 512;
        stPoolConfig.BlkCnt = 10;
        stPoolConfig.BlkSize = FrameSize;
        stPoolConfig.CacheMode = POOL_CACHE_MODE_NONCACHE;
        memset(stPoolConfig.PartitionName, 0, sizeof(stPoolConfig.PartitionName));
        strcpy((AX_CHAR *)stPoolConfig.PartitionName, "anonymous");

        s32PoolId = AX_POOL_CreatePool(&stPoolConfig);
        if (AX_INVALID_POOLID == s32PoolId)
        {
            printf("Create pool err.\n");
            return AX_ERR_VDEC_NULL_PTR;
        }
        pipe->m_vdec_attr.poolid = s32PoolId;
    }
    break;
    default:
        break;
    }

    return 0;
}

int _destore_jvdec_grp(pipeline_t *pipe)
{
    int ret = AX_VDEC_StopRecvStream(pipe->m_vdec_attr.n_vdec_grp);

    ret = AX_VDEC_DetachPool(pipe->m_vdec_attr.n_vdec_grp, 0);
    // AX_POOL_DestroyPool(pipe->m_vdec_attr.poolid);

    ret = AX_VDEC_DestroyGrp(pipe->m_vdec_attr.n_vdec_grp);
    return 0;
}

int _destore_vdec_grp(pipeline_t *pipe)
{
#if !VDEC_LINK_MODE
    pthread_join(pipe->m_vdec_attr.tid, NULL);
#endif
    _destore_jvdec_grp(pipe);
    AX_VDEC_DetachPool(pipe->m_vdec_attr.n_vdec_grp, 0);
    AX_POOL_DestroyPool(pipe->m_vdec_attr.poolid);

    return 0;
}

int _create_jvdec_grp(pipeline_t *pipe)
{
    if (pipe->m_vdec_attr.n_vdec_grp > MAX_VDEC_GRP_COUNT)
    {
        ALOGE("vdec_grp must lower than %d, got %d\n", MAX_VDEC_GRP_COUNT, pipe->m_vdec_attr.n_vdec_grp);
        return -1;
    }
    AX_VDEC_GRP_ATTR_T gGrpAttr;
    memset(&gGrpAttr, 0, sizeof(AX_VDEC_GRP_ATTR_T));

    gGrpAttr.enCodecType = PT_JPEG;
    gGrpAttr.u32MaxPicWidth = 1920;
    gGrpAttr.u32MaxPicHeight = 1080;
    gGrpAttr.u32StreamBufSize = 8 * 1024 * 1024;
    // gGrpAttr.u32MaxFrameBufCnt = 10;
    gGrpAttr.enInputMode = AX_VDEC_INPUT_MODE_FRAME;
#if VDEC_LINK_MODE
    // gGrpAttr.enLinkMode = AX_LINK_MODE;
#endif
    AX_S32 ret = AX_VDEC_CreateGrp(pipe->m_vdec_attr.n_vdec_grp, &gGrpAttr);
    if (ret != AX_SUCCESS)
    {
        ALOGE("AX_VDEC_CreateGrp error: 0x%x\n", ret);
        return -1;
    }
    ret = AX_VDEC_AttachPool(pipe->m_vdec_attr.n_vdec_grp, 0, pipe->m_vdec_attr.poolid);
    if (ret != AX_SUCCESS)
    {
        AX_POOL_DestroyPool(pipe->m_vdec_attr.poolid);
        printf("Attach pool err. %x\n", ret);
    }
    AX_VDEC_RECV_PIC_PARAM_T stRecvParam = {0};
    stRecvParam.s32RecvPicNum = 10;
    ret = AX_VDEC_StartRecvStream(pipe->m_vdec_attr.n_vdec_grp, &stRecvParam);
    if (ret != AX_SUCCESS)
    {
        ALOGE("AX_VDEC_StartRecvStream error: 0x%x\n", ret);
        AX_VDEC_DestroyGrp(pipe->m_vdec_attr.n_vdec_grp);
        return -1;
    }
    return 0;
}