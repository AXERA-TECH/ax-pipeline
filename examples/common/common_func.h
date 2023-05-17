#ifndef __COMMON_FUNC_H__
#define __COMMON_FUNC_H__

typedef enum
{
    SYS_CASE_NONE = -1,
    SYS_CASE_SINGLE_OS04A10 = 0,
    SYS_CASE_SINGLE_IMX334 = 1,
    SYS_CASE_SINGLE_GC4653 = 2,
    SYS_CASE_DUAL_OS04A10 = 3,
    SYS_CASE_SINGLE_OS08A20 = 4,
    SYS_CASE_SINGLE_OS04A10_ONLINE = 5,
    SYS_CASE_SINGLE_DVP = 6,
    SYS_CASE_SINGLE_BT601 = 7,
    SYS_CASE_SINGLE_BT656 = 8,
    SYS_CASE_SINGLE_BT1120 = 9,
    SYS_CASE_MIPI_YUV = 10,
    SYS_CASE_BUTT
} COMMON_SYS_CASE_E;

#if __cplusplus
extern "C"
{
#endif
#ifdef AXERA_TARGET_CHIP_AX620
#include "npu_cv_kit/ax_npu_imgproc.h"
#include "common_vin.h"
#include "common_cam.h"
#include "common_sys.h"
    int COMMON_SET_CAM(CAMERA_T Cams[MAX_CAMERAS], COMMON_SYS_CASE_E eSysCase,
                       AX_SNS_HDR_MODE_E eHdrMode, SAMPLE_SNS_TYPE_E *eSnsType, COMMON_SYS_ARGS_T *tCommonArgs, int s_sample_framerate);
#elif defined(AXERA_TARGET_CHIP_AX650)
#include "common_vin.h"
#include "common_cam.h"
#include "common_sys.h"
#endif
#if __cplusplus
}
#endif

#endif //__COMMON_FUNC_H__