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
#include "rtsp.h"
#include "../../utilities/sample_log.h"
#include "../../utilities/net_utils.h"

#include "ax_ivps_api.h"
#include "ax_vdec_api.h"
#include "ax_venc_api.h"

#include "algorithm"
#include "vector"
#include "string"
#include "string.h"
#include "map"
#include "unistd.h"

#define RTSP_PORT 8554

typedef struct
{

    std::map<int, pipeline_t *> pipeid_pipe;

    bool b_maix3_init = false;

    rtsp_demo_handle rDemoHandle = NULL;
    std::map<int, rtsp_session_handle> rtsp_pipeid_sessiones;
    std::vector<std::string> rtsp_end_point;

    std::vector<int> ivps_grp;
    std::vector<int> vdec_grp;
    std::vector<int> venc_chn;
} pipeline_internal_handle_t;

static pipeline_internal_handle_t pipeline_handle;

template <typename T>
bool contain(std::vector<T> &v, T &t)
{
    auto item = std::find(v.begin(), v.end(), t);
    if (item != v.end())
    {
        return true;
    }
    return false;
}

template <typename KT, typename VT>
bool contain(std::map<KT, VT> &v, KT &t)
{
    auto item = v.find(t);
    if (item != v.end())
    {
        return true;
    }
    return false;
}

template <typename T>
bool erase(std::vector<T> &v, T &t)
{
    auto item = std::find(v.begin(), v.end(), t);
    if (item != v.end())
    {
        v.erase(item);
        return true;
    }
    return false;
}

template <typename KT, typename VT>
bool erase(std::map<KT, VT> &v, KT &t)
{
    auto item = v.find(t);
    if (item != v.end())
    {
        v.erase(item);
        return true;
    }
    return false;
}

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

int _create_vo(char *pStr, pipeline_t *pipe);
void _destory_vo();
int _create_ivps_grp(pipeline_t *pipe);
int _destore_ivps_grp(pipeline_t *pipe);
int _create_venc_chn(pipeline_t *pipe);
int _destore_venc_grp(pipeline_t *pipe);

int _create_vdec_grp(pipeline_t *pipe);
int _destore_vdec_grp(pipeline_t *pipe);
int _create_jvdec_grp(pipeline_t *pipe);
int _destore_jvdec_grp(pipeline_t *pipe);

bool check_rtsp_session_pipeid(int pipeid)
{
    return contain(pipeline_handle.rtsp_pipeid_sessiones, pipeid);
}

rtsp_demo_handle get_rtsp_demo_handle()
{
    return pipeline_handle.rDemoHandle;
}

rtsp_session_handle get_rtsp_session_handle(int pipeid)
{
    return pipeline_handle.rtsp_pipeid_sessiones[pipeid];
}

int create_pipeline(pipeline_t *pipe)
{
    if (pipe->enable == 0)
    {
        ALOGE("PIPE-%d doesn`t enable", pipe->pipeid);
        return -1;
    }

    if (contain(pipeline_handle.pipeid_pipe, pipe->pipeid))
    {
        ALOGE("PIPE-%d has been create", pipe->pipeid);
        return -1;
    }
    pipeline_handle.pipeid_pipe[pipe->pipeid] = pipe;

    switch (pipe->m_input_type)
    {
    case pi_user:
    {
        if (pipeline_handle.ivps_grp.size() == 0)
        {
            int s32Ret = AX_IVPS_Init();
            if (0 != s32Ret)
            {
                ALOGE("AX_IVPS_Init failed,s32Ret:0x%x\n", s32Ret);
                return s32Ret;
            }
        }
        if (contain(pipeline_handle.ivps_grp, pipe->m_ivps_attr.n_ivps_grp))
        {
            ALOGE("IVPS-%d has been create", pipe->m_ivps_attr.n_ivps_grp);
            return -1;
        }
        pipeline_handle.ivps_grp.push_back(pipe->m_ivps_attr.n_ivps_grp);
        int s32Ret = _create_ivps_grp(pipe);
        if (AX_SUCCESS != s32Ret)
        {
            ALOGE("_create_ivps_grp failed,s32Ret:0x%x\n", s32Ret);
            return -1;
        }
    }
    break;
    case pi_vin:
    {
        if (pipeline_handle.ivps_grp.size() == 0)
        {
            int s32Ret = AX_IVPS_Init();
            if (0 != s32Ret)
            {
                ALOGE("AX_IVPS_Init failed,s32Ret:0x%x\n", s32Ret);
                return s32Ret;
            }
        }
        if (contain(pipeline_handle.ivps_grp, pipe->m_ivps_attr.n_ivps_grp))
        {
            ALOGE("IVPS-%d has been create", pipe->m_ivps_attr.n_ivps_grp);
            return -1;
        }
        pipeline_handle.ivps_grp.push_back(pipe->m_ivps_attr.n_ivps_grp);

        int s32Ret = _create_ivps_grp(pipe);
        if (AX_SUCCESS != s32Ret)
        {
            ALOGE("_create_ivps_grp failed,s32Ret:0x%x\n", s32Ret);
            return -1;
        }

        AX_MOD_INFO_S srcMod, dstMod;
        srcMod.enModId = AX_ID_VIN;
        srcMod.s32GrpId = pipe->n_vin_pipe;
        srcMod.s32ChnId = pipe->n_vin_chn;

        dstMod.enModId = AX_ID_IVPS;
        dstMod.s32GrpId = pipe->m_ivps_attr.n_ivps_grp;
        dstMod.s32ChnId = 0;
        AX_SYS_Link(&srcMod, &dstMod);
    }
    break;
    case pi_vdec_h264:
    case pi_vdec_jpeg:
    {
        if (pipeline_handle.ivps_grp.size() == 0)
        {
            int s32Ret = AX_IVPS_Init();
            if (0 != s32Ret)
            {
                ALOGE("AX_IVPS_Init failed,s32Ret:0x%x\n", s32Ret);
                return s32Ret;
            }
        }
        if (contain(pipeline_handle.ivps_grp, pipe->m_ivps_attr.n_ivps_grp))
        {
            ALOGE("IVPS-%d has been create", pipe->m_ivps_attr.n_ivps_grp);
            return -1;
        }
        pipeline_handle.ivps_grp.push_back(pipe->m_ivps_attr.n_ivps_grp);

        int s32Ret = _create_ivps_grp(pipe);
        if (AX_SUCCESS != s32Ret)
        {
            ALOGE("_create_ivps_grp failed,s32Ret:0x%x\n", s32Ret);
            return -1;
        }

        if (pipeline_handle.vdec_grp.size() == 0)
        {
            int s32Ret = AX_VDEC_Init();
            if (0 != s32Ret)
            {
                ALOGE("AX_IVPS_Init failed,s32Ret:0x%x\n", s32Ret);
                return s32Ret;
            }
        }

        if (!contain(pipeline_handle.vdec_grp, pipe->m_vdec_attr.n_vdec_grp))
        {
            int s32Ret = _create_vdec_grp(pipe);
            if (AX_SUCCESS != s32Ret)
            {
                ALOGE("_create_vdec_grp failed,s32Ret:0x%x\n", s32Ret);
                return -1;
            }
            pipeline_handle.vdec_grp.push_back(pipe->m_vdec_attr.n_vdec_grp);
        }
#if VDEC_LINK_MODE
        AX_MOD_INFO_S srcMod, dstMod;
        srcMod.enModId = AX_ID_VDEC;
        srcMod.s32GrpId = pipe->m_vdec_attr.n_vdec_grp;
        srcMod.s32ChnId = 0;

        dstMod.enModId = AX_ID_IVPS;
        dstMod.s32GrpId = pipe->m_ivps_attr.n_ivps_grp;
        dstMod.s32ChnId = 0;
        AX_SYS_Link(&srcMod, &dstMod);
#endif
    }
    break;
    default:
        break;
    }

    switch (pipe->m_output_type)
    {
    case po_venc_mjpg:
    case po_venc_h264:
    case po_venc_h265:
    case po_rtsp_h264:
    case po_rtsp_h265:
    {
        if (pipeline_handle.venc_chn.size() == 0)
        {
            AX_VENC_MOD_ATTR_S stModAttr;
            stModAttr.enVencType = VENC_MULTI_ENCODER;

            int s32Ret = AX_VENC_Init(&stModAttr);
            if (AX_SUCCESS != s32Ret)
            {
                ALOGE("AX_VENC_Init failed, s32Ret:0x%x", s32Ret);
                return s32Ret;
            }
        }

        if (contain(pipeline_handle.venc_chn, pipe->m_venc_attr.n_venc_chn))
        {
            ALOGE("VENC-%d has been create", pipe->m_venc_attr.n_venc_chn);
            return -1;
        }
        pipeline_handle.venc_chn.push_back(pipe->m_venc_attr.n_venc_chn);

        AX_MOD_INFO_S srcMod, dstMod;
        srcMod.enModId = AX_ID_IVPS;
        srcMod.s32GrpId = pipe->m_ivps_attr.n_ivps_grp;
        srcMod.s32ChnId = 0;

        dstMod.enModId = AX_ID_VENC;
        dstMod.s32GrpId = 0;
        dstMod.s32ChnId = pipe->m_venc_attr.n_venc_chn;
        AX_SYS_Link(&srcMod, &dstMod);
        // }

        // pipeline_handle.b_init_venc++;

        int s32Ret = _create_venc_chn(pipe);
        if (AX_SUCCESS != s32Ret)
        {
            ALOGE("_create_venc_chn failed,s32Ret:0x%x\n", s32Ret);
            return -1;
        }

        if (pipe->m_output_type == po_rtsp_h264 || pipe->m_output_type == po_rtsp_h265)
        {
            if (!pipeline_handle.rDemoHandle)
            {
                pipeline_handle.rDemoHandle = rtsp_new_demo(RTSP_PORT);
            }
            std::string end_point = pipe->m_venc_attr.end_point;
            if (end_point.length())
            {
                if (end_point[0] != '/')
                {
                    end_point = "/" + end_point;
                }
            }

            if (!contain(pipeline_handle.rtsp_end_point, end_point) && !contain(pipeline_handle.rtsp_pipeid_sessiones, pipe->pipeid))
            {
                auto rSessionHandle = create_rtsp_session(pipeline_handle.rDemoHandle, end_point.c_str(), pipe->m_output_type == po_rtsp_h264 ? 0 : 1);
                PrintRtsp((char *)end_point.c_str());
                pipeline_handle.rtsp_pipeid_sessiones[pipe->pipeid] = rSessionHandle;
                pipeline_handle.rtsp_end_point.push_back(end_point);
            }
            else
            {
                ALOGE("rtsp end point %s has been create", end_point.c_str());
            }
        }
    }

    break;
    case po_vo_sipeed_maix3_screen:
    {
        if (!pipeline_handle.b_maix3_init)
        {
            AX_MOD_INFO_S srcMod, dstMod;
            srcMod.enModId = AX_ID_IVPS;
            srcMod.s32GrpId = pipe->m_ivps_attr.n_ivps_grp;
            srcMod.s32ChnId = 0;
            dstMod.enModId = AX_ID_VO;
            dstMod.s32GrpId = 0;
            dstMod.s32ChnId = 0;
            AX_SYS_Link(&srcMod, &dstMod);

            int s32Ret = _create_vo("dsi0@480x854@60", pipe);
            if (AX_SUCCESS != s32Ret)
            {
                ALOGE("VoInit failed,s32Ret:0x%x\n", s32Ret);
                return -1;
            }
            pipeline_handle.b_maix3_init = true;
        }
        else
        {
            ALOGE("screen has been init");
        }
    }
    break;
    default:
        break;
    }
    return 0;
}

int destory_pipeline(pipeline_t *pipe)
{
    if (!pipe->enable)
    {
        return -1;
    }

    if (!contain(pipeline_handle.pipeid_pipe, pipe->pipeid))
    {
        return -1;
    }
    pipe->n_loog_exit = 1;
    usleep(200 * 1000);
    erase(pipeline_handle.pipeid_pipe, pipe->pipeid);

    switch (pipe->m_input_type)
    {
    case pi_user:
        break;
    case pi_vin:
    {
        AX_MOD_INFO_S srcMod, dstMod;
        srcMod.enModId = AX_ID_VIN;
        srcMod.s32GrpId = pipe->n_vin_pipe;
        srcMod.s32ChnId = pipe->n_vin_chn;

        dstMod.enModId = AX_ID_IVPS;
        dstMod.s32GrpId = pipe->m_ivps_attr.n_ivps_grp;
        dstMod.s32ChnId = 0;
        AX_SYS_UnLink(&srcMod, &dstMod);

        if (contain(pipeline_handle.ivps_grp, pipe->m_ivps_attr.n_ivps_grp))
        {
            _destore_ivps_grp(pipe);
            erase(pipeline_handle.ivps_grp, pipe->m_ivps_attr.n_ivps_grp);
        }

        if (pipeline_handle.ivps_grp.size() == 0)
        {
            ALOGN("AX_IVPS_Deinit");
            AX_IVPS_Deinit();
        }
    }
    break;
    case pi_vdec_h264:
    case pi_vdec_jpeg:
    {
        AX_MOD_INFO_S srcMod, dstMod;
        srcMod.enModId = AX_ID_VDEC;
        srcMod.s32GrpId = pipe->m_vdec_attr.n_vdec_grp;
        srcMod.s32ChnId = 0;

        dstMod.enModId = AX_ID_IVPS;
        dstMod.s32GrpId = pipe->m_ivps_attr.n_ivps_grp;
        dstMod.s32ChnId = 0;
        AX_SYS_UnLink(&srcMod, &dstMod);

        if (contain(pipeline_handle.vdec_grp, pipe->m_vdec_attr.n_vdec_grp))
        {
            if (pipe->m_input_type == pi_vdec_h264)
                _destore_vdec_grp(pipe);
            erase(pipeline_handle.vdec_grp, pipe->m_vdec_attr.n_vdec_grp);
        }
        if (pipeline_handle.vdec_grp.size() == 0)
        {
            ALOGN("AX_VDEC_DeInit");
            AX_VDEC_DeInit();
        }
        if (contain(pipeline_handle.ivps_grp, pipe->m_ivps_attr.n_ivps_grp))
        {
            _destore_ivps_grp(pipe);
            erase(pipeline_handle.ivps_grp, pipe->m_ivps_attr.n_ivps_grp);
        }

        if (pipeline_handle.ivps_grp.size() == 0)
        {
            ALOGN("AX_IVPS_Deinit");
            AX_IVPS_Deinit();
        }
    }
    break;
    default:
        break;
    }

    switch (pipe->m_output_type)
    {
    case po_venc_mjpg:
    case po_venc_h264:
    case po_venc_h265:
    case po_rtsp_h264:
    case po_rtsp_h265:
    {
        AX_MOD_INFO_S srcMod, dstMod;
        srcMod.enModId = AX_ID_IVPS;
        srcMod.s32GrpId = pipe->m_ivps_attr.n_ivps_grp;
        srcMod.s32ChnId = 0;

        dstMod.enModId = AX_ID_VENC;
        dstMod.s32GrpId = 0;
        dstMod.s32ChnId = pipe->m_venc_attr.n_venc_chn;
        AX_SYS_UnLink(&srcMod, &dstMod);

        if (contain(pipeline_handle.venc_chn, pipe->m_venc_attr.n_venc_chn))
        {
            _destore_venc_grp(pipe);
            erase(pipeline_handle.venc_chn, pipe->m_venc_attr.n_venc_chn);
        }

        if (pipeline_handle.venc_chn.size() == 0)
        {
            ALOGN("AX_VENC_Deinit");
            AX_VENC_Deinit();
        }

        if (pipe->m_output_type == po_rtsp_h264 || pipe->m_output_type == po_rtsp_h265)
        {
            std::string end_point = pipe->m_venc_attr.end_point;
            if (end_point.length())
            {
                if (end_point[0] != '/')
                {
                    end_point = "/" + end_point;
                }
            }
            if (contain(pipeline_handle.rtsp_pipeid_sessiones, pipe->pipeid))
            {
                rtsp_del_session(pipeline_handle.rtsp_pipeid_sessiones[pipe->pipeid]);
                erase(pipeline_handle.rtsp_pipeid_sessiones, pipe->pipeid);
            }
            if (contain(pipeline_handle.rtsp_end_point, end_point))
                erase(pipeline_handle.rtsp_end_point, end_point);

            if (pipeline_handle.rtsp_pipeid_sessiones.size() == 0)
            {
                rtsp_del_demo(pipeline_handle.rDemoHandle);
                ALOGN("rtsp server release");
            }
        }
    }

    break;
    case po_vo_sipeed_maix3_screen:
    {
        AX_MOD_INFO_S srcMod, dstMod;
        srcMod.enModId = AX_ID_IVPS;
        srcMod.s32GrpId = pipe->m_ivps_attr.n_ivps_grp;
        srcMod.s32ChnId = 0;
        dstMod.enModId = AX_ID_VO;
        dstMod.s32GrpId = 0;
        dstMod.s32ChnId = 0;
        AX_SYS_UnLink(&srcMod, &dstMod);
        if (pipeline_handle.b_maix3_init)
        {
            _destory_vo();
            pipeline_handle.b_maix3_init = false;
        }
    }
    break;
    default:
        break;
    }
    return 0;
}

int user_input(pipeline_t *pipe, int pipe_cnt, pipeline_buffer_t *buf)
{
    if (!contain(pipeline_handle.pipeid_pipe, pipe->pipeid))
    {
        ALOGE("pipe-%d haven`t create", pipe->pipeid);
        return -1;
    }

    switch (pipe->m_input_type)
    {
    case pi_user:
    {
        AX_VIDEO_FRAME_INFO_S frameInfo = {0};

        AX_U32 uiPicSize = (buf->n_width * buf->n_height) * 3 / 2;
        AX_BLK blk_id = AX_POOL_GetBlock(frameInfo.u32PoolId, uiPicSize, NULL);
        if (AX_INVALID_BLOCKID == blk_id)
        {
            printf("AX_POOL_GetBlock AX_POOL_GetBlockfailed! \n");
            return -1;
        }

        frameInfo.bEof = AX_TRUE;
        frameInfo.enModId = AX_ID_IVPS;
        frameInfo.stVFrame.u32BlkId[0] = blk_id;
        frameInfo.stVFrame.u32Width = buf->n_width;
        frameInfo.stVFrame.u32Height = buf->n_height;
        frameInfo.stVFrame.enImgFormat = AX_YUV420_SEMIPLANAR;
        frameInfo.stVFrame.enVscanFormat = AX_VSCAN_FORMAT_RASTER;
        frameInfo.stVFrame.enCompressMode = AX_COMPRESS_MODE_NONE;
        frameInfo.stVFrame.u64PhyAddr[0] = AX_POOL_Handle2PhysAddr(blk_id);
        frameInfo.stVFrame.u64VirAddr[0] = (AX_U64)AX_POOL_GetBlockVirAddr(blk_id);
        frameInfo.stVFrame.u32PicStride[0] = buf->n_width;
        frameInfo.stVFrame.u64PhyAddr[1] = frameInfo.stVFrame.u64PhyAddr[0] + frameInfo.stVFrame.u32PicStride[0] * frameInfo.stVFrame.u32Height;
        frameInfo.stVFrame.u64PhyAddr[2] = 0;
        frameInfo.stVFrame.u64VirAddr[1] = frameInfo.stVFrame.u64VirAddr[0] + frameInfo.stVFrame.u32PicStride[0] * frameInfo.stVFrame.u32Height;
        frameInfo.stVFrame.u64VirAddr[2] = 0;
        frameInfo.u32PoolId = AX_POOL_Handle2PoolId(blk_id);

        memcpy((void *)frameInfo.stVFrame.u64VirAddr[0], buf->p_vir, uiPicSize);
        int ret;
        std::vector<int> tmp_;

        for (int i = 0; i < pipe_cnt; i++)
        {
            if (!contain(tmp_, pipe[i].m_ivps_attr.n_ivps_grp))
            {
                ret = AX_IVPS_SendFrame(pipe[i].m_ivps_attr.n_ivps_grp, &frameInfo.stVFrame, 200);
                if (ret != 0)
                {
                    ALOGE("AX_IVPS_SendFrame 0x%x", ret);
                }
                tmp_.push_back(pipe[i].m_ivps_attr.n_ivps_grp);
            }
        }

        // AX_U32 PoolId = frameInfo.u32PoolId;

        AX_BLK BlkId = frameInfo.stVFrame.u32BlkId[0];
        ret = AX_POOL_ReleaseBlock(BlkId);
        if (ret != 0)
        {
            printf("AX_POOL_ReleaseBlock fail!Error Code:0x%X\n", ret);
            return -1;
        }
    }
    break;
    case pi_vdec_h264:
    {
        AX_VDEC_STREAM_S stream = {0};
        int unsigned long long pts = 0;
        stream.u64PTS = pts++;
        stream.u32Len = buf->n_size;
        stream.pu8Addr = (unsigned char *)buf->p_vir;
        stream.bEndOfFrame = buf->p_vir == NULL ? AX_TRUE : AX_FALSE;
        stream.bEndOfStream = buf->p_vir == NULL ? AX_TRUE : AX_FALSE;

        int ret;
        std::vector<int> tmp_;

        for (int i = 0; i < pipe_cnt; i++)
        {
            if (!contain(tmp_, pipe[i].m_vdec_attr.n_vdec_grp))
            {
                ret = AX_VDEC_SendStream(pipe[i].m_vdec_attr.n_vdec_grp, &stream, 200);
                if (ret != 0)
                {
                    ALOGE("AX_VDEC_SendStream 0x%x", ret);
                }
                tmp_.push_back(pipe[i].m_vdec_attr.n_vdec_grp);
            }
        }

        // int ret = AX_VDEC_SendStream(pipe->m_vdec_attr.n_vdec_grp, &stream, 200);
        // if (ret != 0)
        // {
        //     ALOGE("AX_VDEC_SendStream 0x%x", ret);
        //     return -1;
        // }
    }
    break;
    case pi_vdec_jpeg:
    {
        _create_jvdec_grp(pipe);
        AX_VDEC_STREAM_S stream = {0};
        int unsigned long long pts = 0;
        stream.u64PTS = pts++;
        stream.u32Len = buf->n_size;
        stream.pu8Addr = (unsigned char *)buf->p_vir;
        stream.bEndOfFrame = buf->p_vir == NULL ? AX_TRUE : AX_FALSE;
        stream.bEndOfStream = buf->p_vir == NULL ? AX_TRUE : AX_FALSE;

        int ret;
        std::vector<int> tmp_;

        for (int i = 0; i < pipe_cnt; i++)
        {
            if (!contain(tmp_, pipe[i].m_vdec_attr.n_vdec_grp))
            {
                ret = AX_VDEC_SendStream(pipe->m_vdec_attr.n_vdec_grp, &stream, -1);
                if (ret != 0)
                {
                    ALOGE("AX_VDEC_SendStream 0x%x", ret);
                }
                tmp_.push_back(pipe[i].m_vdec_attr.n_vdec_grp);
            }
        }
        // int ret = AX_VDEC_SendStream(pipe->m_vdec_attr.n_vdec_grp, &stream, -1);
        // if (ret != 0)
        // {
        //     ALOGE("AX_VDEC_SendStream 0x%x", ret);
        //     return -1;
        // }
        _destore_jvdec_grp(pipe);
    }
    break;
    default:
        break;
    }

    return 0;
}