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

#include "../libaxdl/include/c_api.h"
#include "../libaxdl/include/ax_osd_helper.hpp"
#include "../common/common_func.h"
#include "common_pipeline.h"

#include "../utilities/sample_log.h"

#include "ax_ivps_api.h"

#include "fstream"
#include <getopt.h>
#include "unistd.h"
#include "stdlib.h"
#include "string.h"
#include "signal.h"
#include "vector"
#include "map"

#define pipe_count 2

AX_S32 s_sample_framerate = 25;

volatile AX_S32 gLoopExit = 0;

int SAMPLE_MAJOR_STREAM_WIDTH = 1920;
int SAMPLE_MAJOR_STREAM_HEIGHT = 1080;

int SAMPLE_IVPS_ALGO_WIDTH = 960;
int SAMPLE_IVPS_ALGO_HEIGHT = 540;

static struct _g_sample_
{
    int bRunJoint;
    void *gModels;
    ax_osd_helper osd_helper;
    std::vector<pipeline_t *> pipes_need_osd;
    void Init()
    {
        bRunJoint = 0;
        gModels = nullptr;
        ALOGN("g_sample Init\n");
    }
    void Deinit()
    {
        pipes_need_osd.clear();
        
        ALOGN("g_sample Deinit\n");
    }
} g_sample;



void ai_inference_func(pipeline_buffer_t *buff)
{
    if (g_sample.bRunJoint)
    {
        static axdl_results_t mResults;
        axdl_image_t tSrcFrame = {0};
        switch (buff->d_type)
        {
        case po_buff_nv12:
            tSrcFrame.eDtype = axdl_color_space_nv12;
            break;
        case po_buff_bgr:
            tSrcFrame.eDtype = axdl_color_space_bgr;
            break;
        case po_buff_rgb:
            tSrcFrame.eDtype = axdl_color_space_rgb;
            break;
        default:
            break;
        }
        tSrcFrame.nWidth = buff->n_width;
        tSrcFrame.nHeight = buff->n_height;
        tSrcFrame.pVir = (unsigned char *)buff->p_vir;
        tSrcFrame.pPhy = buff->p_phy;
        tSrcFrame.tStride_W = buff->n_stride;
        tSrcFrame.nSize = buff->n_size;

        axdl_inference(g_sample.gModels, &tSrcFrame, &mResults);

g_sample.osd_helper.Update(&mResults);
    }
}

#define NAL_CODED_SLICE_CRA 21
#define NAL_CODED_SLICE_IDR 5

typedef struct _SAMPLE_BSPARSER
{
    FILE *fInput;
    AX_S32 sSize;
} SAMPLE_BSPARSER_T;

typedef enum _SAMPLE_BSBOUNDAR_YTYPE
{
    BSPARSER_NO_BOUNDARY = 0,
    BSPARSER_BOUNDARY = 1,
    BSPARSER_BOUNDARY_NON_SLICE_NAL = 2
} SAMPLE_BSBOUNDAR_YTYPE_E;

static AX_S32 FindNextStartCode(SAMPLE_BSPARSER_T *tBsInfo, AX_U32 *uZeroCount)
{
    AX_S32 i;
    AX_S32 sStart = ftello(tBsInfo->fInput);
    *uZeroCount = 0;

    /* Scan for the beginning of the packet. */
    for (i = 0; i < tBsInfo->sSize && i < tBsInfo->sSize - sStart; i++)
    {
        AX_U8 byte;
        AX_S32 ret_val = fgetc(tBsInfo->fInput);
        if (ret_val == EOF)
            return ftello(tBsInfo->fInput);
        byte = (unsigned char)ret_val;
        switch (byte)
        {
        case 0:
            *uZeroCount = *uZeroCount + 1;
            break;
        case 1:
            /* If there's more than three leading zeros, consider only three
             * of them to be part of this packet and the rest to be part of
             * the previous packet. */
            if (*uZeroCount > 3)
                *uZeroCount = 3;
            if (*uZeroCount >= 2)
            {
                return ftello(tBsInfo->fInput) - *uZeroCount - 1;
            }
            *uZeroCount = 0;
            break;
        default:
            *uZeroCount = 0;
            break;
        }
    }
    return ftello(tBsInfo->fInput);
}

AX_U32 CheckAccessUnitBoundaryH264(FILE *fInput, AX_S32 sNalBegin)
{
    AX_U32 uBoundary = BSPARSER_NO_BOUNDARY;
    AX_U32 uNalType, uVal;

    AX_S32 sStart = ftello(fInput);

    fseeko(fInput, sNalBegin, SEEK_SET);
    uNalType = (getc(fInput) & 0x1F);

    if (uNalType > NAL_CODED_SLICE_IDR)
        uBoundary = BSPARSER_BOUNDARY_NON_SLICE_NAL;
    else
    {
        uVal = getc(fInput);
        /* Check if first mb in slice is 0(ue(v)). */
        if (uVal & 0x80)
            uBoundary = BSPARSER_BOUNDARY;
    }

    fseeko(fInput, sStart, SEEK_SET);
    return uBoundary;
}

AX_S32 StreamParserReadFrameH264(SAMPLE_BSPARSER_T *tBsInfo, AX_U8 *sBuffer,
                                 AX_S32 *sSize)
{
    AX_S32 sBegin, sEnd, sStrmLen;
    AX_U32 sReadLen;
    AX_U32 uZeroCount = 0;

    AX_U32 uTmp = 0;
    AX_S32 sNalBegin;
    /* TODO(min): to extract exact one frame instead of a NALU */

    sBegin = FindNextStartCode(tBsInfo, &uZeroCount);
    sNalBegin = sBegin + uZeroCount + 1;
    uTmp = CheckAccessUnitBoundaryH264(tBsInfo->fInput, sNalBegin);
    sEnd = sNalBegin = FindNextStartCode(tBsInfo, &uZeroCount);

    if (sEnd != sBegin && uTmp != BSPARSER_BOUNDARY_NON_SLICE_NAL)
    {
        do
        {
            sEnd = sNalBegin;
            sNalBegin += uZeroCount + 1;

            /* Check access unit boundary for next NAL */
            uTmp = CheckAccessUnitBoundaryH264(tBsInfo->fInput, sNalBegin);
            if (uTmp == BSPARSER_NO_BOUNDARY)
            {
                sNalBegin = FindNextStartCode(tBsInfo, &uZeroCount);
            }
            else if (uTmp == BSPARSER_BOUNDARY_NON_SLICE_NAL)
            {
                do
                {
                    sNalBegin = FindNextStartCode(tBsInfo, &uZeroCount);
                    if (sEnd == sNalBegin)
                        break;
                    sEnd = sNalBegin;
                    sNalBegin += uZeroCount + 1;
                    uTmp = CheckAccessUnitBoundaryH264(tBsInfo->fInput, sNalBegin);
                } while (uTmp == BSPARSER_BOUNDARY_NON_SLICE_NAL);

                if (sEnd == sNalBegin)
                {
                    break;
                }
                else if (uTmp == BSPARSER_NO_BOUNDARY)
                {
                    sNalBegin = FindNextStartCode(tBsInfo, &uZeroCount);
                }
            }
        } while (uTmp != BSPARSER_BOUNDARY);
    }

    if (sEnd == sBegin)
    {
        return 0; /* End of stream */
    }
    fseeko(tBsInfo->fInput, sBegin, SEEK_SET);
    if (*sSize < sEnd - sBegin)
    {
        *sSize = sEnd - sBegin;
        return 0; /* Insufficient buffer size */
    }

    sStrmLen = sEnd - sBegin;
    sReadLen = fread(sBuffer, 1, sStrmLen, tBsInfo->fInput);

    return sReadLen;
}

// 允许外部调用
extern "C" AX_VOID __sigExit(int iSigNo)
{
    // ALOGN("Catch signal %d!\n", iSigNo);
    gLoopExit = 1;
    sleep(1);
    return;
}

static AX_VOID PrintHelp(char *testApp)
{
    printf("Usage:%s -h for help\n\n", testApp);
    printf("\t-p: model config file path\n");

    printf("\t-f: h264 file\n");

    printf("\t-r: Sensor&Video Framerate (framerate need supported by sensor), default is 25\n");

    exit(0);
}

int main(int argc, char *argv[])
{
    optind = 0;
    gLoopExit = 0;
    g_sample.Init();

    AX_S32 isExit = 0, ch;
    AX_S32 s32Ret = 0;
    COMMON_SYS_ARGS_T tCommonArgs = {0};
    char h26xfile[512];
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, __sigExit);
    char config_file[256];

    ALOGN("sample begin\n\n");

    while ((ch = getopt(argc, argv, "p:f:r:h")) != -1)
    {
        switch (ch)
        {
        case 'f':
            strcpy(h26xfile, optarg);
            ALOGI("file input %s", h26xfile);
            break;
        case 'p':
        {
            strcpy(config_file, optarg);
            break;
        }
        case 'r':
            s_sample_framerate = (AX_S32)atoi(optarg);
            if (s_sample_framerate <= 0)
            {
                s_sample_framerate = 30;
            }
            break;
        case 'h':
            isExit = 1;
            break;
        default:
            isExit = 1;
            break;
        }
    }

    if (isExit)
    {
        PrintHelp(argv[0]);
        exit(0);
    }

#ifdef AXERA_TARGET_CHIP_AX620
    COMMON_SYS_POOL_CFG_T poolcfg[] = {
        {1920, 1088, 1920, AX_YUV420_SEMIPLANAR, 10},
    };
#elif defined(AXERA_TARGET_CHIP_AX650)
    COMMON_SYS_POOL_CFG_T poolcfg[] = {
        {1920, 1088, 1920, AX_FORMAT_YUV420_SEMIPLANAR, 20},
    };
#endif
    tCommonArgs.nPoolCfgCnt = 1;
    tCommonArgs.pPoolCfg = poolcfg;
    /*step 1:sys init*/
    s32Ret = COMMON_SYS_Init(&tCommonArgs);
    if (s32Ret)
    {
        ALOGE("COMMON_SYS_Init failed,s32Ret:0x%x\n", s32Ret);
        return -1;
    }

    /*step 3:npu init*/
#ifdef AXERA_TARGET_CHIP_AX620
    AX_NPU_SDK_EX_ATTR_T sNpuAttr;
    sNpuAttr.eHardMode = AX_NPU_VIRTUAL_1_1;
    s32Ret = AX_NPU_SDK_EX_Init_with_attr(&sNpuAttr);
    if (0 != s32Ret)
    {
        ALOGE("AX_NPU_SDK_EX_Init_with_attr failed,s32Ret:0x%x\n", s32Ret);
        goto EXIT_2;
    }
#endif

    s32Ret = axdl_parse_param_init(config_file, &g_sample.gModels);
    if (s32Ret != 0)
    {
        ALOGE("sample_parse_param_det failed,run joint skip");
        g_sample.bRunJoint = 0;
    }
    else
    {
        s32Ret = axdl_get_ivps_width_height(g_sample.gModels, config_file, &SAMPLE_IVPS_ALGO_WIDTH, &SAMPLE_IVPS_ALGO_HEIGHT);
        ALOGI("IVPS AI channel width=%d height=%d", SAMPLE_IVPS_ALGO_WIDTH, SAMPLE_IVPS_ALGO_HEIGHT);
        g_sample.bRunJoint = 1;
    }

    pipeline_t pipelines[pipe_count];
    memset(&pipelines[0], 0, sizeof(pipelines));
    // 创建pipeline
    {

        pipeline_t &pipe0 = pipelines[0];
        {
            pipeline_ivps_config_t &config0 = pipe0.m_ivps_attr;
            config0.n_ivps_grp = 0;    // 重复的会创建失败
            config0.n_ivps_fps = 60;   // 屏幕只能是60gps
            config0.n_ivps_rotate = 1; // 旋转
            config0.n_ivps_width = 854;
            config0.n_ivps_height = 480;
            config0.n_osd_rgn = 4; // osd rgn 的个数，一个rgn可以osd 32个目标
        }
        pipe0.enable = 1;
        pipe0.pipeid = 0x90015;
        pipe0.m_input_type = pi_vdec_h264;
        pipe0.m_output_type = po_vo_sipeed_maix3_screen;
        pipe0.n_loog_exit = 0;            // 可以用来控制线程退出（如果有的话）
        pipe0.m_vdec_attr.n_vdec_grp = 0; // 可以重复

        pipeline_t &pipe1 = pipelines[1];
        {
            pipeline_ivps_config_t &config1 = pipe1.m_ivps_attr;
            config1.n_ivps_grp = 1; // 重复的会创建失败
            config1.n_ivps_fps = 60;
            config1.n_ivps_width = SAMPLE_IVPS_ALGO_WIDTH;
            config1.n_ivps_height = SAMPLE_IVPS_ALGO_HEIGHT;
            if (axdl_get_model_type(g_sample.gModels) != MT_SEG_PPHUMSEG && axdl_get_model_type(g_sample.gModels) != MT_SEG_DINOV2)
            {
                config1.b_letterbox = 1;
            }
            config1.n_fifo_count = 1; // 如果想要拿到数据并输出到回调 就设为1~4
        }
        pipe1.enable = g_sample.bRunJoint;
        pipe1.pipeid = 0x90016;
        pipe1.m_input_type = pi_vdec_h264;
        if (g_sample.gModels && g_sample.bRunJoint)
        {
            switch (axdl_get_color_space(g_sample.gModels))
            {
            case axdl_color_space_rgb:
                pipe1.m_output_type = po_buff_rgb;
                break;
            case axdl_color_space_bgr:
                pipe1.m_output_type = po_buff_bgr;
                break;
            case axdl_color_space_nv12:
            default:
                pipe1.m_output_type = po_buff_nv12;
                break;
            }
        }
        else
        {
            pipe1.enable = 0;
        }
        pipe1.n_loog_exit = 0;
        pipe1.m_vdec_attr.n_vdec_grp = 0;
        pipe1.output_func = ai_inference_func; // 图像输出的回调函数

        for (size_t i = 0; i < pipe_count; i++)
        {
            create_pipeline(&pipelines[i]);
            if (pipelines[i].m_ivps_attr.n_osd_rgn > 0)
            {
                g_sample.pipes_need_osd.push_back(&pipelines[i]);
            }
        }

        if (g_sample.pipes_need_osd.size() && g_sample.bRunJoint)
        {
            g_sample.osd_helper.Start(g_sample.gModels,g_sample.pipes_need_osd);
        }
    }

    {
        SAMPLE_BSPARSER_T tStreamInfo = {0};
        int sSize = 3 * 1024 * 1024;
        std::vector<unsigned char> cbuffer(sSize);
        // unsigned char *cbuffer = new unsigned char[1024 * 1024];
        pipeline_buffer_t buf_h26x = {0};
        buf_h26x.p_vir = cbuffer.data();

        FILE *fInput = NULL;
        fInput = fopen(h26xfile, "rb");
        if (fInput == NULL)
        {
            ALOGE("Unable to open input file\n");
            goto EXIT_6;
        }
        fseek(fInput, 0L, SEEK_END);
        AX_S32 sLen = ftell(fInput);
        rewind(fInput);

        tStreamInfo.fInput = fInput;
        tStreamInfo.sSize = sLen;

        AX_U32 sReadLen = 0;
        while (!gLoopExit)
        {
            sReadLen = StreamParserReadFrameH264(&tStreamInfo, cbuffer.data(), &sSize);
            buf_h26x.p_vir = cbuffer.data();
            buf_h26x.n_size = sReadLen;
            if (sReadLen == 0)
                break;
            else
                user_input(&pipelines[0], 1, &buf_h26x);
            usleep(10 * 1000);
        }
        ALOGN("h264 file decode finish,quit the loop");
        gLoopExit = 1;
        sleep(1);
        pipeline_buffer_t end_buf = {0};
        user_input(&pipelines[0], 1, &end_buf);
        fclose(fInput);
    }

    // 销毁pipeline
    {
        gLoopExit = 1;
        if (g_sample.pipes_need_osd.size() && g_sample.bRunJoint)
        {
            g_sample.osd_helper.Stop();
        }

        for (size_t i = 0; i < pipe_count; i++)
        {
            destory_pipeline(&pipelines[i]);
        }
    }

EXIT_6:

    axdl_deinit(&g_sample.gModels);

EXIT_2:

    COMMON_SYS_DeInit();
    g_sample.Deinit();

    ALOGN("sample end\n");
    return 0;
}
