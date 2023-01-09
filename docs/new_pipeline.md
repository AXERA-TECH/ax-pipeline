# 简化版pipeline的讲解
主要是想简化pipeline的创建

原先带的三个pipeline，现在都用新的api创建了一次，降低心智负担的同时，提高效率。
- [sample_vin_ivps_joint_vo](../examples/sample_vin_ivps_joint_vo) -> [sample_vin_ivps_joint_vo_new](../examples/sample_vin_ivps_joint_vo_new)
- [sample_vin_ivps_joint_venc_rtsp](../examples/sample_vin_ivps_joint_venc_rtsp) ->  [sample_vin_ivps_joint_venc_rtsp_new](../examples/sample_vin_ivps_joint_venc_rtsp_new)
- [sample_vin_ivps_joint_venc_rtsp_vo](../examples/sample_vin_ivps_joint_venc_rtsp_vo) -> [sample_vin_ivps_joint_venc_rtsp_vo_new](../examples/sample_vin_ivps_joint_venc_rtsp_vo_new)


例如，我们想创建一路，从相机拿到数据，rtsp 输出的 pipeline
```c++
pipeline_t pipe_rtsp;
{
    pipeline_ivps_config_t &config = pipe_rtsp.m_ivps_attr;
    config.n_ivps_grp = 0;
    config.n_ivps_fps = 25;
    config.n_ivps_width = 1920;
    config.n_ivps_height = 1080;
    config.n_osd_rgn = 1; // osd rgn 的个数（最多五个），一个rgn可以osd 32个目标，现在用的是自定义的rgba画布，所以指挥占用一个rgn里的一个目标，所以这里只创建一个
}
pipe_rtsp.enable = 1;
pipe_rtsp.pipeid = 0x90015;
pipe_rtsp.m_input_type = pi_vin;
pipe_rtsp.m_output_type = po_rtsp_h265; //可以创建265，降低带宽压力
pipe_rtsp.n_loog_exit = 0; // 可以用来控制线程退出（如果有的话）
pipe_rtsp.n_vin_pipe = 0;
pipe_rtsp.n_vin_chn = 0;
sprintf(pipe_rtsp.m_venc_attr.end_point, "axstream0"); // 重复的会创建失败
pipe_rtsp.m_venc_attr.n_venc_chn = 0;                  // 重复的会创建失败
create_pipeline(&pipe_rtsp);
```

例如，我们想创建一路，从相机拿到对应分辨率的数据，并对数据进行letterbox填充缩放，使用ai检测pipeline
```c++
void ai_inference_func(pipeline_buffer_t *buff)
{
    static sample_run_joint_results mResults;
    AX_NPU_CV_Image tSrcFrame = {0};

    tSrcFrame.eDtype = (AX_NPU_CV_FrameDataType)buff->d_type;
    tSrcFrame.nWidth = buff->n_width;
    tSrcFrame.nHeight = buff->n_height;
    tSrcFrame.pVir = (unsigned char *)buff->p_vir;
    tSrcFrame.pPhy = buff->p_phy;
    tSrcFrame.tStride.nW = buff->n_stride;
    tSrcFrame.nSize = buff->n_size;

    sample_run_joint_inference_single_func(gModels, &tSrcFrame, &mResults);
}
...

pipeline_t pipe_ai;
{
    pipeline_ivps_config_t &config = pipe_ai.m_ivps_attr;
    config.n_ivps_grp = 1; // 重复的会创建失败
    config.n_ivps_fps = 60;
    config.n_ivps_width = 640;
    config.n_ivps_height = 640;
    config.b_letterbox = 1;   //填充缩放
    config.n_fifo_count = 1;  // 如果想要拿到数据并输出到回调 就设为1~4
}
pipe_ai.enable = 1;
pipe_ai.pipeid = 0x90016;
pipe_ai.m_input_type = pi_vin;
switch (gModels.SAMPLE_ALGO_FORMAT)
{
case AX_FORMAT_RGB888:
    pipe_ai.m_output_type = po_buff_rgb;
    break;
case AX_FORMAT_BGR888:
    pipe_ai.m_output_type = po_buff_bgr;
    break;
case AX_YUV420_SEMIPLANAR:
default:
    pipe_ai.m_output_type = po_buff_nv12;
    break;
}
pipe_ai.n_loog_exit = 0;
pipe_ai.n_vin_pipe = 0;
pipe_ai.n_vin_chn = 0;
pipe_ai.output_func = ai_inference_func; // 图像输出的回调函数
create_pipeline(&pipe_ai);
```

例如，我们想创建一路，从相机拿到数据，输出到爱芯派屏幕的 pipeline
```c++
 pipeline_t pipe_vo;
{
    pipeline_ivps_config_t &config = pipe_vo.m_ivps_attr;
    config.n_ivps_grp = 0;    // 重复的会创建失败
    config.n_ivps_fps = 60;   // 屏幕只能是60gps
    config.n_ivps_rotate = 1; // 旋转
    config.n_ivps_width = 854;
    config.n_ivps_height = 480;
    config.n_osd_rgn = 1; // osd rgn 的个数，一个rgn可以osd 32个目标
}
pipe_vo.enable = 1;
pipe_vo.pipeid = 0x90015;
pipe_vo.m_input_type = pi_vin;
pipe_vo.m_output_type = po_vo_sipeed_maix3_screen;
pipe_vo.n_loog_exit = 0; // 可以用来控制线程退出（如果有的话）
pipe_vo.n_vin_pipe = 0;
pipe_vo.n_vin_chn = 0;
create_pipeline(&pipe_vo);
```

一切都变得清晰简单

## ***pipeline_t*** 的参数解释可以直接看[头文件的解释](../examples/common/common_pipeline.h)
