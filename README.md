# ax-pipeline
**AX-Pipeline** 由 **[爱芯元智](https://www.axera-tech.com/)** 主导开发。该项目基于 **AXera-Pi** 展示 **ISP**、**图像处理**、**NPU**、**编码**、**显示** 等功能模块软件调用方法，方便社区开发者进行快速评估和二次开发自己的多媒体应用。

## 更新日志
```2022-12-29``` 新增直接输入 NV12 数据的示例 [sample_v4l2_user_ivps_joint_vo](examples/sample_v4l2_user_ivps_joint_vo)，如果用户可以获取到 NV12 的数据，则可以参考此示例。为满足日益增加的模型支持列表，修改了枚举类 [SAMPLE_RUN_JOINT_MODEL_TYPE](examples/sample_run_joint/sample_run_joint_post_process.h) 的数值。

```2022-12-26``` 新增 rtsp 输入的示例代码 [sample_rtsp_ivps_joint_vo](examples/sample_rtsp_ivps_joint_vo)

```2022-12-26``` 新增人脸识别

```2022-12-16``` 新增 usb 相机输入的 sample。新增车牌检测、车牌识别，详情请查阅 [ModelZoo](docs/modelzoo.md)

```2022-12-14``` 新增 h264 文件输入 pipeline，新增yolov7-face、yolov7-palm-hand

```2022-12-09``` 新增简化的 pipeline 构建 api，降低构建 pipeline 的难度，详情请看 [new_pipeline](docs/new_pipeline.md)

```2022-11-29``` 新增人手检测加手势识别，感谢 [FeiGeChuanShu](https://github.com/FeiGeChuanShu)，详情请查阅 [ModelZoo](docs/modelzoo.md)。自适应联机模型输入格式，联级模型现在可以不需要相同的输入色彩空间格式

```2022-11-24``` 新增爱芯元智开源版本人体检测以及姿态模型

```2022-11-21``` 
- 新增 pp-human-seg 人像分割、yolov5s-seg 实例分割
- 新增二级推理模型 hrnet-pose（基于yolov5s检测人体后抠图实现的人体姿态检测），配置文件 ```MODEL_TYPE``` 支持字符串以及int设置，设置值对应见枚举 [SAMPLE_RUN_JOINT_MODEL_TYPE](examples/sample_run_joint/sample_run_joint_post_process.h) 或者 [ModelZoo](docs/modelzoo.md)
- 新增模型详细内容见 [ModelZoo](docs/modelzoo.md)
- 配置文件新增 ```MODEL_PATH```，用户可以对其进行设置，参考 [hrnet_pose.json](examples/sample_run_joint/config/hrnet_pose.json)

```2022-11-17``` 新增 yolov7-tiny、yolox-s 检测模型，配置文件新增了 ```MODEL_TYPE``` 的 int 属性，这个值必须要设置，不然就不跑了，设置值对应见枚举 [SAMPLE_RUN_JOINT_MODEL_TYPE](examples/sample_run_joint/sample_run_joint_post_process.h) 或者 [ModelZoo](docs/modelzoo.md)

```2022-11-15``` 解耦 sample_run_joint，可以同时加载不同任务的模型，方便进行类似人脸识别、人体姿态、车牌识别等多级模型任务的推理

```2022-11-14``` 新增自适应 NV12/RGB/BGR 模型，通过 IVPS 直接输出模型需要的数据格式，现在 [ax-samples]() 的模型可以直接白嫖到 ax-pipeline 了（除了 yolov5 和 yolov5face，其他模型还是需要用户移植后处理部分）

## 已支持开发板

- [AXera-Pi](https://wiki.sipeed.com/m3axpi)(AX620A)

## 快速上手

### 文档
- [快速编译](docs/compile.md)  基于 cmake 实现简单的跨平台编译。
- [如何更换自己训练的 yolov5 模型](docs/how_to_deploy_custom_yolov5_model.md)
- [如何部署自己的其他模型](docs/how_to_deploy_custom_model.md)
- [如何调整图像方向](docs/how_to_adjust_image_orientation.md)
- [ModelZoo](docs/modelzoo.md) 一些支持或将支持的模型，和一些模型的说明
- [配置文件说明](docs/config_file.md)
- [简化版本 pipeline 构建 api](docs/new_pipeline.md)
  
### 示例
| 示例 | 简介|
|-|-|
| [sample_vin_joint](examples/sample_vin_joint) |直接从 ISP 获取 NV12 数据进行 NPU 推理|
| [sample_vin_ivps_joint_vo](examples/sample_vin_ivps_joint_vo) |IVPS 出两路视频，一路用作屏幕显示，一路用作 NPU 推理|
| [sample_vin_ivps_joint_venc_rtsp](examples/sample_vin_ivps_joint_venc_rtsp) |IVPS 出三路视频，两路用作 RTSP 推流，一路用作 NPU 推理|
| [sample_vin_ivps_joint_venc_rtsp_vo](examples/sample_vin_ivps_joint_venc_rtsp_vo) |IVPS 出三路视频，一路用作 RTSP 推流，一路用作屏幕显示，一路用作 NPU 推理|
| [sample_vin_ivps_joint_vo_new](examples/sample_vin_ivps_joint_vo_new) |同 ```sample_vin_ivps_joint_vo```|
| [sample_vin_ivps_joint_venc_rtsp_new](examples/sample_vin_ivps_joint_venc_rtsp_new) |同 ```sample_vin_ivps_joint_venc_rtsp```|
| [sample_vin_ivps_joint_venc_rtsp_vo_new](examples/sample_vin_ivps_joint_venc_rtsp_vo_new) |同 ```sample_vin_ivps_joint_venc_rtsp_vo```|
| [sample_h264_ivps_joint_vo](examples/sample_h264_ivps_joint_vo) |读取 h264 文件解码，通过IVPS 出两路视频，一路用作屏幕显示，一路用作 NPU 推理|
| [sample_vin_ivps_joint_vo_h265_new](examples/sample_vin_ivps_joint_vo_h265_new) |IVPS 出三路视频，一路用作屏幕显示，一路用作 h265 文件保存，一路用作 NPU 推理|
| [sample_v4l2_ivps_joint_vo](examples/sample_v4l2_ivps_joint_vo) | USB的 jpeg 输入，IVPS 出两路视频，一路用作屏幕显示，一路用作 NPU 推理|

### DEMO视频
https://user-images.githubusercontent.com/46700201/198552009-b55a5776-fc5a-4725-9906-d37548385359.mp4

https://user-images.githubusercontent.com/46700201/199662116-9f5748bd-8565-43ed-af99-c36903f9efdd.mp4

https://user-images.githubusercontent.com/46700201/203208967-4a6039f0-416e-4d5d-8c8f-f5acdc9d5787.mp4

https://user-images.githubusercontent.com/46700201/203263149-c7c33bee-3253-410c-9adf-77b982695f1e.mp4

https://user-images.githubusercontent.com/46700201/204993980-58762181-d19d-4529-b48b-a22f3542e3f9.mp4

## 技术讨论
- Github issues
- QQ 群: 139953715
