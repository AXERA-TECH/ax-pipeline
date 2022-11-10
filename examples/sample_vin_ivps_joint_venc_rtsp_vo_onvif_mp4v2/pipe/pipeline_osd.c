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
#include "../../sample_run_joint/sample_run_joint.h"
#include "../../utilities/osd_utils.h"

sample_run_joint_results pResult_disp;

AX_U32 OSD_Grp[SAMPLE_REGION_COUNT] = {0, 2, 0};

AX_VOID StartOverlay(AX_VOID)
{
    memset(&g_arrRgnThreadParam[0], 0, sizeof(IVPS_REGION_PARAM_T) * SAMPLE_REGION_COUNT);

    AX_S32 nRet = 0;
    AX_U8 nRgnIndex = 0;

    for (AX_U8 i = 0; i < SAMPLE_REGION_COUNT; i++)
    {
        IVPS_RGN_HANDLE hChnRgn = AX_IVPS_RGN_Create();
        if (AX_IVPS_INVALID_REGION_HANDLE != hChnRgn)
        {
            AX_S32 nFilter = 0x11;
            if(i == 2)
                nFilter = 0x21;
            nRet = AX_IVPS_RGN_AttachToFilter(hChnRgn, OSD_Grp[i], nFilter);
            if (0 != nRet)
            {
                ALOGE("AX_IVPS_RGN_AttachToFilter(Grp: %d, Filter: 0x%x) failed, ret=0x%x", OSD_Grp[i], nFilter, nRet);
                return;
            }

            g_arrRgnThreadParam[nRgnIndex].hChnRgn = hChnRgn;
            g_arrRgnThreadParam[nRgnIndex].nGroupIdx = OSD_Grp[i];
            g_arrRgnThreadParam[nRgnIndex].nFilter = nFilter;
            nRgnIndex++;

            ALOGN("AX_IVPS_RGN_AttachToFilter(Grp: %d, Filter: 0x%x) successfully, hChnRgn=%d.", i, nFilter, hChnRgn);
        }
        else
        {
            ALOGE("AX_IVPS_RGN_Create(Grp: %d) failed.", i);
        }
    }
}

AX_VOID StopOverlay(AX_VOID)
{
    AX_S32 nRet = 0;
    AX_U8 nRgnIndex = 0;

    for (AX_U8 i = 0; i < SAMPLE_REGION_COUNT; i++)
    {
        g_arrRgnThreadParam[i].bExit = AX_TRUE;
    }

    for (AX_U8 i = 0; i < SAMPLE_REGION_COUNT; i++)
    {
        AX_S32 nFilter = 0x11;
        if(i == 2)
            nFilter = 0x21;
        nRet = AX_IVPS_RGN_DetachFromFilter(g_arrRgnThreadParam[nRgnIndex].hChnRgn, OSD_Grp[i], nFilter);
        if (0 != nRet)
        {
            ALOGE("AX_IVPS_RGN_DetachFromFilter(Grp: %d, Filter: %x) failed, ret=0x%x", i, nFilter, nRet);
        }

        nRet = AX_IVPS_RGN_Destroy(g_arrRgnThreadParam[nRgnIndex].hChnRgn);
        if (0 != nRet)
        {
            ALOGE("AX_IVPS_RGN_Destroy(Grp: %d) failed, ret=0x%x", i, nRet);
        }

        nRgnIndex++;
    }
}

AX_VOID *RgnThreadFunc(AX_VOID *pArg)
{
    if (!pArg)
    {
        return (AX_VOID *)0;
    }

    prctl(PR_SET_NAME, "SAMPLE_IVPS_RGN");

    RGN_GROUP_CFG_T tRgnGroupConfig[SAMPLE_REGION_COUNT] = {
        {OSD_Grp[0], 0x11, SAMPLE_MAJOR_STREAM_WIDTH, SAMPLE_MAJOR_STREAM_HEIGHT, AX_IVPS_REGION_MAX_DISP_NUM, AX_IVPS_RGN_LAYER_COVER},
        {OSD_Grp[1], 0x11, SAMPLE_MINOR_STREAM_WIDTH, SAMPLE_MINOR_STREAM_HEIGHT, AX_IVPS_REGION_MAX_DISP_NUM, AX_IVPS_RGN_LAYER_COVER},
        {OSD_Grp[2], 0x21, SAMPLE_MINOR_STREAM_WIDTH, SAMPLE_MINOR_STREAM_HEIGHT, AX_IVPS_REGION_MAX_DISP_NUM, AX_IVPS_RGN_LAYER_COVER},
    };

    IVPS_REGION_PARAM_PTR pThreadParam = (IVPS_REGION_PARAM_PTR)pArg;
    AX_IVPS_FILTER nFilter = pThreadParam->nFilter;
    IVPS_GRP nIvpsGrp = pThreadParam->nGroupIdx;

    AX_U8 nCfgIndex = nIvpsGrp;

    ALOGN("[%d][0x%02x] +++", nIvpsGrp, nFilter);

    osd_utils_img list_baseimgs[SAMPLE_RECT_BOX_COUNT];
    for (AX_U8 i = 0; i < SAMPLE_RECT_BOX_COUNT; ++i)
    {
        genImg(SAMPLE_OBJ_NAME_MAX_LEN, nCfgIndex == 0 ? 2.0 : 0.6, nCfgIndex == 0 ? 2 : 1, &list_baseimgs[i]);
    }

    AX_S32 ret = 0;
    pThreadParam->bExit = AX_FALSE;
    while (!pThreadParam->bExit && !gLoopExit)
    {
        RGN_GROUP_CFG_T *tGrpCfg = &tRgnGroupConfig[nCfgIndex];
        if (0 == tGrpCfg->nRgnNum)
        {
            break;
        }
        pthread_mutex_lock(&g_result_mutex);

        AX_IVPS_RGN_DISP_GROUP_S tDisp;
        memset(&tDisp, 0, sizeof(AX_IVPS_RGN_DISP_GROUP_S));

        tDisp.nNum = tGrpCfg->nRgnNum;
        tDisp.tChnAttr.nAlpha = 1024;
        tDisp.tChnAttr.eFormat = AX_FORMAT_RGBA8888;
        tDisp.tChnAttr.nZindex = nIvpsGrp + 1;
        tDisp.tChnAttr.nBitColor.nColor = 0xFF0000;
        tDisp.tChnAttr.nBitColor.bEnable = AX_FALSE;
        tDisp.tChnAttr.nBitColor.nColorInv = 0xFF;
        tDisp.tChnAttr.nBitColor.nColorInvThr = 0xA0A0A0;

#pragma omp parallel for num_threads(2)
        for (AX_U8 i = 0; i < tDisp.nNum; ++i)
        {
            if (i < g_result_disp.size)
            {
                // bbox
                {
                    tDisp.arrDisp[i].bShow = AX_TRUE;
                    tDisp.arrDisp[i].eType = AX_IVPS_RGN_TYPE_RECT;

                    AX_U32 nChnWidth = tGrpCfg->nChnWidth;
                    AX_U32 nChnHeight = tGrpCfg->nChnHeight;

                    tDisp.arrDisp[i].uDisp.tPolygon.tRect.nX = (AX_U32)(g_result_disp.objects[i].x * nChnWidth);
                    tDisp.arrDisp[i].uDisp.tPolygon.tRect.nY = (AX_U32)(g_result_disp.objects[i].y * nChnHeight) + (nCfgIndex == 0 ? 0 : 32);
                    tDisp.arrDisp[i].uDisp.tPolygon.tRect.nW = (AX_U32)(g_result_disp.objects[i].w * nChnWidth);
                    tDisp.arrDisp[i].uDisp.tPolygon.tRect.nH = (AX_U32)(g_result_disp.objects[i].h * nChnHeight);
                    tDisp.arrDisp[i].uDisp.tPolygon.bSolid = AX_FALSE;
                    tDisp.arrDisp[i].uDisp.tPolygon.bCornerRect = AX_FALSE;
                    tDisp.arrDisp[i].uDisp.tPolygon.nLineWidth = 2;
                    tDisp.arrDisp[i].uDisp.tPolygon.nColor = GREEN;
                    tDisp.arrDisp[i].uDisp.tPolygon.nAlpha = 255;
                }
                // text
                {
                    tDisp.arrDisp[i + SAMPLE_RECT_BOX_COUNT].bShow = AX_TRUE;
                    tDisp.arrDisp[i + SAMPLE_RECT_BOX_COUNT].eType = AX_IVPS_RGN_TYPE_OSD;

                    AX_U32 nChnWidth = tGrpCfg->nChnWidth;
                    AX_U32 nChnHeight = tGrpCfg->nChnHeight;
                    osd_utils_img out;
                    putText(g_result_disp.objects[i].objname, nCfgIndex == 0 ? 2.0 : 0.6, nCfgIndex == 0 ? 2 : 1, &list_baseimgs[i], &out);

                    tDisp.arrDisp[i + SAMPLE_RECT_BOX_COUNT].uDisp.tOSD.bEnable = AX_TRUE;
                    tDisp.arrDisp[i + SAMPLE_RECT_BOX_COUNT].uDisp.tOSD.enRgbFormat = AX_FORMAT_RGBA8888;
                    tDisp.arrDisp[i + SAMPLE_RECT_BOX_COUNT].uDisp.tOSD.u32Zindex = 1;
                    tDisp.arrDisp[i + SAMPLE_RECT_BOX_COUNT].uDisp.tOSD.u32ColorKey = 0x0;
                    tDisp.arrDisp[i + SAMPLE_RECT_BOX_COUNT].uDisp.tOSD.u32BgColorLo = 0xFFFFFFFF;
                    tDisp.arrDisp[i + SAMPLE_RECT_BOX_COUNT].uDisp.tOSD.u32BgColorHi = 0xFFFFFFFF;
                    tDisp.arrDisp[i + SAMPLE_RECT_BOX_COUNT].uDisp.tOSD.u32BmpWidth = out.width;
                    tDisp.arrDisp[i + SAMPLE_RECT_BOX_COUNT].uDisp.tOSD.u32BmpHeight = out.height;
                    tDisp.arrDisp[i + SAMPLE_RECT_BOX_COUNT].uDisp.tOSD.u32DstXoffset = tDisp.arrDisp[i].uDisp.tPolygon.tRect.nX;
                    tDisp.arrDisp[i + SAMPLE_RECT_BOX_COUNT].uDisp.tOSD.u32DstYoffset = tDisp.arrDisp[i].uDisp.tPolygon.tRect.nY;
                    tDisp.arrDisp[i + SAMPLE_RECT_BOX_COUNT].uDisp.tOSD.u64PhyAddr = 0;
                    tDisp.arrDisp[i + SAMPLE_RECT_BOX_COUNT].uDisp.tOSD.pBitmap = out.data;
                }
            }
        }

        ret = AX_IVPS_RGN_Update(pThreadParam->hChnRgn, &tDisp);
        if (0 != ret)
        {
            ALOGE("[%d][0x%02x] AX_IVPS_RGN_Update fail, ret=0x%x, hChnRgn=%d", nIvpsGrp, nFilter, ret, pThreadParam->hChnRgn);
        }

        pthread_mutex_unlock(&g_result_mutex);
        usleep(10000);
    }

    for (AX_U8 i = 0; i < SAMPLE_RECT_BOX_COUNT; ++i)
    {
        releaseImg(&list_baseimgs[i]);
    }

    ALOGN("[%d][0x%02x] ---", nIvpsGrp, nFilter);

    return (AX_VOID *)0;
}

AX_VOID *RgnThreadFunc_V2(AX_VOID *pArg)
{
    if (!pArg)
    {
        return (AX_VOID *)0;
    }

    prctl(PR_SET_NAME, "SAMPLE_IVPS_RGN");

    RGN_GROUP_CFG_T tRgnGroupConfig[SAMPLE_REGION_COUNT] = {
        {OSD_Grp[0], 0x11, SAMPLE_MAJOR_STREAM_WIDTH, SAMPLE_MAJOR_STREAM_HEIGHT, 1, AX_IVPS_RGN_LAYER_COVER},
        {OSD_Grp[1], 0x11, SAMPLE_MINOR_STREAM_WIDTH, SAMPLE_MINOR_STREAM_HEIGHT, 1, AX_IVPS_RGN_LAYER_COVER},
        {OSD_Grp[2], 0x21, SAMPLE_FHD_STREAM_WIDTH, SAMPLE_FHD_STREAM_HEIGHT, 1, AX_IVPS_RGN_LAYER_COVER},
    };
    //tRgnGroupConfig[nCfgIndex].nGroup
    IVPS_REGION_PARAM_T *g_arrRgnThreadParam = pArg;

    osd_utils_img img_overlay[SAMPLE_REGION_COUNT];
    for (int i = 0; i < SAMPLE_REGION_COUNT; i++)
    {
        ALOGN("[%d][0x%02x] +++", g_arrRgnThreadParam[i].nGroupIdx, g_arrRgnThreadParam[i].nFilter);
        g_arrRgnThreadParam[i].bExit = AX_FALSE;
        img_overlay[i].channel = 4;
        img_overlay[i].data = malloc(tRgnGroupConfig[i].nChnWidth * tRgnGroupConfig[i].nChnHeight * 4);
        img_overlay[i].width = tRgnGroupConfig[i].nChnWidth;
        img_overlay[i].height = tRgnGroupConfig[i].nChnHeight;
    }

    AX_S32 ret = 0;
    AX_IVPS_RGN_DISP_GROUP_S tDisp;
    while (!g_arrRgnThreadParam[0].bExit && !g_arrRgnThreadParam[1].bExit && !gLoopExit)
    {
        pthread_mutex_lock(&g_result_mutex);
        for (int i = 0; i < SAMPLE_REGION_COUNT; i++)
        {
            RGN_GROUP_CFG_T *tGrpCfg = &tRgnGroupConfig[i];
            if (0 == tGrpCfg->nRgnNum)
            {
                break;
            }

            memset(img_overlay[i].data, 0, tRgnGroupConfig[i].nChnWidth * tRgnGroupConfig[i].nChnHeight * 4);
            drawObjs(&img_overlay[i], i == 1 ? 0.6 : 1.8, i == 1 ? 1.0 : 2.0, &g_result_disp, 0, 0);

            memset(&tDisp, 0, sizeof(AX_IVPS_RGN_DISP_GROUP_S));

            tDisp.nNum = tGrpCfg->nRgnNum;
            tDisp.tChnAttr.nAlpha = 1024;
            tDisp.tChnAttr.eFormat = AX_FORMAT_RGBA8888;
            tDisp.tChnAttr.nZindex = i + 1;
            tDisp.tChnAttr.nBitColor.nColor = 0xFF0000;
            tDisp.tChnAttr.nBitColor.bEnable = AX_FALSE;
            tDisp.tChnAttr.nBitColor.nColorInv = 0xFF;
            tDisp.tChnAttr.nBitColor.nColorInvThr = 0xA0A0A0;

            tDisp.arrDisp[0].bShow = AX_TRUE;
            tDisp.arrDisp[0].eType = AX_IVPS_RGN_TYPE_OSD;

            tDisp.arrDisp[0].uDisp.tOSD.bEnable = AX_TRUE;
            tDisp.arrDisp[0].uDisp.tOSD.enRgbFormat = AX_FORMAT_RGBA8888;
            tDisp.arrDisp[0].uDisp.tOSD.u32Zindex = 1;
            tDisp.arrDisp[0].uDisp.tOSD.u32ColorKey = 0x0;
            tDisp.arrDisp[0].uDisp.tOSD.u32BgColorLo = 0xFFFFFFFF;
            tDisp.arrDisp[0].uDisp.tOSD.u32BgColorHi = 0xFFFFFFFF;
            tDisp.arrDisp[0].uDisp.tOSD.u32BmpWidth = tRgnGroupConfig[i].nChnWidth;
            tDisp.arrDisp[0].uDisp.tOSD.u32BmpHeight = tRgnGroupConfig[i].nChnHeight;
            tDisp.arrDisp[0].uDisp.tOSD.u32DstXoffset = 0;
            tDisp.arrDisp[0].uDisp.tOSD.u32DstYoffset = i == 1 ? 32 : 0;
            tDisp.arrDisp[0].uDisp.tOSD.u64PhyAddr = 0;
            tDisp.arrDisp[0].uDisp.tOSD.pBitmap = img_overlay[i].data;

            ret = AX_IVPS_RGN_Update(g_arrRgnThreadParam[i].hChnRgn, &tDisp);
            if (0 != ret)
            {
                ALOGE("[%d][0x%02x] AX_IVPS_RGN_Update fail, ret=0x%x, hChnRgn=%d", i, g_arrRgnThreadParam[i].nFilter, ret, g_arrRgnThreadParam[i].hChnRgn);
            }
        }
        pthread_mutex_unlock(&g_result_mutex);

        usleep(10000);
    }

    releaseImg(&img_overlay);

    for (int i = 0; i < SAMPLE_REGION_COUNT; i++)
    {
        ALOGN("[%d][0x%02x] ---", g_arrRgnThreadParam[i].nGroupIdx, g_arrRgnThreadParam[i].nFilter);
    }

    return (AX_VOID *)0;
}