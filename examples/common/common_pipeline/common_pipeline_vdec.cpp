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
        printf("Create pool err.\n");
        return AX_ERR_VDEC_NULL_PTR;
    }

    *PoolId = s32PoolId;

    s32Ret = AX_VDEC_AttachPool(VdGrp, s32PoolId);
    if (s32Ret != AX_SUCCESS)
    {
        AX_POOL_MarkDestroyPool(s32PoolId);
        printf("Attach pool err. %x\n", s32Ret);
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
    AX_VDEC_GRP_ATTR_S gGrpAttr;
    memset(&gGrpAttr, 0, sizeof(AX_VDEC_GRP_ATTR_S));
    switch (pipe->m_input_type)
    {
    case pi_vdec_h264:
    {
        gGrpAttr.enType = PT_H264;
        gGrpAttr.u32PicWidth = 1920;
        gGrpAttr.u32PicHeight = 1080;
        gGrpAttr.u32StreamBufSize = 8 * 1024 * 1024;
        gGrpAttr.u32FrameBufCnt = 10;
#if VDEC_LINK_MODE
        gGrpAttr.enLinkMode = AX_LINK_MODE;
#endif
        AX_POOL s32PoolId;
        AX_U32 FrameSize = 0;

        AX_S32 ret = AX_VDEC_CreateGrp(pipe->m_vdec_attr.n_vdec_grp, &gGrpAttr);
        if (ret != AX_SUCCESS)
        {
            ALOGE("AX_VDEC_CreateGrp error: 0x%x\n", ret);
            return -1;
        }

        FrameSize = AX_VDEC_GetPicBufferSize(1920, 1088, PT_H264); // 3655712;
        printf("Get pool mem size is %d\n", FrameSize);
        ret = FramePoolInit(pipe->m_vdec_attr.n_vdec_grp, FrameSize, &s32PoolId);
        if (ret != AX_SUCCESS)
        {
            printf("FramePoolInit failed! Error:%x\n", ret);
            return -1;
        }
        pipe->m_vdec_attr.poolid = s32PoolId;
        ret = AX_VDEC_StartRecvStream(pipe->m_vdec_attr.n_vdec_grp);
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
        FrameSize = AX_VDEC_GetPicBufferSize(4096, 4096, PT_JPEG); // 3655712;
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

    ret = AX_VDEC_DetachPool(pipe->m_vdec_attr.n_vdec_grp);
    // AX_POOL_MarkDestroyPool(pipe->m_vdec_attr.poolid);

    ret = AX_VDEC_DestroyGrp(pipe->m_vdec_attr.n_vdec_grp);
    return 0;
}

int _destore_vdec_grp(pipeline_t *pipe)
{
#if !VDEC_LINK_MODE
    pthread_join(pipe->m_vdec_attr.tid, NULL);
#endif
    _destore_jvdec_grp(pipe);
    AX_POOL_MarkDestroyPool(pipe->m_vdec_attr.poolid);

    return 0;
}

int _create_jvdec_grp(pipeline_t *pipe)
{
    if (pipe->m_vdec_attr.n_vdec_grp > MAX_VDEC_GRP_COUNT)
    {
        ALOGE("vdec_grp must lower than %d, got %d\n", MAX_VDEC_GRP_COUNT, pipe->m_vdec_attr.n_vdec_grp);
        return -1;
    }
    AX_VDEC_GRP_ATTR_S gGrpAttr;
    memset(&gGrpAttr, 0, sizeof(AX_VDEC_GRP_ATTR_S));

    gGrpAttr.enType = PT_JPEG;
    gGrpAttr.u32PicWidth = 1920;
    gGrpAttr.u32PicHeight = 1080;
    gGrpAttr.u32StreamBufSize = 8 * 1024 * 1024;
    gGrpAttr.u32FrameBufCnt = 10;
#if VDEC_LINK_MODE
    gGrpAttr.enLinkMode = AX_LINK_MODE;
#endif
    AX_S32 ret = AX_VDEC_CreateGrp(pipe->m_vdec_attr.n_vdec_grp, &gGrpAttr);
    if (ret != AX_SUCCESS)
    {
        ALOGE("AX_VDEC_CreateGrp error: 0x%x\n", ret);
        return -1;
    }
    ret = AX_VDEC_AttachPool(pipe->m_vdec_attr.n_vdec_grp, pipe->m_vdec_attr.poolid);
    if (ret != AX_SUCCESS)
    {
        AX_POOL_MarkDestroyPool(pipe->m_vdec_attr.poolid);
        printf("Attach pool err. %x\n", ret);
    }

    ret = AX_VDEC_StartRecvStream(pipe->m_vdec_attr.n_vdec_grp);
    if (ret != AX_SUCCESS)
    {
        ALOGE("AX_VDEC_StartRecvStream error: 0x%x\n", ret);
        AX_VDEC_DestroyGrp(pipe->m_vdec_attr.n_vdec_grp);
        return -1;
    }
    return 0;
}