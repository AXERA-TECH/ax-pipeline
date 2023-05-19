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

#include "ax_ivps_api.h"

class ax_osd_helper
{
private:
    static void osd_thread(volatile int &gLoopExit, void *gModels, std::vector<pipeline_t *> &pipes_need_osd, std::mutex &locker, axdl_results_t &results)
    {
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
            }
            // freeObjs(&mResults);
            usleep(0);
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
            AX_SYS_MemAlloc(&canvas.dataphy, (void **)&canvas.data, pipes_need_osd[i]->m_ivps_attr.n_ivps_width * pipes_need_osd[i]->m_ivps_attr.n_ivps_height * 4, 128, (const AX_S8 *)"osd_image");
            canvas.width = pipes_need_osd[i]->m_ivps_attr.n_ivps_width;
            canvas.height = pipes_need_osd[i]->m_ivps_attr.n_ivps_height;
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
                    axdl_canvas_t &img_overlay = pipes_osd_canvas[osd_pipe->pipeid];
                    AX_IVPS_RGN_DISP_GROUP_T &tDisp = pipes_osd_struct[osd_pipe->pipeid];

                    memset(img_overlay.data, 0, img_overlay.width * img_overlay.height * img_overlay.channel);

                    axdl_draw_results(gModels, &img_overlay, &mResults, 0.6, 1.0, 0, 0);

                    tDisp.nNum = 1;
                    tDisp.tChnAttr.nAlpha = 255;
                    tDisp.tChnAttr.eFormat = AX_FORMAT_RGBA8888;
                    tDisp.tChnAttr.nZindex = 0;
                    tDisp.tChnAttr.nBitColor.nColor = 0xFF0000;
                    // tDisp.tChnAttr.nBitColor.bEnable = AX_FALSE;
                    tDisp.tChnAttr.nBitColor.nColorInv = 0xFF;
                    tDisp.tChnAttr.nBitColor.nColorInvThr = 0xA0A0A0;

                    tDisp.arrDisp[0].bShow = AX_TRUE;
                    tDisp.arrDisp[0].eType = AX_IVPS_RGN_TYPE_OSD;

                    // tDisp.arrDisp[0].uDisp.tOSD.bEnable = AX_TRUE;
                    // tDisp.arrDisp[0].uDisp.tOSD.u16Alpha = 50;
                    tDisp.arrDisp[0].uDisp.tOSD.enRgbFormat = AX_FORMAT_RGBA8888;
                    // tDisp.arrDisp[0].uDisp.tOSD.u32Zindex = 1;
                    // tDisp.arrDisp[0].uDisp.tOSD.u32ColorKey = 0x0;
                    // tDisp.arrDisp[0].uDisp.tOSD.u32BgColorLo = 0xFFFFFFFF;
                    // tDisp.arrDisp[0].uDisp.tOSD.u32BgColorHi = 0xFFFFFFFF;
                    tDisp.arrDisp[0].uDisp.tOSD.u32BmpWidth = img_overlay.width;
                    tDisp.arrDisp[0].uDisp.tOSD.u32BmpHeight = img_overlay.height;
                    tDisp.arrDisp[0].uDisp.tOSD.u32DstXoffset = 0;
                    tDisp.arrDisp[0].uDisp.tOSD.u32DstYoffset = osd_pipe->m_output_type == po_vo_sipeed_maix3_screen ? 32 : 0;
                    // tDisp.arrDisp[0].uDisp.tOSD.u64PhyAddr = 0;
                    tDisp.arrDisp[0].uDisp.tOSD.pBitmap = img_overlay.data;
                    tDisp.arrDisp[0].uDisp.tOSD.u64PhyAddr = img_overlay.dataphy;

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
            // freeObjs(&mResults);
            usleep(0);
        }
        for (size_t i = 0; i < pipes_need_osd.size(); i++)
        {
            auto &canvas = pipes_osd_canvas[pipes_need_osd[i]->pipeid];
            AX_SYS_MemFree(canvas.dataphy, canvas.data);
        }
#endif
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