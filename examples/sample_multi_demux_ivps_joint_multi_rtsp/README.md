## 简介
  通过 RTSP/MP4 输入，输出到rtsp流媒体服务器，实现 rtsp-in-rtsp-out 算力盒子。


## 快速体验
1、准备想要进行推流的 h264 格式的二进制文件，或 h264 格式的 mp4 文件，下文用 test.h264、test2.h264

2、打开 [rtsp-simple-server](https://github.com/aler9/rtsp-simple-server/releases/tag/v0.21.0) 选择对应的平台，下载 rtsp-simple-server 可执行文件，并双击或者在命令行执行在后台(注意，内置的 rtsp 推流服务器默认是 8554，请修改 rtsp-simple-server.yml 中的配置避免端口冲突，我们这里修改成 5554)

3、下载安装 ffmpeg，通过以下命令，将 test.h264 推流
```
ffmpeg -re -stream_loop -1 -i test.h264 -rtsp_transport tcp -c copy -f rtsp rtsp://localhost:5554/test
```

4、通过以下命令，将 test2.h264 推流，这是第二路视频，如此类推，我们可以根据需求，再推几路视频。
```
ffmpeg -re -stream_loop -1 -i test2.h264 -rtsp_transport tcp -c copy -f rtsp rtsp://localhost:5554/test2
```

5、假设我们已知 rtsp-simple-server 运行的宿主机的 ip 为 192.168.31.1，并已知爱芯派也在宿主机同一局域网中

6、运行以下命令，进行 rtsp 的取流、解码、推理的 pipeline（重复设置 ```-f rtsp_url``` 进行多路输入输出）
```
./sample_multi_demux_ivps_joint_multi_rtsp -f rtsp://192.168.31.1:5554/test -f rtsp://192.168.31.1:5554/test2 -p config/yolov5s.json
```