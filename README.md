# ax-pipeline

[![License](https://img.shields.io/badge/license-BSD--3--Clause-blue.svg)](https://raw.githubusercontent.com/AXERA-TECH/ax-pipeline/main/LICENSE)
[![GitHub Workflow Status](https://img.shields.io/github/actions/workflow/status/AXERA-TECH/ax-pipeline/build.yml?branch=main)](https://github.com/AXERA-TECH/ax-pipeline/actions)

## 简介

**AX-Pipeline** 由 **[爱芯元智](https://www.axera-tech.com/)** 主导开发。该项目基于 **AXera-Pi** 展示 [ISP 图像处理](https://zh.wikipedia.org/wiki/%E5%9C%96%E5%83%8F%E8%99%95%E7%90%86%E5%99%A8)、**NPU**、**编码**、**显示** 等功能模块软件调用方法，方便社区开发者进行快速评估和二次开发自己的多媒体应用。

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
- [如何加速子模块的下载](docs/how_to_speed_up_submodule_init.md)
  
### 示例
| 示例 | 简介|
|-|-|
| [sample_vin_ivps_joint_vo](examples/sample_vin_ivps_joint_vo) |IVPS 出两路视频，一路用作屏幕显示，一路用作 NPU 推理 |
| [sample_vin_ivps_joint_venc_rtsp](examples/sample_vin_ivps_joint_venc_rtsp) |IVPS 出三路视频，两路用作 RTSP 推流，一路用作 NPU 推理 |
| [sample_vin_ivps_joint_venc_rtsp_vo](examples/sample_vin_ivps_joint_venc_rtsp_vo) |IVPS 出三路视频，一路用作 RTSP 推流，一路用作屏幕显示，一路用作 NPU 推理|
| [sample_vin_ivps_joint_vo_h265](examples/sample_vin_ivps_joint_vo_h265) |IVPS 出三路视频，一路用作屏幕显示，一路用作 h265 文件保存，一路用作 NPU 推理|
| [sample_v4l2_ivps_joint_vo](examples/sample_v4l2_ivps_joint_vo) | USB的 jpeg 输入，IVPS 出两路视频，一路用作屏幕显示，一路用作 NPU 推理 |
| [sample_v4l2_user_ivps_joint_vo](examples/sample_v4l2_user_ivps_joint_vo) | USB的 jpeg 输入，使用libjpeg解码成NV12，输入到IVPS中，IVPS 出两路视频，一路用作屏幕显示，一路用作 NPU 推理，演示了如何将NV12的图像输入到IVPS中 |
| [sample_demux_ivps_joint_vo](examples/sample_demux_ivps_joint_vo) |读取 h264/mp4/rtsp 解码，通过IVPS 出两路视频，一路用作屏幕显示，一路用作 NPU 推理|
| [sample_demux_ivps_joint_rtsp](examples/sample_demux_ivps_joint_rtsp) | 读取 h264/mp4/rtsp 解码，IVPS 出两路视频，一路用作 RTSP 推流，一路用作 NPU 推理 |
| [sample_demux_ivps_joint_rtsp_vo](examples/sample_demux_ivps_joint_rtsp_vo) | 读取 h264/mp4/rtsp 解码，IVPS出三路视频，一路用作屏幕显示，一路用作 RTSP 推流，一路用作 NPU 推理 |
| [sample_multi_demux_ivps_joint_multi_rtsp](examples/sample_multi_demux_ivps_joint_multi_rtsp) | 读取多路 h264/mp4/rtsp 解码，推理模型进行 OSD 后，多路 rtsp 输出 |
| [sample_demux_ivps_joint_hdmi_vo](examples/sample_demux_ivps_joint_hdmi_vo) | 读取 h264/mp4/rtsp 解码，推理多个模型进行 OSD 后，分屏（分屏数量等于指定模型个数）同时输出到 HDMI 屏幕 |

## [更新日志](docs/update.md)
详情请看 [更新日志](docs/update.md)

## DEMO视频
https://user-images.githubusercontent.com/46700201/198552009-b55a5776-fc5a-4725-9906-d37548385359.mp4

https://user-images.githubusercontent.com/46700201/199662116-9f5748bd-8565-43ed-af99-c36903f9efdd.mp4

https://user-images.githubusercontent.com/46700201/203208967-4a6039f0-416e-4d5d-8c8f-f5acdc9d5787.mp4

https://user-images.githubusercontent.com/46700201/203263149-c7c33bee-3253-410c-9adf-77b982695f1e.mp4

https://user-images.githubusercontent.com/46700201/204993980-58762181-d19d-4529-b48b-a22f3542e3f9.mp4

## 技术讨论
- Github issues
- QQ 群: 139953715
  
