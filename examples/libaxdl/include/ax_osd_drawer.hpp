#pragma once
#include "mutex"
#include "vector"
#include "map"
#include "thread"
#include "string.h"
#include "unistd.h"

#include "c_api.h"
#include "../../utilities/sample_log.h"
#include "../../utilities/ringbuffer.hpp"

#include "ax_ivps_api.h"

#include "opencv2/opencv.hpp"

#define MAX_NUM_PTS 10

class ax_osd_drawer
{
private:
#ifdef AXERA_TARGET_CHIP_AX620
    std::vector<AX_IVPS_RGN_DISP_GROUP_S> vRgns;
#elif defined(AXERA_TARGET_CHIP_AX650)
    std::vector<AX_IVPS_RGN_DISP_GROUP_T> vRgns;
#endif

    SimpleRingBuffer<axdl_mat_t> mRingBufferMatText, mRingBufferMatMask, mRingBufferMatMaskV2;
    int nWidth, nHeight;
    int index = -1;

    bool add_index()
    {
        int limit = vRgns.size() * AX_IVPS_REGION_MAX_DISP_NUM;
        // printf("%d,%d,%d\n", index, vRgns.size(), limit);
        index++;
        if (index >= limit)
        {
            printf("limit %d,%d,%d\n", index, vRgns.size(), limit);
            return false;
        }
        return true;
    }

    int get_cur_rgn_id()
    {
        return int(index / AX_IVPS_REGION_MAX_DISP_NUM);
    }

    int get_cur_rgn_idx()
    {
        return index % AX_IVPS_REGION_MAX_DISP_NUM;
    }

public:
    union ax_abgr_t
    {
        unsigned char abgr[4];
        int iargb;

        ax_abgr_t()
        {
            iargb = 0;
        }

        ax_abgr_t(unsigned char a, unsigned char b, unsigned char g, unsigned char r)
        {
            abgr[0] = a;
            abgr[1] = b;
            abgr[2] = g;
            abgr[3] = r;
        }
    };

    ax_osd_drawer(/* args */) {}
    ~ax_osd_drawer() {}

    void init(int num_rgn, int image_width, int image_height)
    {
        mRingBufferMatText.resize(SAMPLE_MAX_BBOX_COUNT * SAMPLE_RINGBUFFER_CACHE_COUNT);
        mRingBufferMatMask.resize(SAMPLE_MAX_MASK_OBJ_COUNT * SAMPLE_RINGBUFFER_CACHE_COUNT);
        mRingBufferMatMaskV2.resize(SAMPLE_RINGBUFFER_CACHE_COUNT);
        memset(mRingBufferMatText.data(), 0, mRingBufferMatText.size() * sizeof(axdl_mat_t));
        memset(mRingBufferMatMask.data(), 0, mRingBufferMatMask.size() * sizeof(axdl_mat_t));
        memset(mRingBufferMatMaskV2.data(), 0, mRingBufferMatMaskV2.size() * sizeof(axdl_mat_t));
        vRgns.resize(num_rgn);
        nWidth = image_width;
        nHeight = image_height;
        reset();
    }

    void resize(int text_cnt, int mask_cnt, int maskv2_cnt)
    {
        if (text_cnt > 0)
        {
            mRingBufferMatText.resize(text_cnt);
        }
        if (mask_cnt > 0)
        {
            mRingBufferMatMask.resize(mask_cnt);
        }
        if (maskv2_cnt > 0)
        {
            mRingBufferMatMaskV2.resize(maskv2_cnt);
        }
    }

    void reset()
    {
        index = -1;
#ifdef AXERA_TARGET_CHIP_AX620
        memset(vRgns.data(), 0, vRgns.size() * sizeof(AX_IVPS_RGN_DISP_GROUP_S));
#elif defined(AXERA_TARGET_CHIP_AX650)
        memset(vRgns.data(), 0, vRgns.size() * sizeof(AX_IVPS_RGN_DISP_GROUP_T));
#endif
    }

    int get_width()
    {
        return nWidth;
    }

    int get_height()
    {
        return nHeight;
    }
#ifdef AXERA_TARGET_CHIP_AX620
    std::vector<AX_IVPS_RGN_DISP_GROUP_S> &get()
#elif defined(AXERA_TARGET_CHIP_AX650)
    std::vector<AX_IVPS_RGN_DISP_GROUP_T> &get()
#endif
    {
        for (size_t i = 0; i < vRgns.size(); i++)
        {
            vRgns[i].tChnAttr.nAlpha = 255;
            vRgns[i].tChnAttr.eFormat = AX_FORMAT_RGBA8888;
            vRgns[i].tChnAttr.nZindex = i;
            vRgns[i].tChnAttr.nBitColor.nColor = 0xFF0000;
            vRgns[i].tChnAttr.nBitColor.nColorInv = 0xFF;
            vRgns[i].tChnAttr.nBitColor.nColorInvThr = 0xA0A0A0;
            if (i < get_cur_rgn_id())
            {
                vRgns[i].nNum = AX_IVPS_REGION_MAX_DISP_NUM;
            }
        }
        vRgns[get_cur_rgn_id()].nNum = get_cur_rgn_idx() + 1;

        return vRgns;
    }

    void add_rect(axdl_bbox_t *box, ax_abgr_t color, int linewidth)
    {
        if (!add_index())
        {
            return;
        }
        vRgns[get_cur_rgn_id()].arrDisp[get_cur_rgn_idx()].bShow = AX_TRUE;
        vRgns[get_cur_rgn_id()].arrDisp[get_cur_rgn_idx()].eType = AX_IVPS_RGN_TYPE_RECT;
        vRgns[get_cur_rgn_id()].arrDisp[get_cur_rgn_idx()].uDisp.tPolygon.nAlpha = 255;
        vRgns[get_cur_rgn_id()].arrDisp[get_cur_rgn_idx()].uDisp.tPolygon.nColor = color.iargb;
        vRgns[get_cur_rgn_id()].arrDisp[get_cur_rgn_idx()].uDisp.tPolygon.nLineWidth = linewidth;
        vRgns[get_cur_rgn_id()].arrDisp[get_cur_rgn_idx()].uDisp.tPolygon.tRect.nX = box->x * nWidth;
        vRgns[get_cur_rgn_id()].arrDisp[get_cur_rgn_idx()].uDisp.tPolygon.tRect.nY = box->y * nHeight;
        vRgns[get_cur_rgn_id()].arrDisp[get_cur_rgn_idx()].uDisp.tPolygon.tRect.nW = box->w * nWidth;
        vRgns[get_cur_rgn_id()].arrDisp[get_cur_rgn_idx()].uDisp.tPolygon.tRect.nH = box->h * nHeight;
    }

    void add_polygon(axdl_point_t *pts, int num, ax_abgr_t color, int linewidth)
    {
        if (!add_index())
        {
            return;
        }
        vRgns[get_cur_rgn_id()].arrDisp[get_cur_rgn_idx()].bShow = AX_TRUE;
        vRgns[get_cur_rgn_id()].arrDisp[get_cur_rgn_idx()].eType = AX_IVPS_RGN_TYPE_POLYGON;
        vRgns[get_cur_rgn_id()].arrDisp[get_cur_rgn_idx()].uDisp.tPolygon.nAlpha = 255;
        vRgns[get_cur_rgn_id()].arrDisp[get_cur_rgn_idx()].uDisp.tPolygon.nColor = color.iargb;
        vRgns[get_cur_rgn_id()].arrDisp[get_cur_rgn_idx()].uDisp.tPolygon.nLineWidth = linewidth;
        vRgns[get_cur_rgn_id()].arrDisp[get_cur_rgn_idx()].uDisp.tPolygon.nPointNum = std::min(num, MAX_NUM_PTS);
        for (size_t i = 0; i < vRgns[get_cur_rgn_id()].arrDisp[get_cur_rgn_idx()].uDisp.tPolygon.nPointNum; i++)
        {
            vRgns[get_cur_rgn_id()].arrDisp[get_cur_rgn_idx()].uDisp.tPolygon.tPTs[i].nX = pts[i].x * nWidth;
            vRgns[get_cur_rgn_id()].arrDisp[get_cur_rgn_idx()].uDisp.tPolygon.tPTs[i].nY = pts[i].y * nHeight;
        }
    }

    void add_point(axdl_point_t *pos, ax_abgr_t color, int linewidth)
    {
        if (!add_index())
        {
            return;
        }
        vRgns[get_cur_rgn_id()].arrDisp[get_cur_rgn_idx()].bShow = AX_TRUE;
        vRgns[get_cur_rgn_id()].arrDisp[get_cur_rgn_idx()].eType = AX_IVPS_RGN_TYPE_RECT;
        vRgns[get_cur_rgn_id()].arrDisp[get_cur_rgn_idx()].uDisp.tPolygon.nAlpha = 255;
        vRgns[get_cur_rgn_id()].arrDisp[get_cur_rgn_idx()].uDisp.tPolygon.nColor = color.iargb;
        vRgns[get_cur_rgn_id()].arrDisp[get_cur_rgn_idx()].uDisp.tPolygon.nLineWidth = linewidth;
        vRgns[get_cur_rgn_id()].arrDisp[get_cur_rgn_idx()].uDisp.tPolygon.tRect.nX = pos->x * nWidth - std::max(1, linewidth / 2);
        vRgns[get_cur_rgn_id()].arrDisp[get_cur_rgn_idx()].uDisp.tPolygon.tRect.nY = pos->y * nHeight - std::max(1, linewidth / 2);
        vRgns[get_cur_rgn_id()].arrDisp[get_cur_rgn_idx()].uDisp.tPolygon.tRect.nW = linewidth;
        vRgns[get_cur_rgn_id()].arrDisp[get_cur_rgn_idx()].uDisp.tPolygon.tRect.nH = linewidth;
    }

    void add_text(std::string text, axdl_point_t pos, ax_abgr_t color, float fontsize, int linewidth)
    {
        if (!add_index())
        {
            return;
        }
        int baseLine = 0;
        auto label_size = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, fontsize, linewidth, &baseLine);

        auto &canvas_ptr = mRingBufferMatText.next();
        if (canvas_ptr.w < 4 * (label_size.height + baseLine) * label_size.width)
        {
            if (canvas_ptr.data)
            {
                delete[] canvas_ptr.data;
                canvas_ptr.data = nullptr;
            }
            canvas_ptr.w = 4 * (label_size.height + baseLine) * label_size.width;
            canvas_ptr.data = new unsigned char[canvas_ptr.w];
        }

        auto canvas = cv::Mat(label_size.height + baseLine, label_size.width, CV_8UC4, canvas_ptr.data);
        memset(canvas.data, 255, canvas.cols * canvas.rows * 4);
        cv::putText(canvas, text, cv::Point(0, label_size.height), cv::FONT_HERSHEY_SIMPLEX, fontsize,
                    cv::Scalar(color.abgr[0], color.abgr[1], color.abgr[2], color.abgr[3]), linewidth);

        vRgns[get_cur_rgn_id()].arrDisp[get_cur_rgn_idx()].bShow = AX_TRUE;
        vRgns[get_cur_rgn_id()].arrDisp[get_cur_rgn_idx()].eType = AX_IVPS_RGN_TYPE_OSD;
        vRgns[get_cur_rgn_id()].arrDisp[get_cur_rgn_idx()].uDisp.tOSD.enRgbFormat = AX_FORMAT_RGBA8888;
        vRgns[get_cur_rgn_id()].arrDisp[get_cur_rgn_idx()].uDisp.tOSD.u32BmpWidth = canvas.cols;
        vRgns[get_cur_rgn_id()].arrDisp[get_cur_rgn_idx()].uDisp.tOSD.u32BmpHeight = canvas.rows;
        vRgns[get_cur_rgn_id()].arrDisp[get_cur_rgn_idx()].uDisp.tOSD.u32DstXoffset = MAX(0, pos.x * nWidth);
        vRgns[get_cur_rgn_id()].arrDisp[get_cur_rgn_idx()].uDisp.tOSD.u32DstYoffset = MAX(0, pos.y * nHeight - canvas.rows);
        vRgns[get_cur_rgn_id()].arrDisp[get_cur_rgn_idx()].uDisp.tOSD.pBitmap = canvas.data;
    }

    void add_line(axdl_point_t *pts, int num, ax_abgr_t color, int linewidth)
    {
        if (!add_index())
        {
            return;
        }
        vRgns[get_cur_rgn_id()].arrDisp[get_cur_rgn_idx()].bShow = AX_TRUE;
        vRgns[get_cur_rgn_id()].arrDisp[get_cur_rgn_idx()].eType = AX_IVPS_RGN_TYPE_LINE;
        vRgns[get_cur_rgn_id()].arrDisp[get_cur_rgn_idx()].uDisp.tLine.nAlpha = 255;
        vRgns[get_cur_rgn_id()].arrDisp[get_cur_rgn_idx()].uDisp.tLine.nColor = color.iargb;
        vRgns[get_cur_rgn_id()].arrDisp[get_cur_rgn_idx()].uDisp.tLine.nLineWidth = linewidth;
        vRgns[get_cur_rgn_id()].arrDisp[get_cur_rgn_idx()].uDisp.tLine.nPointNum = std::min(num, MAX_NUM_PTS);
        for (size_t i = 0; i < vRgns[get_cur_rgn_id()].arrDisp[get_cur_rgn_idx()].uDisp.tLine.nPointNum; i++)
        {
            vRgns[get_cur_rgn_id()].arrDisp[get_cur_rgn_idx()].uDisp.tLine.tPTs[i].nX = pts[i].x * nWidth;
            vRgns[get_cur_rgn_id()].arrDisp[get_cur_rgn_idx()].uDisp.tLine.tPTs[i].nY = pts[i].y * nHeight;
        }
    }

    void add_mask(axdl_bbox_t *box, axdl_mat_t *mask, ax_abgr_t color)
    {
        if (!add_index())
        {
            return;
        }
        cv::Rect rect(0,
                      0,
                      box ? box->w * nWidth
                          : nWidth,
                      box ? box->h * nHeight
                          : nHeight);
        if (rect.width <= 0 || rect.height <= 0)
        {
            printf("%d %d  %d %d\n", rect.width, rect.height, mask->w, mask->h);
            return;
        }

        cv::Mat mask_mat(mask->h, mask->w, CV_8U, mask->data);
        cv::Mat mask_target;
        auto &mask_color_ptr = mRingBufferMatMask.next();
        if (mask_color_ptr.w < 4 * rect.height * rect.width)
        {
            if (mask_color_ptr.data)
            {
                delete[] mask_color_ptr.data;
                mask_color_ptr.data = nullptr;
            }
            mask_color_ptr.w = 4 * rect.height * rect.width;
            mask_color_ptr.data = new unsigned char[mask_color_ptr.w];
        }

        auto mask_color = cv::Mat(rect.height, rect.width, CV_8UC4, mask_color_ptr.data);
        memset(mask_color.data, 0, mask_color.cols * mask_color.rows * 4);

        cv::resize(mask_mat, mask_target, cv::Size(rect.width, rect.height), 0, 0, cv::INTER_NEAREST);
        mask_color(rect).setTo(cv::Scalar(color.abgr[0], color.abgr[1], color.abgr[2], color.abgr[3]), mask_target);

        vRgns[get_cur_rgn_id()].arrDisp[get_cur_rgn_idx()].bShow = AX_TRUE;
        vRgns[get_cur_rgn_id()].arrDisp[get_cur_rgn_idx()].eType = AX_IVPS_RGN_TYPE_OSD;
        vRgns[get_cur_rgn_id()].arrDisp[get_cur_rgn_idx()].uDisp.tOSD.enRgbFormat = AX_FORMAT_RGBA8888;
        vRgns[get_cur_rgn_id()].arrDisp[get_cur_rgn_idx()].uDisp.tOSD.u32BmpWidth = mask_color.cols;
        vRgns[get_cur_rgn_id()].arrDisp[get_cur_rgn_idx()].uDisp.tOSD.u32BmpHeight = mask_color.rows;
        vRgns[get_cur_rgn_id()].arrDisp[get_cur_rgn_idx()].uDisp.tOSD.u32DstXoffset = box ? MAX(0, box->x * nWidth) : 0;
        vRgns[get_cur_rgn_id()].arrDisp[get_cur_rgn_idx()].uDisp.tOSD.u32DstYoffset = box ? MAX(0, box->y * nHeight) : 0;
        vRgns[get_cur_rgn_id()].arrDisp[get_cur_rgn_idx()].uDisp.tOSD.pBitmap = mask_color.data;
    }

    void add_mask(axdl_bbox_t *box, axdl_mat_t *mask)
    {
        if (!add_index())
        {
            return;
        }
        cv::Rect rect(0,
                      0,
                      box ? box->w * nWidth
                          : nWidth,
                      box ? box->h * nHeight
                          : nHeight);
        if (rect.width <= 0 || rect.height <= 0)
        {
            printf("%d %d  %d %d\n", rect.width, rect.height, mask->w, mask->h);
            return;
        }

        cv::Mat mask_mat(mask->h, mask->w, CV_8UC4, mask->data);

        auto &mask_color_ptr = mRingBufferMatMaskV2.next();
        if (mask_color_ptr.w < 4 * rect.height * rect.width)
        {
            if (mask_color_ptr.data)
            {
                delete[] mask_color_ptr.data;
                mask_color_ptr.data = nullptr;
            }
            mask_color_ptr.w = 4 * rect.height * rect.width;
            mask_color_ptr.data = new unsigned char[mask_color_ptr.w];
        }

        auto mask_color = cv::Mat(rect.height, rect.width, CV_8UC4, mask_color_ptr.data);

        cv::resize(mask_mat, mask_color, cv::Size(rect.width, rect.height));

        vRgns[get_cur_rgn_id()].arrDisp[get_cur_rgn_idx()].bShow = AX_TRUE;
        vRgns[get_cur_rgn_id()].arrDisp[get_cur_rgn_idx()].eType = AX_IVPS_RGN_TYPE_OSD;
        vRgns[get_cur_rgn_id()].arrDisp[get_cur_rgn_idx()].uDisp.tOSD.enRgbFormat = AX_FORMAT_RGBA8888;
        vRgns[get_cur_rgn_id()].arrDisp[get_cur_rgn_idx()].uDisp.tOSD.u32BmpWidth = mask_color.cols;
        vRgns[get_cur_rgn_id()].arrDisp[get_cur_rgn_idx()].uDisp.tOSD.u32BmpHeight = mask_color.rows;
        vRgns[get_cur_rgn_id()].arrDisp[get_cur_rgn_idx()].uDisp.tOSD.u32DstXoffset = 0;
        vRgns[get_cur_rgn_id()].arrDisp[get_cur_rgn_idx()].uDisp.tOSD.u32DstYoffset = 0;
        vRgns[get_cur_rgn_id()].arrDisp[get_cur_rgn_idx()].uDisp.tOSD.pBitmap = mask_color.data;
    }
};
