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
    int ax_npu_csc(axdl_image_t *src, axdl_image_t *dst);
    int ax_npu_warp(axdl_image_t *src, axdl_image_t *dst, const float *pMat33, const int const_val);
    int ax_align_face(axdl_object_t *obj, axdl_image_t *src, axdl_image_t *dst);
#ifdef __cplusplus
}
#endif

#endif //_ax_sys_api_h_