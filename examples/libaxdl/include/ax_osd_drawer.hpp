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

class ax_osd_drawer
{
private:
#ifdef AXERA_TARGET_CHIP_AX620
    std::vector<AX_IVPS_RGN_DISP_GROUP_S> vRgns;
#elif defined(AXERA_TARGET_CHIP_AX650)
    std::vector<AX_IVPS_RGN_DISP_GROUP_T> vRgns;
#endif

    SimpleRingBuffer<std::vector<unsigned char>> mRingBufferMatText, mRingBufferMatMask;
    int nWidth, nHeight;
    int index = -1;

    bool add_index()
    {
        index++;
        if (index >= vRgns.size() * AX_IVPS_REGION_MAX_DISP_NUM)
        {
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
        mRingBufferMatText.resize(num_rgn * SAMPLE_MAX_BBOX_COUNT * SAMPLE_RINGBUFFER_CACHE_COUNT);
        mRingBufferMatMask.resize(num_rgn * SAMPLE_MAX_BBOX_COUNT * SAMPLE_RINGBUFFER_CACHE_COUNT);
        vRgns.resize(num_rgn);
        nWidth = image_width;
        nHeight = image_height;
        reset();
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
        vRgns[get_cur_rgn_id()].arrDisp[get_cur_rgn_idx()].uDisp.tPolygon.nPointNum = std::min(num, 10);
        for (size_t i = 0; i < vRgns[get_cur_rgn_id()].arrDisp[get_cur_rgn_idx()].uDisp.tLine.nPointNum; i++)
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
        canvas_ptr.resize(4 * (label_size.height + baseLine) * label_size.width);
        auto canvas = cv::Mat(label_size.height + baseLine, label_size.width, CV_8UC4, canvas_ptr.data());
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
        vRgns[get_cur_rgn_id()].arrDisp[get_cur_rgn_idx()].uDisp.tLine.nPointNum = std::min(num, 10);
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
        mask_color_ptr.resize(4 * rect.height * rect.width);
        auto mask_color = cv::Mat(rect.height, rect.width, CV_8UC4, mask_color_ptr.data());
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
};
