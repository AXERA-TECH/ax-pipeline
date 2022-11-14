#ifndef _SAMPLE_DEF_H_
#define _SAMPLE_DEF_H_
#include "ax_sys_api.h"
#include "ax_ivps_api.h"

#include "common_cam.h"
#include "common_sys.h"
#include "common_codec/common_venc.h"

#include "../sample_run_joint/sample_run_joint.h"
#include "../rtsp/inc/rtsp.h"
#define RED 0xFF0000
#define PINK 0xFFC0CB
#define GREEN 0x00FF00
#define BLUE 0x0000FF
#define PURPLE 0xA020F0
#define ORANGE 0xFFA500
#define YELLOW 0xFFFF00

#ifndef ALIGN_UP
#define ALIGN_UP(x, align) ((((x) + ((align)-1)) / (align)) * (align))
#endif

#ifndef ALIGN_DOWN
#define ALIGN_DOWN(x, align) (((x) / (align)) * (align))
#endif

typedef AX_S32 IVPS_GRP;
typedef AX_S32 AX_IVPS_FILTER;

typedef enum
{
    SYS_CASE_NONE = -1,
    SYS_CASE_SINGLE_OS04A10 = 0,
    SYS_CASE_SINGLE_IMX334 = 1,
    SYS_CASE_SINGLE_GC4653 = 2,
    SYS_CASE_SINGLE_OS08A20 = 3,
    SYS_CASE_SINGLE_OS04A10_ONLINE = 4,
    SYS_CASE_BUTT
} COMMON_SYS_CASE_E;

typedef enum
{
    VENC_CASE_NONE = -1,
    VENC_CASE_H264 = 0,
    VENC_CASE_H265 = 1,
    VENC_CASE_BUTT
} COMMON_VENC_CASE_E;

typedef struct _IVPS_REGION_PARAM
{
    IVPS_RGN_HANDLE hChnRgn;
    IVPS_GRP nGroupIdx;
    AX_IVPS_FILTER nFilter;
    AX_BOOL bExit;
} IVPS_REGION_PARAM_T, *IVPS_REGION_PARAM_PTR;

typedef struct
{
    IVPS_GRP nGroup;
    AX_IVPS_FILTER nFilter;
    AX_U32 nChnWidth;
    AX_U32 nChnHeight;
    AX_U32 nRgnNum;
    AX_IVPS_RGN_LAYER_E eLayer;
} RGN_GROUP_CFG_T;

typedef struct _stVencGetStreamParam
{
    AX_S32 VeChn;
    AX_BOOL bThreadStart;
    AX_PAYLOAD_TYPE_E ePayloadType;
    rtsp_session_handle rSessionHandle;
} VENC_GETSTREAM_PARAM_T;

typedef struct _stRCInfo
{
    SAMPLE_VENC_RC_E eRCType;
    AX_U32 nMinQp;
    AX_U32 nMaxQp;
    AX_U32 nMinIQp;
    AX_U32 nMaxIQp;
    AX_S32 nIntraQpDelta;
} RC_INFO_T;

typedef struct _stVideoConfig
{
    AX_PAYLOAD_TYPE_E ePayloadType;
    AX_U32 nGOP;
    AX_U32 nSrcFrameRate;
    AX_U32 nDstFrameRate;
    AX_U32 nStride;
    AX_S32 nInWidth;
    AX_S32 nInHeight;
    AX_S32 nOutWidth;
    AX_S32 nOutHeight;
    AX_S32 nOffsetCropX;
    AX_S32 nOffsetCropY;
    AX_S32 nOffsetCropW;
    AX_S32 nOffsetCropH;
    AX_IMG_FORMAT_E eImgFormat;
    RC_INFO_T stRCInfo;
    AX_S32 nBitrate;
} VIDEO_CONFIG_T;

typedef struct axSAMPLE_VO_CHN_THREAD_PARAM
{
    pthread_t ThreadID;

    AX_U32 u32ThreadForceStop;

    AX_U32 u32LayerID;
    AX_U32 u32ChnID;
    AX_POOL u32UserPoolId;
} SAMPLE_VO_CHN_THREAD_PARAM_S;

extern AX_S32 s_sample_framerate; //25fps

extern volatile AX_S32 gLoopExit;

extern pthread_mutex_t g_result_mutex;
extern sample_run_joint_results g_result_disp;
extern void *gJointHandle;

extern CAMERA_T gCams[MAX_CAMERAS];

extern int SAMPLE_ALGO_WIDTH;  // 640
extern int SAMPLE_ALGO_HEIGHT; // 640
extern int SAMPLE_ALGO_FORMAT;
extern int SAMPLE_MAJOR_STREAM_WIDTH;
extern int SAMPLE_MAJOR_STREAM_HEIGHT;

#define RTSP_PORT 8554
#define RTSP_END_POINT "axstream"

#endif