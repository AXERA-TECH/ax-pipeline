## 简介
  通过 RTSP/MP4 输入，输出到rtsp流媒体服务器，实现 rtsp-in-rtsp-out 算力盒子。

## 环境准备
rtsp服务及推拉流配置请参考   [rtsp-simple-server](../../docs/rtsp.md)

## 快速体验
1、运行以下命令，进行多路 rtsp 的取流、解码、推理，并通过多路 rtsp 推流输出结果。
```
./sample_multi_demux_ivps_npu_multi_rtsp -f rtsp://192.168.31.1:5554/test -f rtsp://192.168.31.1:5554/test2 -p config/yolov5s.json -r 25
```
2、运行以下命令，进行多路 mp4/h264 文件的解包、解码、推理，并通过多路 rtsp 推流输出结果。
```
./sample_multi_demux_ivps_npu_multi_rtsp -f -f test.mp4 -f test2.mp4 -p config/yolov5s.json -r 25
```