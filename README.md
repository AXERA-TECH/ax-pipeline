# ax-pipeline
**AX-Pipeline** 由 **[爱芯元智](https://www.axera-tech.com/)** 主导开发。该项目基于 **AXera-Pi** 展示 **ISP**、**图像处理**、**NPU**、**编码**、**显示** 等功能模块软件调用方法，方便社区开发者进行快速评估和二次开发自己的多媒体应用。

## 更新日志
```2022-11-15``` 解耦 sample_run_joint，可以同时加载不同任务的模型，方便进行类似人脸识别、人体姿态、车牌识别等多级模型任务的推理。

```2022-11-14``` 新增自适应 NV12/RGB/BGR 模型，通过 IVPS 直接输出模型需要的数据格式，现在 [ax-samples]() 的模型可以直接白嫖到 ax-pipeline 了（除了 yolov5 和 yolov5face，其他模型还是需要用户移植后处理部分）

## 已支持开发板

- [AXera-Pi](https://wiki.sipeed.com/m3axpi)(AX620A)

## 快速上手

### 文档
- [快速编译](docs/compile.md)  基于 cmake 实现简单的跨平台编译。
- [如何更换自己训练的 yolov5 模型](docs/how_to_deploy_custom_yolov5_model.md)
- [如何部署自己的其他模型](docs/how_to_deploy_custom_model.md)
- [如何调整图像方向](docs/how_to_adjust_image_orientation.md)
  
  
### 示例
- [sample_vin_joint](examples/sample_vin_joint)
- [sample_vin_ivps_joint_vo](examples/sample_vin_ivps_joint_vo)
- [sample_vin_ivps_joint_venc_rtsp](examples/sample_vin_ivps_joint_venc_rtsp)
- [sample_vin_ivps_joint_venc_rtsp_vo](examples/sample_vin_ivps_joint_venc_rtsp_vo)

### DEMO视频
https://user-images.githubusercontent.com/46700201/198552009-b55a5776-fc5a-4725-9906-d37548385359.mp4

https://user-images.githubusercontent.com/46700201/199662116-9f5748bd-8565-43ed-af99-c36903f9efdd.mp4



## 技术讨论
- Github issues
- QQ 群: 139953715
