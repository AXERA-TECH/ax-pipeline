#ifndef _ax_sys_api_h_
#define _ax_sys_api_h_

#include "c_api.h"

#ifdef __cplusplus
extern "C"
{
#endif
    int ax_sys_memalloc(unsigned long long int *phyaddr, void **pviraddr, unsigned int size, unsigned int align, const char *token);
    int ax_sys_memfree(unsigned long long int phyaddr, void *pviraddr);
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C"
{
#endif
    int ax_imgproc_csc(axdl_image_t *src, axdl_image_t *dst);
    int ax_imgproc_warp(axdl_image_t *src, axdl_image_t *dst, const float *pMat33, const int const_val);
    int ax_imgproc_crop_resize(axdl_image_t *src, axdl_image_t *dst, axdl_bbox_t *box);
    int ax_imgproc_crop_resize_keep_ratio(axdl_image_t *src, axdl_image_t *dst, axdl_bbox_t *box);
    int ax_imgproc_crop_resize_warp(axdl_image_t *src, axdl_image_t *dst, axdl_bbox_t *box, double *m, double *m_inv);
    int ax_imgproc_crop_resize_keep_ratio_warp(axdl_image_t *src, axdl_image_t *dst, axdl_bbox_t *box, double *m, double *m_inv);
    int ax_imgproc_align_face(axdl_object_t *obj, axdl_image_t *src, axdl_image_t *dst);
#ifdef __cplusplus
}
#endif

#include "mutex"
#include "string.h"
#include "sample_log.h"
class ax_imgproc_t
{
private:
    int target_width = {0};
    int target_height = {0};
    bool make_broder = {0};
    axdl_color_space_e target_color = axdl_color_space_unknown;

    axdl_image_t image_bgr = {0}, image_rgb = {0}, image_nv12 = {0}, image_nv21 = {0};

    std::mutex locker;

    static void init_image(int width, int height, axdl_color_space_e color, axdl_image_t &image)
    {
        image.eDtype = color;
        image.nWidth = width;
        image.nHeight = height;
        image.tStride_W = width;
        switch (color)
        {
        case axdl_color_space_nv12:
        case axdl_color_space_nv21:
            image.nSize = width * height * 1.5;
            break;
        case axdl_color_space_bgr:
        case axdl_color_space_rgb:
            image.nSize = width * height * 3;
            break;
        default:
            break;
        }
        ax_sys_memalloc(&image.pPhy, &image.pVir, image.nSize, 0x100, "imgproc");
    }

    static void release_image(axdl_image_t &image)
    {
        if (image.pVir)
        {
            ax_sys_memfree(image.pPhy, image.pVir);
        }
        memset(&image, 0, sizeof(image));
    }

    int resize(axdl_image_t *image, axdl_bbox_t *box)
    {
        axdl_image_t *target_image = nullptr;
        switch (image->eDtype)
        {
        case axdl_color_space_nv12:
            target_image = &image_nv12;
            break;
        case axdl_color_space_nv21:
            target_image = &image_nv21;
            break;
        case axdl_color_space_bgr:
            target_image = &image_bgr;
            break;
        case axdl_color_space_rgb:
            target_image = &image_rgb;
            break;
        default:
            return -1;
            break;
        }
        if (box)
        {
            if (make_broder)
            {
                return ax_imgproc_crop_resize_keep_ratio_warp(image, target_image, box, nullptr, nullptr);
            }
            else
            {
                return ax_imgproc_crop_resize_warp(image, target_image, box, nullptr, nullptr);
            }
        }
        else
        {
            if (make_broder)
            {
                return ax_imgproc_crop_resize_keep_ratio(image, target_image, box);
            }
            else
            {
                return ax_imgproc_crop_resize(image, target_image, box);
            }
        }
    }

    int csc(axdl_color_space_e src_color, axdl_color_space_e dst_color)
    {
        axdl_image_t *src = nullptr, *dst = nullptr;
        switch (src_color)
        {
        case axdl_color_space_nv12:
            src = &image_nv12;
            break;
        case axdl_color_space_nv21:
            src = &image_nv21;
            break;
        case axdl_color_space_bgr:
            src = &image_bgr;
            break;
        case axdl_color_space_rgb:
            src = &image_rgb;
            break;
        default:
            return -1;
            break;
        }

        switch (dst_color)
        {
        case axdl_color_space_nv12:
            dst = &image_nv12;
            break;
        case axdl_color_space_nv21:
            dst = &image_nv21;
            break;
        case axdl_color_space_bgr:
            dst = &image_bgr;
            break;
        case axdl_color_space_rgb:
            dst = &image_rgb;
            break;
        default:
            return -1;
            break;
        }

        if (src == nullptr || dst == nullptr)
        {
            return -1;
        }

        if (src != dst)
        {
            if (src_color == axdl_color_space_nv12 && dst_color == axdl_color_space_rgb)
            {
                ax_imgproc_csc(src, &image_bgr);
                return ax_imgproc_csc(&image_bgr, dst);
            }

            return ax_imgproc_csc(src, dst);
        }
        return 0;
    }

public:
    ax_imgproc_t(/* args */) {}
    ax_imgproc_t(int _target_width, int _target_height, bool _make_broder, axdl_color_space_e _target_color)
    {
        init(_target_width, _target_height, _make_broder, _target_color);
    }
    ~ax_imgproc_t() {}

    void init(int _target_width, int _target_height, bool _make_broder, axdl_color_space_e _target_color)
    {
        std::lock_guard<std::mutex> tmplocker(locker);
        target_width = _target_width;
        target_height = _target_height;
        make_broder = _make_broder;
        target_color = _target_color;

        init_image(target_width, target_height, axdl_color_space_bgr, image_bgr);
        init_image(target_width, target_height, axdl_color_space_rgb, image_rgb);
        init_image(target_width, target_height, axdl_color_space_nv12, image_nv12);
        init_image(target_width, target_height, axdl_color_space_nv21, image_nv21);
    }

    void deinit()
    {
        release_image(image_bgr);
        release_image(image_rgb);
        release_image(image_nv12);
        release_image(image_nv21);
    }

    int process(axdl_image_t *src, axdl_bbox_t *box, axdl_image_t *dst, axdl_color_space_e _target_color)
    {
        std::lock_guard<std::mutex> tmplocker(locker);
        int ret = resize(src, box);
        if (ret != 0)
        {
            ALOGE("resize failed");
            return ret;
        }
        ret = csc(src->eDtype, _target_color);
        if (ret != 0)
        {
            ALOGE("csc failed");
            return ret;
        }

        if (dst)
        {
            switch (_target_color)
            {
            case axdl_color_space_nv12:
                dst = &image_nv12;
                break;
            case axdl_color_space_nv21:
                dst = &image_nv21;
                break;
            case axdl_color_space_bgr:
                dst = &image_bgr;
                break;
            case axdl_color_space_rgb:
                dst = &image_rgb;
                break;
            default:
                return -1;
            }
        }

        return 0;
    }

    int process(axdl_image_t *src, axdl_bbox_t *box, axdl_image_t *dst)
    {
        return process(src, box, dst, target_color);
    }

    int process(axdl_image_t *src, axdl_bbox_t *box)
    {
        return process(src, box, nullptr, target_color);
    }

    int process(axdl_image_t *src, axdl_image_t *dst, axdl_color_space_e _target_color)
    {
        return process(src, nullptr, dst, _target_color);
    }

    int process(axdl_image_t *src, axdl_image_t *dst)
    {
        return process(src, nullptr, dst, target_color);
    }

    int process(axdl_image_t *src, axdl_color_space_e _target_color)
    {
        return process(src, nullptr, nullptr, _target_color);
    }

    int process(axdl_image_t *src)
    {
        return process(src, nullptr, nullptr, target_color);
    }

    axdl_image_t *get(axdl_color_space_e _target_color)
    {
        switch (_target_color)
        {
        case axdl_color_space_nv12:
            return &image_nv12;
            break;
        case axdl_color_space_nv21:
            return &image_nv21;
            break;
        case axdl_color_space_bgr:
            return &image_bgr;
            break;
        case axdl_color_space_rgb:
            return &image_rgb;
            break;
        default:
            return nullptr;
        }
    }
};

#endif //_ax_sys_api_h_