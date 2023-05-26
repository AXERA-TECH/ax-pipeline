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
        std::map<int, std::vector<AX_IVPS_RGN_DISP_GROUP_T>> pipes_osd_struct;
        std::vector<ax_osd_drawer> m_drawer(pipes_need_osd.size());
        for (size_t i = 0; i < pipes_need_osd.size(); i++)
        {
            pipes_osd_canvas[pipes_need_osd[i]->pipeid];
            pipes_osd_struct[pipes_need_osd[i]->pipeid];
            auto &canvas = pipes_osd_canvas[pipes_need_osd[i]->pipeid];
            auto &tDisp = pipes_osd_struct[pipes_need_osd[i]->pipeid];
            tDisp.resize(pipes_need_osd[i]->m_ivps_attr.n_osd_rgn);
            // memset(&tDisp, 0, sizeof(AX_IVPS_RGN_DISP_GROUP_T));
            canvas.channel = 4;
            AX_SYS_MemAlloc(&canvas.dataphy, (void **)&canvas.data, pipes_need_osd[i]->m_ivps_attr.n_ivps_width * pipes_need_osd[i]->m_ivps_attr.n_ivps_height * 4, 128, (const AX_S8 *)"osd_image");
            canvas.width = pipes_need_osd[i]->m_ivps_attr.n_ivps_width;
            canvas.height = pipes_need_osd[i]->m_ivps_attr.n_ivps_height;
            m_drawer[i].init(pipes_need_osd[i]->m_ivps_attr.n_osd_rgn, pipes_need_osd[i]->m_ivps_attr.n_ivps_width, pipes_need_osd[i]->m_ivps_attr.n_ivps_height);

            axdl_native_osd_init(gModels, pipes_need_osd[i]->pipeid, pipes_need_osd[i]->m_ivps_attr.n_ivps_width, pipes_need_osd[i]->m_ivps_attr.n_ivps_height, pipes_need_osd[i]->m_ivps_attr.n_osd_rgn);
        }
        axdl_results_t mResults;

        ax_osd_drawer::ax_abgr_t color, color_pose, color_text;
        color.abgr[0] = color.abgr[1] = color.abgr[2] = color.abgr[3] = 255;
        color_pose.abgr[0] = color_pose.abgr[1] = color_pose.abgr[2] = color_pose.abgr[3] = 255;
        color_pose.abgr[1] = color_pose.abgr[2] = 0;
        color_text.abgr[0] = color_text.abgr[1] = color_text.abgr[2] = color_text.abgr[3] = 255;
        color_text.abgr[1] = color_text.abgr[2] = color_text.abgr[3] = 0;

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

                    // for (size_t g = 0; g < pipes_osd_struct[osd_pipe->pipeid].size(); g++)
                    // {
                    //
                    // memset(img_overlay.data, 0, img_overlay.width * img_overlay.height * img_overlay.channel);

                    if (false)
                    {
                        axdl_canvas_t &img_overlay = pipes_osd_canvas[osd_pipe->pipeid];
                        AX_IVPS_RGN_DISP_GROUP_T &tDisp = pipes_osd_struct[osd_pipe->pipeid][0];

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
                    else if (true)
                    {
                        axdl_native_osd_draw_results(gModels, osd_pipe->pipeid, &mResults, 0.6, 1.0);
                        AX_IVPS_RGN_DISP_GROUP_T *rgn_disp_grp = (AX_IVPS_RGN_DISP_GROUP_T *)axdl_native_osd_get_handle(gModels, osd_pipe->pipeid);
                        if (rgn_disp_grp)
                        {
                            for (size_t d = 0; d < osd_pipe->m_ivps_attr.n_osd_rgn; d++)
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
                    else
                    {
                        m_drawer[i].reset();

                        static std::vector<int> head{4, 2, 0, 1, 3};
                        static std::vector<int> hand_arm{10, 8, 6, 5, 7, 9};
                        static std::vector<int> leg{16, 14, 12, 6, 12, 11, 5, 11, 13, 15};
                        std::vector<axdl_point_t> pts(leg.size());
                        for (size_t d = 0; d < mResults.nObjSize; d++)
                        {
                            m_drawer[i].add_rect(&mResults.mObjects[d].bbox, color, 3);
                            if (mResults.bObjTrack)
                            {
                                m_drawer[i].add_text(std::string(mResults.mObjects[d].objname) + " " + std::to_string(mResults.mObjects[d].track_id),
                                                     {mResults.mObjects[d].bbox.x, mResults.mObjects[d].bbox.y},
                                                     {UCHAR_MAX, 0, 0, 0}, 1, 2);
                            }
                            else
                            {
                                m_drawer[i].add_text(mResults.mObjects[d].objname,
                                                     {mResults.mObjects[d].bbox.x, mResults.mObjects[d].bbox.y},
                                                     {UCHAR_MAX, 0, 0, 0}, 1, 2);
                            }

                            if (mResults.mObjects[d].nLandmark == SAMPLE_BODY_LMK_SIZE)
                            {
                                for (size_t k = 0; k < head.size(); k++)
                                {
                                    pts[k].x = mResults.mObjects[d].landmark[head[k]].x;
                                    pts[k].y = mResults.mObjects[d].landmark[head[k]].y;
                                }
                                m_drawer[i].add_line(pts.data(), head.size(), color_pose, 3);
                                for (size_t k = 0; k < hand_arm.size(); k++)
                                {
                                    pts[k].x = mResults.mObjects[d].landmark[hand_arm[k]].x;
                                    pts[k].y = mResults.mObjects[d].landmark[hand_arm[k]].y;
                                }
                                m_drawer[i].add_line(pts.data(), hand_arm.size(), color_pose, 3);
                                for (size_t k = 0; k < leg.size(); k++)
                                {
                                    pts[k].x = mResults.mObjects[d].landmark[leg[k]].x;
                                    pts[k].y = mResults.mObjects[d].landmark[leg[k]].y;
                                }
                                m_drawer[i].add_line(pts.data(), leg.size(), color_pose, 3);
                            }
                            else if (mResults.mObjects[d].nLandmark == SAMPLE_FACE_LMK_SIZE)
                            {
                                for (size_t k = 0; k < 5; k++)
                                {
                                    m_drawer[i].add_point(&mResults.mObjects[d].landmark[k], {255, 0, 255, 0}, 4);
                                }
                            }

                            if (mResults.mObjects[d].bHasMask)
                            {
                                m_drawer[i].add_mask(&mResults.mObjects[d].bbox, &mResults.mObjects[d].mYolov5Mask, {255, 0, 255, 0});
                            }
                        }
                        m_drawer[i].add_text("fps:" + std::to_string(mResults.niFps),
                                             {0, 0},
                                             {UCHAR_MAX, 0, 0, 0}, 1, 2);
                        auto &disps = m_drawer[i].get();
                        for (size_t d = 0; d < disps.size() && d < osd_pipe->m_ivps_attr.n_osd_rgn; d++)
                        {
                            int ret = AX_IVPS_RGN_Update(osd_pipe->m_ivps_attr.n_osd_rgn_chn[d], &disps[d]);
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