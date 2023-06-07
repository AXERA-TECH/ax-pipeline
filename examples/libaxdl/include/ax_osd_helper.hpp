#pragma once
#include "mutex"
#include "vector"
#include "map"
#include "thread"
#include "string.h"
#include "unistd.h"

#include "c_api.h"
#include "common_pipeline.h"
#include "../../utilities/sample_log.h"
#include "../../utilities/ringbuffer.hpp"
#include "ax_osd_drawer.hpp"
#include "ax_ivps_api.h"

#include "opencv2/opencv.hpp"

class ax_osd_helper
{
private:
    static void osd_thread(volatile int &gLoopExit, void *gModels, std::vector<pipeline_t *> &pipes_need_osd, std::mutex &locker, axdl_results_t &results)
    {
        ALOGI("osd thread:++++");
#ifdef AXERA_TARGET_CHIP_AX620
        std::map<int, axdl_canvas_t> pipes_osd_canvas;
        std::map<int, AX_IVPS_RGN_DISP_GROUP_S> pipes_osd_struct;
        for (size_t i = 0; i < pipes_need_osd.size(); i++)
        {
            pipes_osd_canvas[pipes_need_osd[i]->pipeid];
            pipes_osd_struct[pipes_need_osd[i]->pipeid];
            auto &canvas = pipes_osd_canvas[pipes_need_osd[i]->pipeid];
            auto &tDisp = pipes_osd_struct[pipes_need_osd[i]->pipeid];
            memset(&tDisp, 0, sizeof(AX_IVPS_RGN_DISP_GROUP_S));
            canvas.channel = 4;
            canvas.data = (unsigned char *)malloc(pipes_need_osd[i]->m_ivps_attr.n_ivps_width * pipes_need_osd[i]->m_ivps_attr.n_ivps_height * 4);
            canvas.width = pipes_need_osd[i]->m_ivps_attr.n_ivps_width;
            canvas.height = pipes_need_osd[i]->m_ivps_attr.n_ivps_height;

            axdl_native_osd_init(gModels, pipes_need_osd[i]->pipeid, pipes_need_osd[i]->m_ivps_attr.n_ivps_width, pipes_need_osd[i]->m_ivps_attr.n_ivps_height, pipes_need_osd[i]->m_ivps_attr.n_osd_rgn);
        }
        axdl_results_t mResults;
        while (!gLoopExit)
        {
            locker.lock();
            memcpy(&mResults, &results, sizeof(axdl_results_t));
            locker.unlock();
            for (size_t i = 0; i < pipes_need_osd.size(); i++)
            {
                auto &osd_pipe = pipes_need_osd[i];
                if (osd_pipe && osd_pipe->m_ivps_attr.n_osd_rgn)
                {
                    if (false)
                    {
                        axdl_canvas_t &img_overlay = pipes_osd_canvas[osd_pipe->pipeid];
                        AX_IVPS_RGN_DISP_GROUP_S &tDisp = pipes_osd_struct[osd_pipe->pipeid];

                        memset(img_overlay.data, 0, img_overlay.width * img_overlay.height * img_overlay.channel);

                        axdl_draw_results(gModels, &img_overlay, &mResults, 0.6, 1.0, 0, 0);

                        tDisp.nNum = 1;
                        tDisp.tChnAttr.nAlpha = 1024;
                        tDisp.tChnAttr.eFormat = AX_FORMAT_RGBA8888;
                        tDisp.tChnAttr.nZindex = 1;
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
                        tDisp.arrDisp[0].uDisp.tOSD.u32BmpWidth = img_overlay.width;
                        tDisp.arrDisp[0].uDisp.tOSD.u32BmpHeight = img_overlay.height;
                        tDisp.arrDisp[0].uDisp.tOSD.u32DstXoffset = 0;
                        tDisp.arrDisp[0].uDisp.tOSD.u32DstYoffset = osd_pipe->m_output_type == po_vo_sipeed_maix3_screen ? 32 : 0;
                        tDisp.arrDisp[0].uDisp.tOSD.u64PhyAddr = 0;
                        tDisp.arrDisp[0].uDisp.tOSD.pBitmap = img_overlay.data;

                        int ret = AX_IVPS_RGN_Update(osd_pipe->m_ivps_attr.n_osd_rgn_chn[0], &tDisp);
                        if (0 != ret)
                        {
                            static int cnt = 0;
                            if (cnt++ % 100 == 0)
                            {
                                ALOGE("AX_IVPS_RGN_Update fail, ret=0x%x, hChnRgn=%d", ret, osd_pipe->m_ivps_attr.n_osd_rgn_chn[0]);
                            }
                            usleep(30 * 1000);
                        }
                    }
                    else
                    {
                        axdl_native_osd_draw_results(gModels, osd_pipe->pipeid, &mResults, 0.6, 2);
                        AX_IVPS_RGN_DISP_GROUP_S *rgn_disp_grp = (AX_IVPS_RGN_DISP_GROUP_S *)axdl_native_osd_get_handle(gModels, osd_pipe->pipeid);
                        if (rgn_disp_grp)
                        {
                            for (size_t d = 0; d < osd_pipe->m_ivps_attr.n_osd_rgn; d++)
                            {
                                if (rgn_disp_grp[d].nNum > 0 && rgn_disp_grp[d].nNum <= AX_IVPS_REGION_MAX_DISP_NUM)
                                {
                                    if (osd_pipe->m_output_type == po_vo_sipeed_maix3_screen)
                                    {
                                        for (size_t k = 0; k < rgn_disp_grp[d].nNum; k++)
                                        {
                                            if (rgn_disp_grp[d].arrDisp[k].bShow)
                                            {
                                                switch (rgn_disp_grp[d].arrDisp[k].eType)
                                                {
                                                case AX_IVPS_RGN_TYPE_LINE:
                                                    for (size_t j = 0; j < 10; j++)
                                                    {
                                                        rgn_disp_grp[d].arrDisp[k].uDisp.tLine.tPTs[j].nY += 32;
                                                    }
                                                    break;
                                                case AX_IVPS_RGN_TYPE_RECT:
                                                    rgn_disp_grp[d].arrDisp[k].uDisp.tPolygon.tRect.nY += 32;
                                                    break;
                                                case AX_IVPS_RGN_TYPE_POLYGON:
                                                    for (size_t j = 0; j < 10; j++)
                                                    {
                                                        rgn_disp_grp[d].arrDisp[k].uDisp.tPolygon.tPTs[j].nY += 32;
                                                    }
                                                    break;
                                                case AX_IVPS_RGN_TYPE_MOSAIC:
                                                    rgn_disp_grp[d].arrDisp[k].uDisp.tMosaic.tRect.nY += 32;
                                                    break;
                                                case AX_IVPS_RGN_TYPE_OSD:
                                                    rgn_disp_grp[d].arrDisp[k].uDisp.tOSD.u32DstYoffset += 32;
                                                    break;
                                                default:
                                                    break;
                                                }
                                            }
                                        }
                                    }
                                    int ret = AX_IVPS_RGN_Update(osd_pipe->m_ivps_attr.n_osd_rgn_chn[d], &rgn_disp_grp[d]);
                                    if (0 != ret)
                                    {
                                        static int cnt = 0;
                                        if (cnt++ % 100 == 0)
                                        {
                                            ALOGE("AX_IVPS_RGN_Update fail, ret=0x%x, hChnRgn=%d", ret, osd_pipe->m_ivps_attr.n_osd_rgn_chn[0]);
                                        }
                                        usleep(30 * 1000);
                                    }
                                }
                            }
                        }
                    }
                }
            }
            // freeObjs(&mResults);
            usleep(1 * 1000);
        }
        for (size_t i = 0; i < pipes_need_osd.size(); i++)
        {
            auto &canvas = pipes_osd_canvas[pipes_need_osd[i]->pipeid];
            free(canvas.data);
        }
#elif defined(AXERA_TARGET_CHIP_AX650)

        std::map<int, axdl_canvas_t> pipes_osd_canvas;
        std::map<int, AX_IVPS_RGN_DISP_GROUP_T> pipes_osd_struct;
        for (size_t i = 0; i < pipes_need_osd.size(); i++)
        {
            pipes_osd_canvas[pipes_need_osd[i]->pipeid];
            pipes_osd_struct[pipes_need_osd[i]->pipeid];
            auto &canvas = pipes_osd_canvas[pipes_need_osd[i]->pipeid];
            auto &tDisp = pipes_osd_struct[pipes_need_osd[i]->pipeid];
            memset(&tDisp, 0, sizeof(AX_IVPS_RGN_DISP_GROUP_T));
            canvas.channel = 4;
            canvas.data = (unsigned char *)malloc(pipes_need_osd[i]->m_ivps_attr.n_ivps_width * pipes_need_osd[i]->m_ivps_attr.n_ivps_height * 4);
            canvas.width = pipes_need_osd[i]->m_ivps_attr.n_ivps_width;
            canvas.height = pipes_need_osd[i]->m_ivps_attr.n_ivps_height;

            axdl_native_osd_init(gModels, pipes_need_osd[i]->pipeid, pipes_need_osd[i]->m_ivps_attr.n_ivps_width, pipes_need_osd[i]->m_ivps_attr.n_ivps_height, pipes_need_osd[i]->m_ivps_attr.n_osd_rgn);
        }

        axdl_results_t mResults;

        while (!gLoopExit)
        {
            locker.lock();
            memcpy(&mResults, &results, sizeof(axdl_results_t));
            locker.unlock();
            for (size_t i = 0; i < pipes_need_osd.size(); i++)
            {
                auto &osd_pipe = pipes_need_osd[i];
                if (osd_pipe && osd_pipe->m_ivps_attr.n_osd_rgn)
                {
                    if (true)
                    {
                        axdl_native_osd_draw_results(gModels, osd_pipe->pipeid, &mResults, 0.6, 2);
                        AX_IVPS_RGN_DISP_GROUP_T *rgn_disp_grp = (AX_IVPS_RGN_DISP_GROUP_T *)axdl_native_osd_get_handle(gModels, osd_pipe->pipeid);
                        if (rgn_disp_grp)
                        {
                            for (size_t d = 0; d < osd_pipe->m_ivps_attr.n_osd_rgn; d++)
                            {
                                if (rgn_disp_grp[d].nNum > 0 && rgn_disp_grp[d].nNum <= AX_IVPS_REGION_MAX_DISP_NUM)
                                {
                                    int ret = AX_IVPS_RGN_Update(osd_pipe->m_ivps_attr.n_osd_rgn_chn[d], &rgn_disp_grp[d]);
                                    if (0 != ret)
                                    {
                                        static int cnt = 0;
                                        if (cnt++ % 100 == 0)
                                        {
                                            ALOGE("AX_IVPS_RGN_Update fail, ret=0x%x, hChnRgn=%d", ret, osd_pipe->m_ivps_attr.n_osd_rgn_chn[0]);
                                        }
                                        usleep(30 * 1000);
                                    }
                                }
                            }
                        }
                    }
                    else
                    {
                        axdl_canvas_t &img_overlay = pipes_osd_canvas[osd_pipe->pipeid];
                        AX_IVPS_RGN_DISP_GROUP_T &tDisp = pipes_osd_struct[osd_pipe->pipeid];

                        memset(img_overlay.data, 0, img_overlay.width * img_overlay.height * img_overlay.channel);

                        axdl_draw_results(gModels, &img_overlay, &mResults, 0.6, 1.0, 0, 0);

                        tDisp.nNum = 1;
                        tDisp.tChnAttr.nAlpha = 255;
                        tDisp.tChnAttr.eFormat = AX_FORMAT_RGBA8888;
                        tDisp.tChnAttr.nZindex = 0;
                        tDisp.tChnAttr.nBitColor.nColor = 0xFF0000;
                        tDisp.tChnAttr.nBitColor.nColorInv = 0xFF;
                        tDisp.tChnAttr.nBitColor.nColorInvThr = 0xA0A0A0;

                        tDisp.arrDisp[0].bShow = AX_TRUE;
                        tDisp.arrDisp[0].eType = AX_IVPS_RGN_TYPE_OSD;
                        tDisp.arrDisp[0].uDisp.tOSD.enRgbFormat = AX_FORMAT_RGBA8888;
                        tDisp.arrDisp[0].uDisp.tOSD.u32BmpWidth = img_overlay.width;
                        tDisp.arrDisp[0].uDisp.tOSD.u32BmpHeight = img_overlay.height;
                        tDisp.arrDisp[0].uDisp.tOSD.u32DstXoffset = 0;
                        tDisp.arrDisp[0].uDisp.tOSD.u32DstYoffset = osd_pipe->m_output_type == po_vo_sipeed_maix3_screen ? 32 : 0;
                        tDisp.arrDisp[0].uDisp.tOSD.u64PhyAddr = 0;
                        tDisp.arrDisp[0].uDisp.tOSD.pBitmap = img_overlay.data;

                        int ret = AX_IVPS_RGN_Update(osd_pipe->m_ivps_attr.n_osd_rgn_chn[0], &tDisp);
                        if (0 != ret)
                        {
                            static int cnt = 0;
                            if (cnt++ % 100 == 0)
                            {
                                ALOGE("AX_IVPS_RGN_Update fail, ret=0x%x, hChnRgn=%d", ret, osd_pipe->m_ivps_attr.n_osd_rgn_chn[0]);
                            }
                            usleep(30 * 1000);
                        }
                    }
                }
            }

            usleep(1 * 1000);
        }
        for (size_t i = 0; i < pipes_need_osd.size(); i++)
        {
            auto &canvas = pipes_osd_canvas[pipes_need_osd[i]->pipeid];
            free(canvas.data);
        }
#endif
        ALOGI("osd thread:----");
    }

    axdl_results_t results;
    std::thread th_osd;
    std::mutex locker;
    volatile int gLoopExit = 0;
    void *gModels = nullptr;

public:
    ax_osd_helper()
    {
    }
    ~ax_osd_helper()
    {
        Stop();
    }

    void Start(void *models, std::vector<pipeline_t *> &pipes_need_osd)
    {
        gModels = models;
        gLoopExit = 0;
        th_osd = std::thread(ax_osd_helper::osd_thread, std::ref(gLoopExit), gModels, std::ref(pipes_need_osd), std::ref(locker), std::ref(results));
    }

    void Stop()
    {
        gLoopExit = 1;
        th_osd.join();
    }

    void Update(axdl_results_t *pResults)
    {
        std::lock_guard<std::mutex> tmplocker(locker);
        memcpy(&results, pResults, sizeof(axdl_results_t));
    }
};