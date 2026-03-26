# 配置格式(JSON)

`ax_pipeline_app` 通过一个 JSON 文件描述多个 pipeline 的行为。

## 顶层结构

```jsonc
{
  "system": {
    "device_id": -1,
    "enable_vdec": true,
    "enable_venc": true,
    "enable_ivps": true
  },
  "pipelines": [
    { /* Pipeline */ }
  ]
}
```

`system.device_id`:

- MSP 板端通常保持 `-1`
- AXCL 建议显式指定默认卡，pipeline 级别也可单独覆盖 `device_id`

## Pipeline

```jsonc
{
  "name": "p0",
  "device_id": -1,
  "uri": "xxx.mp4 或 rtsp://...",
  "realtime_playback": true,
  "loop_playback": false,

  "outputs": [
    {
      "codec": "h264",
      "width": 0,
      "height": 0,
      "frame_rate": 0,
      "bitrate_kbps": 0,
      "gop": 0,
      "input_queue_depth": 0,
      "overflow_policy": "drop_oldest",
      "resize": {
        "mode": "keep_aspect",
        "horizontal_align": "center",
        "vertical_align": "center",
        "background_color": 0
      },
      "uris": [
        "/tmp/out.mp4",
        "rtsp://0.0.0.0:8554/p0"
      ]
    }
  ],

  "frame_output": {
    "format": "bgr",
    "width": 640,
    "height": 640,
    "resize": { "mode": "keep_aspect", "background_color": 0 }
  },

  "npu_max_fps": 15,
  "npu": {
    "enable": true,
    "enable_osd": true,
    "enable_tracking": true,
    "track_buffer": 30,
    "model_path": "models/ax650/yolov8s.axmodel",
    "model_type": "yolov8",
    "num_classes": 80,
    "conf_threshold": 0.25,
    "nms_threshold": 0.45
  },

  "log_every_n_frames": 300
}
```

### 输入 `uri`

- MP4: 直接填写文件路径
  - `realtime_playback=true`: 按源 fps 节奏读取(播放器语义)
  - `realtime_playback=false`: 尽快读(离线转码)
  - `loop_playback=true`: 循环播放
- RTSP: `rtsp://...` 拉流

### 输出 `outputs[].uris`

同一个输出可以同时写 MP4 和推 RTSP:

- MP4: `"/tmp/out.mp4"`
- RTSP: `"rtsp://0.0.0.0:8554/stream_name"`

### `npu_max_fps`

- `> 0`: 限制 NPU 处理最大 FPS(最佳努力)
- `0` 或 `-1`: 不限速

### `frame_output`

控制 `GetLatestFrame()/frame callback` 的输出格式/尺寸/缩放策略。

注意:

- 当 `npu.enable && npu.enable_osd` 时，示例程序会强制让 frame_output 跟随解码原图，以保证检测坐标与 OSD 绘制坐标一致。

