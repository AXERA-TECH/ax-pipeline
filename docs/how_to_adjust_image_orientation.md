# 如何调整图像方向？

# 新版本

比如 [sample_vin_ivps_joint_vo](../examples/sample_vin_ivps_joint_vo)，我们如果想旋转、翻转、镜像等操作，可以打开 [sample_vin_ivps_joint_vo.cpp](../examples/sample_vin_ivps_joint_vo/sample_vin_ivps_joint_vo.cpp#L398) 398行左右

```C
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
```
可以参考以下结构体进行修改，结构体定义在 [pipeline_ivps_config_t](../examples/common/common_pipeline.h#L67)
```C
    typedef struct
    {
#define MAX_IVPS_GRP_COUNT 20
        int n_ivps_grp; // 少于20，并且不能重复

        int n_osd_rgn; // rgn的个数，设为0则表示不进行osd，少于 MAX_OSD_RGN_COUNT 个，每一个rgn可以绘制32个目标
#define MAX_OSD_RGN_COUNT 5
        int n_osd_rgn_chn[MAX_OSD_RGN_COUNT]; // rgn 的句柄

        int n_ivps_fps; // 输出的帧率

        // 0-0 1-90 2-180 3-270
        int n_ivps_rotate;

        // must be even number，必须是偶数
        int n_ivps_width;
        int n_ivps_height;

        int b_ivps_mirror; // 镜像
        int b_ivps_flip;   // 翻转

        int b_letterbox; // 填充，一般用于ai检测

        int n_fifo_count; // [0]表示不输出，[1-4]表示队列的个数，大于[0]则可以在调用回调输出图像
        unsigned long int tid; // internal variable
    } pipeline_ivps_config_t;
```

## 老版本

比如 [examples/sample_vin_ivps_joint_venc_rtsp_vo/pipe/pipeline_ivps.c](../examples/sample_vin_ivps_joint_venc_rtsp_vo/pipe/pipeline_ivps.c) SampleIvpsInit 函数中，大约 253 行：
```
...
stPipelineAttr.tFilter[nChn + 1][1].bEnable = AX_TRUE;
stPipelineAttr.tFilter[nChn + 1][1].tFRC.nSrcFrameRate = 60;
stPipelineAttr.tFilter[nChn + 1][1].tFRC.nDstFrameRate = 60;
stPipelineAttr.tFilter[nChn + 1][1].nDstPicOffsetX0 = 0;
stPipelineAttr.tFilter[nChn + 1][1].nDstPicOffsetY0 = 0;
stPipelineAttr.tFilter[nChn + 1][1].nDstPicWidth = SAMPLE_MINOR_STREAM_HEIGHT;
stPipelineAttr.tFilter[nChn + 1][1].nDstPicHeight = SAMPLE_MINOR_STREAM_WIDTH;
stPipelineAttr.tFilter[nChn + 1][1].nDstPicStride = ALIGN_UP(stPipelineAttr.tFilter[nChn + 1][0].nDstPicWidth, 64);
stPipelineAttr.tFilter[nChn + 1][1].nDstFrameWidth = SAMPLE_MINOR_STREAM_HEIGHT;
stPipelineAttr.tFilter[nChn + 1][1].nDstFrameHeight = SAMPLE_MINOR_STREAM_WIDTH;
stPipelineAttr.tFilter[nChn + 1][1].eDstPicFormat = AX_YUV420_SEMIPLANAR;
stPipelineAttr.tFilter[nChn + 1][1].eEngine = AX_IVPS_ENGINE_TDP;
stPipelineAttr.tFilter[nChn + 1][1].tTdpCfg.eRotation = AX_IVPS_ROTATION_90;
stPipelineAttr.nOutFifoDepth[nChn] = 0;
...
```
其中 ```stPipelineAttr.tFilter[nChn + 1][1].tTdpCfg.eRotation = AX_IVPS_ROTATION_90;``` 表示 旋转90度，因为 AXera-PI 的屏幕是纵向的，而图像是横向的。

 ```stPipelineAttr.tFilter[nChn + 1][1].tTdpCfg``` 中还有其他可供设置的参数，如上下、左右镜像操作等，用户可以根据需求进行设置，结构体具体定义如下：
 ```
 /*
 * AX_IVPS_TDP_CFG_S
 * This configuration is specific to TDP engine.
 * This engine can support many functions,
 * such as mirror,flip,rotation,scale,mosaic,osd and so on.
 * For AX620, if in eCompressMode, stride and width should be 64 pixels aligned.
 * If NOT in eCompressMode, stride and width should be 32 pixels aligned.
 * 16 alignment is also supported, but the bandwidth will decrease.
 */
typedef struct axIVPS_TDP_CFG_S
{
    AX_BOOL bCrop;
    AX_IVPS_RECT_S tCropRect;
    AX_BOOL bMirror;
    AX_BOOL bFlip;
    AX_IVPS_ROTATION_E eRotation;
    AX_IVPS_ASPECT_RATIO_S tAspectRatio;
    AX_BOOL bDither;
    AX_COMPRESS_MODE_E eCompressMode;
    AX_BOOL bInplace;
} AX_IVPS_TDP_CFG_S;
 ```

