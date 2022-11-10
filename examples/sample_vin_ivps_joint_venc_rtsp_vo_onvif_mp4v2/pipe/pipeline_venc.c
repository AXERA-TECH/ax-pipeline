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
#include "../sample_vin_ivps_joint_venc_rtsp_vo.h"
#include "mp4v2/mp4v2.h"
#include <time.h>

VENC_GETSTREAM_PARAM_T gGetStreamPara[SAMPLE_VENC_CHN_NUM];
pthread_t gGetStreamPid[SAMPLE_VENC_CHN_NUM];

static void *VencGetStreamProc(void *arg)
{
    AX_S32 s32Ret = -1;
    AX_VENC_RECV_PIC_PARAM_S stRecvParam;
    VENC_GETSTREAM_PARAM_T *pstPara;
    pstPara = (VENC_GETSTREAM_PARAM_T *)arg;
    AX_VENC_STREAM_S stStream;
    AX_S16 syncType = -1;
    AX_U32 totalGetStream = 0;

    s32Ret = AX_VENC_StartRecvFrame(pstPara->VeChn, &stRecvParam);
    if (AX_SUCCESS != s32Ret)
    {
        ALOGE("AX_VENC_StartRecvFrame failed, s32Ret:0x%x\n", s32Ret);
        return NULL;
    }

    memset(&stStream, 0, sizeof(AX_VENC_STREAM_S));

    while (AX_TRUE == pstPara->bThreadStart)
    {
        s32Ret = AX_VENC_GetStream(pstPara->VeChn, &stStream, syncType);

        if (AX_SUCCESS == s32Ret)
        {
            totalGetStream++;
            // ALOGN("VencChn %d: g_rtsplive %d session %d pts %u \n", pstPara->VeChn, g_rtsplive, rtsp_session, stStream.stPack.u64PTS);
            if (g_rtsplive)
            {
                rtsp_sever_tx_video(g_rtsplive, rtsp_session, stStream.stPack.pu8Addr, stStream.stPack.u32Len, stStream.stPack.u64PTS);
            }

            s32Ret = AX_VENC_ReleaseStream(pstPara->VeChn, &stStream);
            if (AX_SUCCESS != s32Ret)
            {
                // ALOGE("VencChn %d: AX_VENC_ReleaseStream failed!s32Ret:0x%x\n",pstPara->VeChn,s32Ret);
                goto EXIT;
            }
        }
    }

EXIT:
    ALOGN("VencChn %d: Total get %u encoded frames. getStream Exit!\n", pstPara->VeChn, totalGetStream);
    return NULL;
}

AX_S32 SAMPLE_COMM_VENC_MP4(AX_VENC_STREAM_S *stStream, bool rectrl)
{
    static bool recording = false;
    static bool vpsflag = false;
    static bool spsflag = false;
    static bool ppsflag = false;
    static int len = 0;
    static int timeScale = 90000;
    static MP4TrackId videoID = 0;
    static MP4FileHandle hMP4File = NULL;
    static AX_U64 SeqNum = 0;

    AX_U8 *pData = NULL;
    char isSyncSample = 0;
    char outputfile[150] = {0};

    if (rectrl)
    {
        if (hMP4File == NULL)
        {
            SeqNum = stStream->stPack.u64SeqNum;
            time_t timep;
            time(&timep);
            char tmp[64];
            strftime(tmp, sizeof(tmp), "%Y%m%d%H%M%S", localtime(&timep));
            sprintf(outputfile, "record_%s_%lld.mp4", tmp, SeqNum);
            hMP4File = MP4Create(outputfile, 0); //文件存储路径
            if (hMP4File == MP4_INVALID_FILE_HANDLE)
            {
                ALOGE("Creat MP4 file fialed.\n");
                return -1;
            }
            MP4SetTimeScale(hMP4File, timeScale);
            ALOGN("Create mp4  file !\n");
        }

        if (stStream->stPack.enCodingType == VENC_INTRA_FRAME) //从I帧开始编码，保证文件开始就能播放
            recording = true;

        if (recording)
        {
            if (stStream->stPack.enType == PT_H264)
            {
                // ALOGN("This pack have %d Nalus!\n", stStream->stPack.u32NaluNum);
                for (int j = 0; j < stStream->stPack.u32NaluNum; j++)
                {
                    len = stStream->stPack.stNaluInfo[j].u32NaluLength;
                    pData = (stStream->stPack.pu8Addr + stStream->stPack.stNaluInfo[j].u32NaluOffset);
                    // ALOGN("This Nalu len is %d !\n", len);
                    if (stStream->stPack.stNaluInfo[j].unNaluType.enH264EType == H264E_NALU_SPS)
                    {
                        if (!spsflag)
                        {
                            spsflag = true;
                            //写sps
                            ALOGN("Write sps!\n");
                            videoID = MP4AddH264VideoTrack(hMP4File,
                                                           timeScale,
                                                           timeScale / s_sample_framerate,
                                                           SAMPLE_MAJOR_STREAM_WIDTH,
                                                           SAMPLE_MAJOR_STREAM_HEIGHT,
                                                           pData[4 + 1], // sps[1] AVCProfileIndication
                                                           pData[4 + 2], // sps[2] profile_compat
                                                           pData[4 + 3], // sps[3] AVCLevelIndication
                                                           3);           // 4 bytes length before each NAL unit
                            MP4SetVideoProfileLevel(hMP4File, 0x7F);
                            MP4AddH264SequenceParameterSet(hMP4File, videoID, pData + 4, len - 4);
                        }
                        continue;
                    }

                    if (stStream->stPack.stNaluInfo[j].unNaluType.enH264EType == H264E_NALU_PPS)
                    {
                        if (!ppsflag)
                        {
                            ppsflag = true;
                            //写pps
                            ALOGN("Write pps!\n");
                            MP4AddH264PictureParameterSet(hMP4File, videoID, pData + 4, len - 4);
                        }
                        continue;
                    }

                    isSyncSample = (stStream->stPack.stNaluInfo[j].unNaluType.enH264EType == H264E_NALU_ISLICE) ? (1) : (0);
                    pData[0] = (len - 4) >> 24;
                    pData[1] = (len - 4) >> 16;
                    pData[2] = (len - 4) >> 8;
                    pData[3] = len - 4;
                    MP4WriteSample(hMP4File, videoID, pData, len, MP4_INVALID_DURATION, 0, isSyncSample);
                }
            }
            else if (stStream->stPack.enType == PT_H265)
            {
                // ALOGN("This pack have %d Nalus!\n", stStream->stPack.u32NaluNum);
                for (int j = 0; j < stStream->stPack.u32NaluNum; j++)
                {
                    len = stStream->stPack.stNaluInfo[j].u32NaluLength;
                    pData = (stStream->stPack.pu8Addr + stStream->stPack.stNaluInfo[j].u32NaluOffset);
                    // ALOGN("This Nalu len %d !\n", len);
                    if (stStream->stPack.stNaluInfo[j].unNaluType.enH265EType == H265E_NALU_VPS)
                    {
                        if (!vpsflag)
                        {
                            vpsflag = true;
                            //写vps
                            ALOGN("Write vps!\n");
                            videoID = MP4AddH265VideoTrack(hMP4File,
                                                           timeScale,
                                                           timeScale / s_sample_framerate,
                                                           SAMPLE_MAJOR_STREAM_WIDTH,
                                                           SAMPLE_MAJOR_STREAM_HEIGHT,
                                                           pData[4 + 1], // sps[1] AVCProfileIndication
                                                           pData[4 + 2], // sps[2] profile_compat
                                                           pData[4 + 3], // sps[3] AVCLevelIndication
                                                           3);           // 4 bytes length before each NAL unit
                            MP4SetVideoProfileLevel(hMP4File, 0x7F);
                            MP4AddH265VideoParameterSet(hMP4File, videoID, pData + 4, len - 4);
                        }
                        continue;
                    }

                    if (stStream->stPack.stNaluInfo[j].unNaluType.enH265EType == H265E_NALU_SPS)
                    {
                        if (!spsflag)
                        {
                            spsflag = true;
                            //写sps
                            ALOGN("Write sps!\n");
                            MP4AddH265SequenceParameterSet(hMP4File, videoID, pData + 4, len - 4);
                        }
                        continue;
                    }

                    if (stStream->stPack.stNaluInfo[j].unNaluType.enH265EType == H265E_NALU_PPS)
                    {
                        if (!ppsflag)
                        {
                            ppsflag = true;
                            //写pps
                            ALOGN("Write pps!\n");
                            MP4AddH265PictureParameterSet(hMP4File, videoID, pData + 4, len - 4);
                        }
                        continue;
                    }

                    isSyncSample = (stStream->stPack.stNaluInfo[j].unNaluType.enH265EType == H265E_NALU_ISLICE) ? (1) : (0);
                    pData[0] = (len - 4) >> 24;
                    pData[1] = (len - 4) >> 16;
                    pData[2] = (len - 4) >> 8;
                    pData[3] = len - 4;
                    MP4WriteSample(hMP4File, videoID, pData, len, MP4_INVALID_DURATION, 0, isSyncSample);
                }
            }
            else
            {
                ALOGN("Not support this type of payload!\n");
            }
        }
    }
    else if (!rectrl && recording)
    {
        recording = false;
        MP4Close(hMP4File, 0);
        hMP4File = NULL;
        videoID = 0;
        vpsflag = false;
        spsflag = false;
        ppsflag = false;
        ALOGN("Save %d frames to MP4 file!\n", stStream->stPack.u64SeqNum - SeqNum);
        SeqNum = 0;
    }
    return 0;
}

void set_gpio_led_irq_in(unsigned int gpio_num, unsigned int led_num) // user key num 85 led0 68 led1 69
{
    FILE *fp;
    char file_name[50];
    unsigned int led = led_num + 68;

    sprintf(file_name, "/sys/class/gpio/export");
    fp = fopen(file_name, "w");
    if (fp == NULL)
    {
        printf("Cannot open %s.\n", file_name);
        return -1;
    }
    fprintf(fp, "%d", gpio_num);
    fclose(fp);

    sprintf(file_name, "/sys/class/gpio/export");
    fp = fopen(file_name, "w");
    if (fp == NULL)
    {
        printf("Cannot open %s.\n", file_name);
        return -1;
    }
    fprintf(fp, "%d", led);
    fclose(fp);

    sprintf(file_name, "/sys/class/gpio/gpio%d/direction", gpio_num);
    fp = fopen(file_name, "rb+");
    if (fp == NULL)
    {
        printf("Cannot open %s.\n", file_name);
        return -1;
    }
    fprintf(fp, "in");
    fclose(fp);

    sprintf(file_name, "/sys/class/gpio/gpio%d/edge", gpio_num);
    fp = fopen(file_name, "rb+");
    if (fp == NULL)
    {
        printf("Cannot open %s.\n", file_name);
        return -1;
    }
    fprintf(fp, "falling");
    fclose(fp);

    sprintf(file_name, "/sys/class/gpio/gpio%d/direction", led);
    fp = fopen(file_name, "rb+");
    if (fp == NULL)
    {
        printf("Cannot open %s.\n", file_name);
        return -1;
    }
    fprintf(fp, "out");
    fclose(fp);

    sprintf(file_name, "/sys/class/gpio/gpio%d/value", led);
    fp = fopen(file_name, "rb+");
    if (fp == NULL)
    {
        printf("Cannot open %s.\n", file_name);
        return -1;
    }
    fprintf(fp, "0");
    fclose(fp);
}

void set_led_status(unsigned int led_num, bool led_status) // user key num 85 led0 68 led1 69
{
    FILE *fp;
    char file_name[50];
    unsigned int led = led_num + 68;

    sprintf(file_name, "/sys/class/gpio/gpio%d/value", led);
    fp = fopen(file_name, "rb+");
    if (fp == NULL)
    {
        printf("Cannot open %s.\n", file_name);
        return -1;
    }
    if (led_status)
        fprintf(fp, "1");
    else
        fprintf(fp, "0");
    fclose(fp);
}

void unset_gpio_led_irq(unsigned int gpio_num, unsigned int led_num)
{
    FILE *fp;
    char file_name[50];
    unsigned char buf[10];
    unsigned int led = led_num + 68;

    sprintf(file_name, "/sys/class/gpio/unexport");
    fp = fopen(file_name, "w");
    if (fp == NULL)
    {
        printf("Cannot open %s.\n", file_name);
        return -1;
    }
    fprintf(fp, "%d", gpio_num);
    fclose(fp);

    sprintf(file_name, "/sys/class/gpio/unexport");
    fp = fopen(file_name, "w");
    if (fp == NULL)
    {
        printf("Cannot open %s.\n", file_name);
        return -1;
    }
    fprintf(fp, "%d", led);
    fclose(fp);
}

/* venc save stream task */
static void *VencSaveStreamProc(void *arg)
{
    AX_S32 s32Ret = -1;
    AX_VENC_RECV_PIC_PARAM_S stRecvParam;
    VENC_GETSTREAM_PARAM_T *pstPara;
    pstPara = (VENC_GETSTREAM_PARAM_T *)arg;
    AX_VENC_STREAM_S stStream;
    AX_S16 syncType = -1;
    AX_U32 totalGetStream = 0;
    bool rectrl = false;

    int val;
    char buff[10];
    struct pollfd fds;
    int gpio_fd = -1;

    set_gpio_led_irq_in(85, 0);

    gpio_fd = open("/sys/class/gpio/gpio85/value", O_RDONLY);
    if (gpio_fd == -1)
        ALOGE("gpio open err! \n");
    fds.fd = gpio_fd;
    fds.events = POLLPRI;

    s32Ret = read(gpio_fd, buff, 10);
    if (s32Ret == -1)
        ALOGE("read gpio err! \n");

    s32Ret = AX_VENC_StartRecvFrame(pstPara->VeChn, &stRecvParam);
    if (AX_SUCCESS != s32Ret)
    {
        ALOGE("AX_VENC_StartRecvFrame failed, s32Ret:0x%x\n", s32Ret);
        return NULL;
    }
    memset(&stStream, 0, sizeof(AX_VENC_STREAM_S));
    while (AX_TRUE == pstPara->bThreadStart)
    {
        s32Ret = poll(&fds, 1, 0);
        if (s32Ret == -1)
            printf("poll");

        if (fds.revents & POLLPRI)
        {
            s32Ret = lseek(gpio_fd, 0, SEEK_SET);
            if (s32Ret == -1)
                ALOGE("lseek err! \n");
            s32Ret = read(gpio_fd, buff, 10);
            if (s32Ret == -1)
                ALOGE("read gpio err! \n");
            if (!rectrl)
            {
                rectrl = true;
                set_led_status(0, rectrl);
            }
            else
            {
                rectrl = false;
                set_led_status(0, rectrl);
            }

            ALOGN("get interrupt\n");
        }

        s32Ret = AX_VENC_GetStream(pstPara->VeChn, &stStream, syncType);
        if (AX_SUCCESS == s32Ret)
        {
            totalGetStream++;

            SAMPLE_COMM_VENC_MP4(&stStream, rectrl);

            s32Ret = AX_VENC_ReleaseStream(pstPara->VeChn, &stStream);
            if (AX_SUCCESS != s32Ret)
            {
                goto EXIT1;
            }
        }
    }

    unset_gpio_led_irq(85, 0);
EXIT1:
    ALOGN("VencChn %d: Total get %u encoded frames. getStream Exit!\n", pstPara->VeChn, totalGetStream);
    return NULL;
}

AX_S32 SampleVencInit(COMMON_VENC_CASE_E eVencType)
{
    AX_VENC_CHN_ATTR_S stVencChnAttr;
    VIDEO_CONFIG_T config = {0};
    AX_S32 VencChn = 0, s32Ret = 0;

    AX_VENC_MOD_ATTR_S stModAttr;
    stModAttr.enVencType = VENC_MULTI_ENCODER;

    s32Ret = AX_VENC_Init(&stModAttr);
    if (AX_SUCCESS != s32Ret)
    {
        ALOGE("AX_VENC_Init failed, s32Ret:0x%x", s32Ret);
        return s32Ret;
    }

    config.stRCInfo.eRCType = VENC_RC_VBR;
    config.nGOP = 50;
    config.nBitrate = 4096;
    config.stRCInfo.nMinQp = 10;
    config.stRCInfo.nMaxQp = 51;
    config.stRCInfo.nMinIQp = 10;
    config.stRCInfo.nMaxIQp = 51;
    config.stRCInfo.nIntraQpDelta = -2;
    config.nOffsetCropX = 0;
    config.nOffsetCropY = 0;
    config.nOffsetCropW = 0;
    config.nOffsetCropH = 0;

    for (VencChn = 0; VencChn < SAMPLE_VENC_CHN_NUM; VencChn++)
    {
        config.ePayloadType = (eVencType == VENC_CASE_H264) ? PT_H264 : PT_H265;
        switch (VencChn)
        {
        case 0:
            config.nInWidth = gCams[0].stChnAttr.tChnAttr[AX_YUV_SOURCE_ID_MAIN].nWidth;
            config.nInHeight = gCams[0].stChnAttr.tChnAttr[AX_YUV_SOURCE_ID_MAIN].nHeight;
            config.nStride = gCams[0].stChnAttr.tChnAttr[AX_YUV_SOURCE_ID_MAIN].nWidthStride;
            config.nSrcFrameRate = s_sample_framerate;
            config.nDstFrameRate = s_sample_framerate;
            break;
        case 1:
            config.nInWidth = SAMPLE_FHD_STREAM_WIDTH;
            config.nInHeight = SAMPLE_FHD_STREAM_HEIGHT;
            config.nStride = SAMPLE_FHD_STREAM_WIDTH;
            config.nSrcFrameRate = s_sample_framerate;
            config.nDstFrameRate = s_sample_framerate;
            break;
        }

        memset(&stVencChnAttr, 0, sizeof(AX_VENC_CHN_ATTR_S));

        stVencChnAttr.stVencAttr.u32MaxPicWidth = 0;
        stVencChnAttr.stVencAttr.u32MaxPicHeight = 0;

        stVencChnAttr.stVencAttr.u32PicWidthSrc = config.nInWidth;   /*the picture width*/
        stVencChnAttr.stVencAttr.u32PicHeightSrc = config.nInHeight; /*the picture height*/

        stVencChnAttr.stVencAttr.u32CropOffsetX = config.nOffsetCropX;
        stVencChnAttr.stVencAttr.u32CropOffsetY = config.nOffsetCropY;
        stVencChnAttr.stVencAttr.u32CropWidth = config.nOffsetCropW;
        stVencChnAttr.stVencAttr.u32CropHeight = config.nOffsetCropH;
        stVencChnAttr.stVencAttr.u32VideoRange = 1; /* 0: Narrow Range(NR), Y[16,235], Cb/Cr[16,240]; 1: Full Range(FR), Y/Cb/Cr[0,255] */

        ALOGN("VencChn %d:w:%d, h:%d, s:%d, Crop:(%d, %d, %d, %d) rcType:%d, payload:%d", gVencChnMapping[VencChn], stVencChnAttr.stVencAttr.u32PicWidthSrc, stVencChnAttr.stVencAttr.u32PicHeightSrc, config.nStride, stVencChnAttr.stVencAttr.u32CropOffsetX, stVencChnAttr.stVencAttr.u32CropOffsetY, stVencChnAttr.stVencAttr.u32CropWidth, stVencChnAttr.stVencAttr.u32CropHeight, config.stRCInfo.eRCType, config.ePayloadType);

        stVencChnAttr.stVencAttr.u32BufSize = config.nStride * config.nInHeight * 3 / 2; /*stream buffer size*/
        stVencChnAttr.stVencAttr.u32MbLinesPerSlice = 0;                                 /*get stream mode is slice mode or frame mode?*/
        stVencChnAttr.stVencAttr.enLinkMode = AX_LINK_MODE;
        stVencChnAttr.stVencAttr.u32GdrDuration = 0;
        /* GOP Setting */
        stVencChnAttr.stGopAttr.enGopMode = VENC_GOPMODE_NORMALP;

        stVencChnAttr.stVencAttr.enType = config.ePayloadType;
        switch (stVencChnAttr.stVencAttr.enType)
        {
        case PT_H265:
        {
            stVencChnAttr.stVencAttr.enProfile = VENC_HEVC_MAIN_PROFILE;
            stVencChnAttr.stVencAttr.enLevel = VENC_HEVC_LEVEL_6;
            stVencChnAttr.stVencAttr.enTier = VENC_HEVC_MAIN_TIER;

            if (config.stRCInfo.eRCType == VENC_RC_CBR)
            {
                AX_VENC_H265_CBR_S stH265Cbr;
                stVencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_H265CBR;
                stVencChnAttr.stRcAttr.s32FirstFrameStartQp = -1;
                stH265Cbr.u32Gop = config.nGOP;
                stH265Cbr.u32SrcFrameRate = config.nSrcFrameRate;  /* input frame rate */
                stH265Cbr.fr32DstFrameRate = config.nDstFrameRate; /* target frame rate */
                stH265Cbr.u32BitRate = config.nBitrate;
                stH265Cbr.u32MinQp = config.stRCInfo.nMinQp;
                stH265Cbr.u32MaxQp = config.stRCInfo.nMaxQp;
                stH265Cbr.u32MinIQp = config.stRCInfo.nMinIQp;
                stH265Cbr.u32MaxIQp = config.stRCInfo.nMaxIQp;
                stH265Cbr.s32IntraQpDelta = config.stRCInfo.nIntraQpDelta;
                memcpy(&stVencChnAttr.stRcAttr.stH265Cbr, &stH265Cbr, sizeof(AX_VENC_H265_CBR_S));
            }
            else if (config.stRCInfo.eRCType == VENC_RC_VBR)
            {
                AX_VENC_H265_VBR_S stH265Vbr;
                stVencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_H265VBR;
                stVencChnAttr.stRcAttr.s32FirstFrameStartQp = -1;
                stH265Vbr.u32Gop = config.nGOP;
                stH265Vbr.u32SrcFrameRate = config.nSrcFrameRate;
                stH265Vbr.fr32DstFrameRate = config.nDstFrameRate;
                stH265Vbr.u32MaxBitRate = config.nBitrate;
                stH265Vbr.u32MinQp = config.stRCInfo.nMinQp;
                stH265Vbr.u32MaxQp = config.stRCInfo.nMaxQp;
                stH265Vbr.u32MinIQp = config.stRCInfo.nMinIQp;
                stH265Vbr.u32MaxIQp = config.stRCInfo.nMaxIQp;
                stH265Vbr.s32IntraQpDelta = config.stRCInfo.nIntraQpDelta;
                memcpy(&stVencChnAttr.stRcAttr.stH265Vbr, &stH265Vbr, sizeof(AX_VENC_H265_VBR_S));
            }
            else if (config.stRCInfo.eRCType == VENC_RC_FIXQP)
            {
                AX_VENC_H265_FIXQP_S stH265FixQp;
                stVencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_H265FIXQP;
                stH265FixQp.u32Gop = config.nGOP;
                stH265FixQp.u32SrcFrameRate = config.nSrcFrameRate;
                stH265FixQp.fr32DstFrameRate = config.nDstFrameRate;
                stH265FixQp.u32IQp = 25;
                stH265FixQp.u32PQp = 30;
                stH265FixQp.u32BQp = 32;
                memcpy(&stVencChnAttr.stRcAttr.stH265FixQp, &stH265FixQp, sizeof(AX_VENC_H265_FIXQP_S));
            }
            break;
        }
        case PT_H264:
        {
            stVencChnAttr.stVencAttr.enProfile = VENC_H264_MAIN_PROFILE;
            stVencChnAttr.stVencAttr.enLevel = VENC_H264_LEVEL_5_2;

            if (config.stRCInfo.eRCType == VENC_RC_CBR)
            {
                AX_VENC_H264_CBR_S stH264Cbr;
                stVencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_H264CBR;
                stVencChnAttr.stRcAttr.s32FirstFrameStartQp = -1;
                stH264Cbr.u32Gop = config.nGOP;
                stH264Cbr.u32SrcFrameRate = config.nSrcFrameRate;  /* input frame rate */
                stH264Cbr.fr32DstFrameRate = config.nDstFrameRate; /* target frame rate */
                stH264Cbr.u32BitRate = config.nBitrate;
                stH264Cbr.u32MinQp = config.stRCInfo.nMinQp;
                stH264Cbr.u32MaxQp = config.stRCInfo.nMaxQp;
                stH264Cbr.u32MinIQp = config.stRCInfo.nMinIQp;
                stH264Cbr.u32MaxIQp = config.stRCInfo.nMaxIQp;
                stH264Cbr.s32IntraQpDelta = config.stRCInfo.nIntraQpDelta;
                memcpy(&stVencChnAttr.stRcAttr.stH264Cbr, &stH264Cbr, sizeof(AX_VENC_H264_CBR_S));
            }
            else if (config.stRCInfo.eRCType == VENC_RC_VBR)
            {
                AX_VENC_H264_VBR_S stH264Vbr;
                stVencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_H264VBR;
                stVencChnAttr.stRcAttr.s32FirstFrameStartQp = -1;
                stH264Vbr.u32Gop = config.nGOP;
                stH264Vbr.u32SrcFrameRate = config.nSrcFrameRate;
                stH264Vbr.fr32DstFrameRate = config.nDstFrameRate;
                stH264Vbr.u32MaxBitRate = config.nBitrate;
                stH264Vbr.u32MinQp = config.stRCInfo.nMinQp;
                stH264Vbr.u32MaxQp = config.stRCInfo.nMaxQp;
                stH264Vbr.u32MinIQp = config.stRCInfo.nMinIQp;
                stH264Vbr.u32MaxIQp = config.stRCInfo.nMaxIQp;
                stH264Vbr.s32IntraQpDelta = config.stRCInfo.nIntraQpDelta;
                memcpy(&stVencChnAttr.stRcAttr.stH264Vbr, &stH264Vbr, sizeof(AX_VENC_H264_VBR_S));
            }
            else if (config.stRCInfo.eRCType == VENC_RC_FIXQP)
            {
                AX_VENC_H264_FIXQP_S stH264FixQp;
                stVencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_H264FIXQP;
                stH264FixQp.u32Gop = config.nGOP;
                stH264FixQp.u32SrcFrameRate = config.nSrcFrameRate;
                stH264FixQp.fr32DstFrameRate = config.nDstFrameRate;
                stH264FixQp.u32IQp = 25;
                stH264FixQp.u32PQp = 30;
                stH264FixQp.u32BQp = 32;
                memcpy(&stVencChnAttr.stRcAttr.stH264FixQp, &stH264FixQp, sizeof(AX_VENC_H264_FIXQP_S));
            }
            break;
        }
        default:
            ALOGE("VencChn %d:Payload type unrecognized.", VencChn);
            return -1;
        }

        AX_S32 ret = AX_VENC_CreateChn(gVencChnMapping[VencChn], &stVencChnAttr);
        if (AX_SUCCESS != ret)
        {
            ALOGE("VencChn %d: AX_VENC_CreateChn failed, s32Ret:0x%x", VencChn, ret);
            return -1;
        }

        /* create get output stream thread */

        gGetStreamPara[VencChn].VeChn = gVencChnMapping[VencChn];
        gGetStreamPara[VencChn].bThreadStart = AX_TRUE;
        gGetStreamPara[VencChn].ePayloadType = config.ePayloadType;

        switch (VencChn)
        {
        case 0:
            pthread_create(&gGetStreamPid[VencChn], NULL, VencSaveStreamProc, (void *)&gGetStreamPara[VencChn]);
            break;
        case 1:
            pthread_create(&gGetStreamPid[VencChn], NULL, VencGetStreamProc, (void *)&gGetStreamPara[VencChn]);
            break;
        }
    }

    return 0;
}

AX_S32 SampleVencDeInit()
{
    AX_S32 VencChn = 0, s32Ret = 0;

    for (VencChn = 0; VencChn < SAMPLE_VENC_CHN_NUM; VencChn++)
    {

        s32Ret = AX_VENC_StopRecvFrame(gVencChnMapping[VencChn]);
        if (0 != s32Ret)
        {
            ALOGE("VencChn %d:AX_VENC_StopRecvFrame failed,s32Ret:0x%x.\n", gVencChnMapping[VencChn], s32Ret);
            return s32Ret;
        }

        s32Ret = AX_VENC_DestroyChn(gVencChnMapping[VencChn]);
        if (0 != s32Ret)
        {
            ALOGE("VencChn %d:AX_VENC_DestroyChn failed,s32Ret:0x%x.\n", gVencChnMapping[VencChn], s32Ret);
            return s32Ret;
        }

        if (AX_TRUE == gGetStreamPara[VencChn].bThreadStart)
        {
            gGetStreamPara[VencChn].bThreadStart = AX_FALSE;
            pthread_join(gGetStreamPid[VencChn], NULL);
        }
    }
    s32Ret = AX_VENC_Deinit();
    if (AX_SUCCESS != s32Ret)
    {
        ALOGE("AX_VENC_Deinit failed, s32Ret=0x%x", s32Ret);
        return s32Ret;
    }

    return 0;
}
