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

#include "cstdio"
#include "stdlib.h"
#include "string.h"
#include "vector"
#include "cmath"
#include "map"
#include "common_vo.h"
// #include "ax_vo_api.h"

#define HDMI_MAX_DEV 1

int _create_vo(char *pStr, pipeline_t *pipe)
{
    ALOGE("");
    return -1;
}
void _destory_vo() {}

AX_VO_INTF_SYNC_E cvt(pipeline_hdmi_vo_e _enIntfSync)
{
    switch (_enIntfSync)
    {
    case phv_1920x1080p25:
        return AX_VO_OUTPUT_1080P25;
    case phv_1920x1080p30:
        return AX_VO_OUTPUT_1080P30;
    case phv_1920x1080p50:
        return AX_VO_OUTPUT_1080P50;
    case phv_1920x1080p60:
        return AX_VO_OUTPUT_1080P60;
    case phv_3840x2160p25:
        return AX_VO_OUTPUT_3840x2160_25;
    case phv_3840x2160p30:
        return AX_VO_OUTPUT_3840x2160_30;
    case phv_3840x2160p50:
        return AX_VO_OUTPUT_3840x2160_50;
    case phv_3840x2160p60:
        return AX_VO_OUTPUT_3840x2160_60;
    case phv_4096x2160p25:
        return AX_VO_OUTPUT_4096x2160_25;
    case phv_4096x2160p30:
        return AX_VO_OUTPUT_4096x2160_30;
    case phv_4096x2160p50:
        return AX_VO_OUTPUT_4096x2160_50;
    case phv_4096x2160p60:
        return AX_VO_OUTPUT_4096x2160_60;
    default:
        return AX_VO_OUTPUT_BUTT;
    }
}

AX_BOOL GetDispInfoFromIntfSync(AX_VO_INTF_SYNC_E eIntfSync, AX_VO_RECT_T &stArea, AX_U32 &nHz)
{
    stArea.u32X = 0;
    stArea.u32Y = 0;
    switch (eIntfSync)
    {
    case AX_VO_OUTPUT_1080P25:
        stArea.u32Width = 1920;
        stArea.u32Height = 1080;
        nHz = 25;
        break;
    case AX_VO_OUTPUT_1080P30:
        stArea.u32Width = 1920;
        stArea.u32Height = 1080;
        nHz = 30;
        break;
    case AX_VO_OUTPUT_1080P50:
        stArea.u32Width = 1920;
        stArea.u32Height = 1080;
        nHz = 50;
        break;
    case AX_VO_OUTPUT_1080P60:
        stArea.u32Width = 1920;
        stArea.u32Height = 1080;
        nHz = 60;
        break;
    case AX_VO_OUTPUT_3840x2160_25:
        stArea.u32Width = 3840;
        stArea.u32Height = 2160;
        nHz = 25;
        break;
    case AX_VO_OUTPUT_3840x2160_30:
        stArea.u32Width = 3840;
        stArea.u32Height = 2160;
        nHz = 30;
        break;
    case AX_VO_OUTPUT_3840x2160_50:
        stArea.u32Width = 3840;
        stArea.u32Height = 2160;
        nHz = 50;
        break;
    case AX_VO_OUTPUT_3840x2160_60:
        stArea.u32Width = 3840;
        stArea.u32Height = 2160;
        nHz = 60;
        break;
    case AX_VO_OUTPUT_4096x2160_25:
        stArea.u32Width = 4096;
        stArea.u32Height = 2160;
        nHz = 25;
        break;
    case AX_VO_OUTPUT_4096x2160_30:
        stArea.u32Width = 4096;
        stArea.u32Height = 2160;
        nHz = 30;
        break;
    case AX_VO_OUTPUT_4096x2160_50:
        stArea.u32Width = 4096;
        stArea.u32Height = 2160;
        nHz = 50;
        break;
    case AX_VO_OUTPUT_4096x2160_60:
        stArea.u32Width = 4096;
        stArea.u32Height = 2160;
        nHz = 60;
        break;
    default:
        ALOGE("%s: UnSupport device %d", __func__, eIntfSync);
        return AX_FALSE;
    }

    return AX_TRUE;
}

AX_BOOL InitLayout(AX_U32 nVideoCount, AX_U32 nScreenWidth, AX_U32 nScreenHeight, std::vector<AX_VO_CHN_ATTR_T> &m_arrChns)
{
    m_arrChns.clear();

    // InitResource();

    struct POINT
    {
        AX_U32 x, y;
    } pt;

    struct COORDINATE
    {
        AX_U32 x1, y1, x2, y2;
    } area;

    const AX_U32 BORDER = 8;
    AX_VO_CHN_ATTR_T stChnAttr;
    memset(&stChnAttr, 0, sizeof(stChnAttr));
    stChnAttr.u32FifoDepth = 1; /* default depth: 1 */
    stChnAttr.u32Priority = 0;

    pt.x = 0;
    pt.y = 0;

    AX_U32 nCols = ceil(sqrt((float)nVideoCount));
    AX_U32 nRows = ((nVideoCount % nCols) > 0) ? (nVideoCount / nCols + 1) : (nVideoCount / nCols);
    if (2 == nVideoCount)
    {
        nCols = 2;
        nRows = 2;
    }

    /* border for both row and col */
    const AX_U32 nAreaW = ALIGN_DOWN(((nScreenWidth - pt.x - BORDER * (nCols - 1)) / nCols), 8);
    const AX_U32 nAreaH = ALIGN_DOWN(((nScreenHeight - pt.y - BORDER * (nRows - 1)) / nRows), 2);

    for (AX_U32 i = 0; i < nRows; ++i)
    {
        for (AX_U32 j = 0; j < nCols; ++j)
        {
            if (m_arrChns.size() >= AX_VO_CHN_MAX)
            {
                break;
            }
            area.x1 = pt.x + j * BORDER + j * nAreaW;
            area.y1 = pt.y + i * BORDER + i * nAreaH;
            area.x2 = area.x1 + nAreaW;
            area.y2 = area.y1 + nAreaH;

            stChnAttr.stRect.u32X = area.x1;
            stChnAttr.stRect.u32Y = area.y1;
            stChnAttr.stRect.u32Width = area.x2 - area.x1;
            stChnAttr.stRect.u32Height = area.y2 - area.y1;

            ALOGI("video chn %d position: %d %d %d %d [%d x %d]", i * nRows + j, area.x1, area.y1, area.x2, area.y2,
                  stChnAttr.stRect.u32Width, stChnAttr.stRect.u32Height);
            m_arrChns.push_back(stChnAttr);
        }
    }

    return AX_TRUE;
}

AX_POOL CreatePool(const AX_POOL_CONFIG_T &stPoolCfg)
{
    AX_POOL pool = AX_POOL_CreatePool((AX_POOL_CONFIG_T *)&stPoolCfg);
    if (AX_INVALID_POOLID == pool)
    {
        ALOGE("%s: AX_POOL_CreatePool(blkSize %lld, blkCnt %d, metaSize %lld) fail", __func__, stPoolCfg.BlkSize, stPoolCfg.BlkCnt,
              stPoolCfg.MetaSize);
        return AX_FALSE;
    }

    // lock_guard<std::mutex> lck(m_mtx);
    // m_arrPools.push_back(pool);
    return pool;
}
AX_BOOL DestoryPool(AX_POOL &pool)
{
    if (AX_INVALID_POOLID == pool)
    {
        return AX_FALSE;
    }
    AX_S32 ret = AX_POOL_DestroyPool(pool);
    if (0 != ret)
    {
        ALOGE("%s: AX_POOL_DestroyPool(pool %d) fail, ret = 0x%x\n", __func__, ret, (AX_U32)pool);
        return AX_FALSE;
    }

    pool = AX_INVALID_POOLID;
    return AX_TRUE;
}

AX_BOOL CreatePools(VO_DEV voDev, AX_U32 nLayerDepth, AX_U32 nScreenWidth, AX_U32 nScreenHeigh, AX_POOL &m_LayerPool)
{
    AX_POOL_CONFIG_T stPoolCfg;
    memset(&stPoolCfg, 0, sizeof(stPoolCfg));
    stPoolCfg.MetaSize = 512;
    stPoolCfg.CacheMode = POOL_CACHE_MODE_NONCACHE;
    strcpy((AX_CHAR *)stPoolCfg.PartitionName, "anonymous");

    /* [0]: video layer pool */
    stPoolCfg.BlkSize = nScreenWidth * nScreenHeigh * 3 / 2;
    stPoolCfg.BlkCnt = nLayerDepth;
    sprintf((AX_CHAR *)stPoolCfg.PoolName, "vo_dev%d_layer_pool", voDev);
    m_LayerPool = CreatePool(stPoolCfg);
    if (AX_INVALID_POOLID == m_LayerPool)
    {
        return AX_FALSE;
    }

    /* [2]: video stream pool which has no source input */
    // if (m_rcNoVideo.bShow)
    // {
    //     AX_U32 nLeftChns = m_arrChns.size() - (m_rcLogo.bShow ? 1 : 0) - m_nVideoChnCount;
    //     if (nLeftChns > 0)
    //     {
    //         AX_U32 n = m_arrChns.size() - 1;
    //         /* if resize by IVPS TDP, image stride should align to 16 */
    //         constexpr AX_U32 IVPS_STRIDE_ALIGN = 16;
    //         stPoolCfg.BlkSize = ALIGN_UP(m_arrChns[n].stRect.u32Width, IVPS_STRIDE_ALIGN) * m_arrChns[n].stRect.u32Height * 3 / 2;
    //         stPoolCfg.BlkCnt = 1 * nLeftChns;
    //         sprintf((AX_CHAR *)stPoolCfg.PoolName, "vo_dev%d_novideo_pool", m_stAttr.voDev);
    //         m_rcNoVideo.pool = CAXPoolManager::GetInstance()->CreatePool(stPoolCfg);
    //         if (AX_INVALID_POOLID == m_rcNoVideo.pool)
    //         {
    //             return AX_FALSE;
    //         }
    //     }
    // }

    return AX_TRUE;
}

AX_BOOL SetChnFrameRate(VO_CHN voChn, VO_LAYER voLayer,
                        VO_DEV voDev, AX_U32 nFps)
{
    ALOGI("set layer %d voChn %d fps to %d", voLayer, voChn, nFps);
#ifdef __DUMMY_VO__
#else
    AX_FRAME_RATE_CTRL_U tmp{0};
    tmp.nFrmRateCtrl = nFps;
    AX_S32 ret = AX_VO_SetChnFrameRate(voLayer, voChn, tmp);
    if (0 != ret)
    {
        ALOGE("AX_VO_SetChnFrameRate(layer %d chn %d fps %d) fail, ret = 0x%x", voLayer, voChn, nFps, ret);
        return AX_FALSE;
    }
#endif

    return AX_TRUE;
}

static struct
{
    std::vector<AX_VO_CHN_ATTR_T> m_arrChns;
    VO_LAYER voLayer = 0;
    VO_DEV voDev = 0;
    AX_U32 nPoolCnt = 10;
    AX_POOL m_LayerPool;

    AX_VO_RECT_T stArea{0, 0, 1920, 1080};
    uint nHz = 60;
    bool inited = false;
} gHdmiAttr[2];

int _create_vo_hdmi(pipeline_t *pipe)
{
    ALOGI("%s: +++", __func__);
    AX_VO_Init();
    if (pipe->m_vo_attr.hdmi.portid < 0 || pipe->m_vo_attr.hdmi.portid > HDMI_MAX_DEV)
    {
        ALOGE("pipe->m_vo_attr.hdmi.portid must >=0&& <=%d", HDMI_MAX_DEV);
        return -1;
    }

    gHdmiAttr[pipe->m_vo_attr.hdmi.portid].voDev = pipe->m_vo_attr.hdmi.portid;
    pipeline_hdmi_vo_e _enIntfSync = pipe->m_vo_attr.hdmi.e_hdmi_type;
    unsigned nVideoCount = (unsigned)pipe->m_vo_attr.hdmi.n_vo_count;
    auto enIntfSync = cvt(_enIntfSync);
    if (!GetDispInfoFromIntfSync(enIntfSync, gHdmiAttr[pipe->m_vo_attr.hdmi.portid].stArea, gHdmiAttr[pipe->m_vo_attr.hdmi.portid].nHz))
    {
        return -1;
    }

    if (!InitLayout(nVideoCount, gHdmiAttr[pipe->m_vo_attr.hdmi.portid].stArea.u32Width, gHdmiAttr[pipe->m_vo_attr.hdmi.portid].stArea.u32Height, gHdmiAttr[pipe->m_vo_attr.hdmi.portid].m_arrChns))
    {
        return -1;
    }

    for (AX_U32 i = 0; i < nVideoCount; ++i)
    {
        VO_CHN voChn = i;
        gHdmiAttr[pipe->m_vo_attr.hdmi.portid].m_arrChns[voChn].u32FifoDepth = gHdmiAttr[pipe->m_vo_attr.hdmi.portid].nPoolCnt;
        gHdmiAttr[pipe->m_vo_attr.hdmi.portid].m_arrChns[voChn].u32Priority = 0;
        gHdmiAttr[pipe->m_vo_attr.hdmi.portid].m_arrChns[voChn].bKeepPrevFr = AX_TRUE;
    }

    AX_S32 ret = AX_VO_CreateVideoLayer(&gHdmiAttr[pipe->m_vo_attr.hdmi.portid].voLayer);
    if (0 != ret)
    {
        ALOGE("AX_VO_CreateVideoLayer(layer %d) fail, ret = 0x%x", gHdmiAttr[pipe->m_vo_attr.hdmi.portid].voLayer, ret);
        return -1;
    }

    enum
    {
        LAYER_CREATED = 0x1,
        VODEV_ENABLED = 0x2,
        LAYER_BINDED = 0x4,
        LAYER_ENABLED = 0x8
    };
    AX_U32 nState = LAYER_CREATED;

    try
    {
        AX_VO_PUB_ATTR_T stPubAttr;
        memset(&stPubAttr, 0, sizeof(stPubAttr));
        stPubAttr.enIntfType = AX_VO_INTF_HDMI;
        stPubAttr.enIntfSync = enIntfSync;
        ret = AX_VO_SetPubAttr(gHdmiAttr[pipe->m_vo_attr.hdmi.portid].voDev, &stPubAttr);
        if (0 != ret)
        {
            ALOGE("AX_VO_SetPubAttr(dev %d) fail, ret = 0x%x", gHdmiAttr[pipe->m_vo_attr.hdmi.portid].voDev, ret);
            throw 1;
        }

        ret = AX_VO_Enable(gHdmiAttr[pipe->m_vo_attr.hdmi.portid].voDev);
        if (0 != ret)
        {
            ALOGE("AX_VO_Enable(dev %d) fail, ret = 0x%x", gHdmiAttr[pipe->m_vo_attr.hdmi.portid].voDev, ret);
            throw 1;
        }
        else
        {
            nState |= VODEV_ENABLED;
        }

        if (!CreatePools(gHdmiAttr[pipe->m_vo_attr.hdmi.portid].voDev, gHdmiAttr[pipe->m_vo_attr.hdmi.portid].nPoolCnt, gHdmiAttr[pipe->m_vo_attr.hdmi.portid].stArea.u32Width, gHdmiAttr[pipe->m_vo_attr.hdmi.portid].stArea.u32Height, gHdmiAttr[pipe->m_vo_attr.hdmi.portid].m_LayerPool))
        {
            ALOGE("%s: CreatePool() fail", __func__);
            throw 1;
        }

        AX_VO_VIDEO_LAYER_ATTR_T stLayerAttr;
        memset(&stLayerAttr, 0, sizeof(stLayerAttr));
        stLayerAttr.stDispRect.u32Width = gHdmiAttr[pipe->m_vo_attr.hdmi.portid].stArea.u32Width;
        stLayerAttr.stDispRect.u32Height = gHdmiAttr[pipe->m_vo_attr.hdmi.portid].stArea.u32Height;
        stLayerAttr.stImageSize.u32Width = gHdmiAttr[pipe->m_vo_attr.hdmi.portid].stArea.u32Width;
        stLayerAttr.stImageSize.u32Height = gHdmiAttr[pipe->m_vo_attr.hdmi.portid].stArea.u32Height;
        stLayerAttr.enPixFmt = AX_FORMAT_YUV420_SEMIPLANAR;

        /* if layer bind to dev, enSynMode is ignored */
        stLayerAttr.enSyncMode = AX_VO_LAYER_SYNC_NORMAL;
        // stLayerAttr.uFrmRate.nFrmRateCtrl = gHdmiAttr[pipe->m_vo_attr.hdmi.portid].nHz;
        stLayerAttr.u32FifoDepth = gHdmiAttr[pipe->m_vo_attr.hdmi.portid].nPoolCnt;
        stLayerAttr.u32ChnNr = gHdmiAttr[pipe->m_vo_attr.hdmi.portid].m_arrChns.size();
        // stLayerAttr.u32BkClr = m_stAttr.nBgClr;
        stLayerAttr.u32PrimaryChnId = 0;
        stLayerAttr.enWBMode = AX_VO_LAYER_WB_POOL;
        stLayerAttr.u32PoolId = gHdmiAttr[pipe->m_vo_attr.hdmi.portid].m_LayerPool;
        stLayerAttr.u32DispatchMode = AX_VO_LAYER_OUT_TO_LINK;
        stLayerAttr.enPartMode = AX_VO_PART_MODE_MULTI;
        stLayerAttr.enBlendMode = AX_VO_BLEND_MODE_DEFAULT;
        // stLayerAttr.u32Toleration = m_stAttr.nTolerance;
        ret = AX_VO_SetVideoLayerAttr(gHdmiAttr[pipe->m_vo_attr.hdmi.portid].voLayer, &stLayerAttr);
        if (0 != ret)
        {
            ALOGE("AX_VO_SetVideoLayerAttr(layer %d) fail, ret = 0x%x", gHdmiAttr[pipe->m_vo_attr.hdmi.portid].voLayer, ret);
            throw 1;
        }
        else
        {
            ALOGI("layer %d: [(%d, %d) %dx%d], dispatch mode %d, layer depth %d, ChnNr %d, part mode %d, tolerance %d",
                  gHdmiAttr[pipe->m_vo_attr.hdmi.portid].voLayer, stLayerAttr.stDispRect.u32X, stLayerAttr.stDispRect.u32Y, stLayerAttr.stDispRect.u32Width,
                  stLayerAttr.stDispRect.u32Height, stLayerAttr.u32DispatchMode, stLayerAttr.u32FifoDepth, stLayerAttr.u32ChnNr,
                  stLayerAttr.enPartMode, stLayerAttr.u32Toleration);
        }

        ret = AX_VO_BindVideoLayer(gHdmiAttr[pipe->m_vo_attr.hdmi.portid].voLayer, gHdmiAttr[pipe->m_vo_attr.hdmi.portid].voDev);
        if (0 != ret)
        {
            ALOGE("AX_VO_BindVideoLayer(layer %d dev %d) fail, ret = 0x%x", gHdmiAttr[pipe->m_vo_attr.hdmi.portid].voLayer, gHdmiAttr[pipe->m_vo_attr.hdmi.portid].voDev, ret);
            throw 1;
        }
        else
        {
            nState |= LAYER_BINDED;
        }

        ret = AX_VO_EnableVideoLayer(gHdmiAttr[pipe->m_vo_attr.hdmi.portid].voLayer);
        if (0 != ret)
        {
            ALOGE("AX_VO_EnableVideoLayer(layer %d) fail, ret = 0x%x", gHdmiAttr[pipe->m_vo_attr.hdmi.portid].voLayer, ret);
            throw 1;
        }
        else
        {
            nState |= LAYER_ENABLED;
        }

        auto DisableChns = [](VO_LAYER layer, VO_CHN i) -> AX_VOID
        {
            for (VO_CHN j = 0; j < i; ++j)
            {
                AX_VO_DisableChn(layer, j);
            }
        };
        for (VO_CHN voChn = 0; voChn < stLayerAttr.u32ChnNr; ++voChn)
        {
            ALOGI("enable voChn %d: [(%d, %d) %dx%d], depth %d prior %d", voChn, gHdmiAttr[pipe->m_vo_attr.hdmi.portid].m_arrChns[voChn].stRect.u32X,
                  gHdmiAttr[pipe->m_vo_attr.hdmi.portid].m_arrChns[voChn].stRect.u32Y, gHdmiAttr[pipe->m_vo_attr.hdmi.portid].m_arrChns[voChn].stRect.u32Width, gHdmiAttr[pipe->m_vo_attr.hdmi.portid].m_arrChns[voChn].stRect.u32Height,
                  gHdmiAttr[pipe->m_vo_attr.hdmi.portid].m_arrChns[voChn].u32FifoDepth, gHdmiAttr[pipe->m_vo_attr.hdmi.portid].m_arrChns[voChn].u32Priority);

            ret = AX_VO_SetChnAttr(gHdmiAttr[pipe->m_vo_attr.hdmi.portid].voLayer, voChn, &gHdmiAttr[pipe->m_vo_attr.hdmi.portid].m_arrChns[voChn]);
            if (0 != ret)
            {
                ALOGE("AX_VO_SetChnAttr(layer %d chn %d) fail, ret = 0x%x", gHdmiAttr[pipe->m_vo_attr.hdmi.portid].voLayer, voChn, ret);
                DisableChns(gHdmiAttr[pipe->m_vo_attr.hdmi.portid].voLayer, voChn);
                throw 1;
            }

            /* set default fps for all chns including logo and idle */
            // if (!SetChnFrameRate(voChn, gHdmiAttr[pipe->m_vo_attr.hdmi.portid].voLayer, gHdmiAttr[pipe->m_vo_attr.hdmi.portid].voDev, pipe->m_vo_attr.hdmi.n_frame_rate))
            // {
            //     DisableChns(gHdmiAttr[pipe->m_vo_attr.hdmi.portid].voLayer, voChn);
            //     throw 1;
            // }

            ret = AX_VO_EnableChn(gHdmiAttr[pipe->m_vo_attr.hdmi.portid].voLayer, voChn);
            if (0 != ret)
            {
                ALOGE("AX_VO_EnableChn(layer %d chn %d) fail, ret = 0x%x", gHdmiAttr[pipe->m_vo_attr.hdmi.portid].voLayer, voChn, ret);
                DisableChns(gHdmiAttr[pipe->m_vo_attr.hdmi.portid].voLayer, voChn);
                throw 1;
            }
            pipe->m_vo_attr.hdmi.n_chns_count = voChn + 1;
            pipe->m_vo_attr.hdmi.n_chns[voChn] = voChn;
            pipe->m_vo_attr.hdmi.n_chn_widths[voChn] = gHdmiAttr[pipe->m_vo_attr.hdmi.portid].m_arrChns[voChn].stRect.u32Width;
            pipe->m_vo_attr.hdmi.n_chn_heights[voChn] = gHdmiAttr[pipe->m_vo_attr.hdmi.portid].m_arrChns[voChn].stRect.u32Height;
        }
    }
    catch (...)
    {
        if (LAYER_ENABLED == (nState & LAYER_ENABLED))
        {
            AX_VO_DisableVideoLayer(gHdmiAttr[pipe->m_vo_attr.hdmi.portid].voLayer);
        }

        if (LAYER_BINDED == (nState & LAYER_BINDED))
        {
            AX_VO_UnBindVideoLayer(gHdmiAttr[pipe->m_vo_attr.hdmi.portid].voLayer, gHdmiAttr[pipe->m_vo_attr.hdmi.portid].voDev);
        }

        if (VODEV_ENABLED == (nState & VODEV_ENABLED))
        {
            AX_VO_Disable(gHdmiAttr[pipe->m_vo_attr.hdmi.portid].voDev);
        }

        if (LAYER_CREATED == (nState & LAYER_CREATED))
        {
            AX_VO_DestroyVideoLayer(gHdmiAttr[pipe->m_vo_attr.hdmi.portid].voLayer);
        }

        return -1;
    }

    gHdmiAttr[pipe->m_vo_attr.hdmi.portid].inited = true;
    ALOGI("%s: ---", __func__);
    return 0;
}

int _destory_vo_hdmi(pipeline_t *pipe)
{
    if (!gHdmiAttr[pipe->m_vo_attr.hdmi.portid].inited)
    {
        return 0;
    }

    AX_S32 ret;

    const AX_U32 nCount = gHdmiAttr[pipe->m_vo_attr.hdmi.portid].m_arrChns.size();
    for (VO_CHN voChn = 0; voChn < nCount; ++voChn)
    {
        ret = AX_VO_DisableChn(gHdmiAttr[pipe->m_vo_attr.hdmi.portid].voLayer, voChn);
        if (0 != ret)
        {
            ALOGE("AX_VO_DisableChn(layer %d chn %d) fail, ret = 0x%x", gHdmiAttr[pipe->m_vo_attr.hdmi.portid].voLayer, voChn, ret);
            return ret;
        }
    }

    ret = AX_VO_DisableVideoLayer(gHdmiAttr[pipe->m_vo_attr.hdmi.portid].voLayer);
    if (0 != ret)
    {
        ALOGE("AX_VO_DisableVideoLayer(layer %d) fail, ret = 0x%x", gHdmiAttr[pipe->m_vo_attr.hdmi.portid].voLayer, ret);
        return ret;
    }

    ret = AX_VO_UnBindVideoLayer(gHdmiAttr[pipe->m_vo_attr.hdmi.portid].voLayer, gHdmiAttr[pipe->m_vo_attr.hdmi.portid].voDev);
    if (0 != ret)
    {
        ALOGE("AX_VO_UnBindVideoLayer(layer %d dev %d) fail, ret = 0x%x", gHdmiAttr[pipe->m_vo_attr.hdmi.portid].voLayer, gHdmiAttr[pipe->m_vo_attr.hdmi.portid].voDev, ret);
        return ret;
    }

    ret = AX_VO_Disable(gHdmiAttr[pipe->m_vo_attr.hdmi.portid].voDev);
    if (0 != ret)
    {
        ALOGE("AX_VO_Disable(dev %d) fail, ret = 0x%x", gHdmiAttr[pipe->m_vo_attr.hdmi.portid].voDev, ret);
        return ret;
    }

    ret = AX_VO_DestroyVideoLayer(gHdmiAttr[pipe->m_vo_attr.hdmi.portid].voLayer);
    if (0 != ret)
    {
        ALOGE("AX_VO_DestroyVideoLayer(layer %d) fail, ret = 0x%x", gHdmiAttr[pipe->m_vo_attr.hdmi.portid].voLayer, ret);
        return ret;
    }
    DestoryPool(gHdmiAttr[pipe->m_vo_attr.hdmi.portid].m_LayerPool);

    gHdmiAttr[pipe->m_vo_attr.hdmi.portid].inited = false;
    AX_VO_Deinit();
    ALOGI("%s: ---", __func__);
    return 0;
}

int _sent_frame_vo(pipeline_t *pipe, AX_VIDEO_FRAME_T *tVideoFrame)
{
    AX_S32 ret = AX_VO_SendFrame(gHdmiAttr[pipe->m_vo_attr.hdmi.portid].voLayer, pipe->m_vo_attr.hdmi.n_chn, tVideoFrame, 200);
    return ret;
}
