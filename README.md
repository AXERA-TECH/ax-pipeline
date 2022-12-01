# ax-pipeline
**AX-Pipeline** 由 **[爱芯元智](https://www.axera-tech.com/)** 主导开发。该项目基于 **AXera-Pi** 展示 **ISP**、**图像处理**、**NPU**、**编码**、**显示** 等功能模块软件调用方法，方便社区开发者进行快速评估和二次开发自己的多媒体应用。

## 更新日志
```2022-11-29``` 新增人手检测加手势识别，感谢 [FeiGeChuanShu](https://github.com/FeiGeChuanShu)，详情请查阅 [ModelZoo](docs/modelzoo.md)。自适应联机模型输入格式，联机模型现在可以不需要相同的输入色彩空间格式

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
  
### 示例
- [sample_vin_joint](examples/sample_vin_joint)
- [sample_vin_ivps_joint_vo](examples/sample_vin_ivps_joint_vo)
- [sample_vin_ivps_joint_venc_rtsp](examples/sample_vin_ivps_joint_venc_rtsp)
- [sample_vin_ivps_joint_venc_rtsp_vo](examples/sample_vin_ivps_joint_venc_rtsp_vo)

### DEMO视频
https://user-images.githubusercontent.com/46700201/198552009-b55a5776-fc5a-4725-9906-d37548385359.mp4

https://user-images.githubusercontent.com/46700201/199662116-9f5748bd-8565-43ed-af99-c36903f9efdd.mp4

https://user-images.githubusercontent.com/46700201/203208967-4a6039f0-416e-4d5d-8c8f-f5acdc9d5787.mp4

https://user-images.githubusercontent.com/46700201/203263149-c7c33bee-3253-410c-9adf-77b982695f1e.mp4

https://user-images.githubusercontent.com/46700201/204993980-58762181-d19d-4529-b48b-a22f3542e3f9.mp4

## 技术讨论
- Github issues
- QQ 群: 139953715
