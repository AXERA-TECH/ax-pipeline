# 更新日志

```2023-06-13``` 
- 新增 YoloV8-Pose
- 新增同时支持 h264、mp4文件、rtsp链接解析的 [VideoDemux类](../examples/common/video_demux.hpp)
- 新增native的osd支持，[osd绘图类](../examples/libaxdl/include/ax_osd_drawer.hpp)，提升osd效率以及易用性
- 新增 cv 处理 api，以及通用预处理类 [ax_imgproc_t](../examples/libaxdl/include/ax_common_api.h#L34)
- 新增 AX650 支持 
  - [HDMI 分屏显示](../examples/common/common_pipeline/ax650/common_pipeline_vo.cpp)
  - 多路编解码 [sample](../examples/sample_multi_demux_ivps_joint_hdmi_vo)
  - DinoV2
  - ...

```2023-01-16``` 新增YoloV8，详情请查阅 [ModelZoo](docs/modelzoo.md)

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

```2022-11-14``` 新增自适应 NV12/RGB/BGR 模型，通过 IVPS 直接输出模型需要的数据格式，现在 [ax-samples](https://github.com/AXERA-TECH/ax-samples) 的模型可以直接白嫖到 ax-pipeline 了（除了 yolov5 和 yolov5face，其他模型还是需要用户移植后处理部分）