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

`system.vdec_max_group_count` / `system.venc_total_thread_num`:

- 属于底层模块初始化参数；一般保持默认即可
- 当需要起大量 pipeline（例如 AX650/AXCL 32 路解码、16 路编码）时可显式调大
- `ax_pipeline_app` 会在 `enable_vdec=true` 时，把 `vdec_max_group_count` 自动提升到至少 `pipelines.size()`
- `venc_total_thread_num` 当前不会自动提升（部分平台把该值设得过大可能触发 `AX_VENC_Init failed`），需要时请手动配置

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
    "enable_tracking": false,
    "track_buffer": 30,
    "ax_plugin_path": "/path/to/libax_plugin_yolov8.so",
    "ax_plugin_isolation": "inproc",
    "ax_plugin_init_info": {
      "model_path": "models/ax650/yolov8s.axmodel",
      "num_classes": 80,
      "conf_threshold": 0.25,
      "nms_threshold": 0.45,

      // 可选：内置 YOLO 插件的“插件内跟踪”参数（yolov5/yolov8/yolov8_split）
      // 若启用插件内跟踪，建议 npu.enable_tracking=false，避免双重跟踪
      "enable_tracking": false,
      "track_fps": 30,
      "track_buffer": 30,
      "track_min_score": 0.0
    }
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

### `outputs[]`

`outputs` 描述该 pipeline 的编码/封装输出列表；每个 output 代表一条编码分支（`decode -> (可选 osd) -> encode -> mux`）。

常用字段：

- `codec`（可选，string）：编码类型
  - 可选值：`"h264"` / `"h265"`（也可写 `"hevc"`）
  - 默认：不填时由底层保持默认（建议显式填）
- `width` / `height` / `frame_rate`（可选，int/float）
  - 省略或传 `0`：跟随解码输入
  - `> 0`：输出 resize/重采样到指定参数
- `overflow_policy`（可选，string）：编码输入队列满时的策略
  - 可选值：`"drop_oldest"`（默认）/ `"drop_newest"` / `"block"`
- `resize`（可选，object）：输出分支 resize 策略（字段同 `frame_output.resize`）

### 不配置 `outputs`（仅解码/AI）

`outputs` 字段可以省略或配置为空数组 `[]`。此时 pipeline 只会做 `demux -> decode -> (可选 npu)`：

- 不会启动编码/推流，也不会写文件
- 如果开启了 `npu.enable_osd`，由于没有编码输出，OSD 不会被“画到”任何可播放的输出流上

### `npu_max_fps`

- `> 0`: 限制 NPU 处理最大 FPS(最佳努力)
- `0` 或 `-1`: 不限速

### `frame_output`

控制 `GetLatestFrame()/frame callback` 的输出格式/尺寸/缩放策略（也会影响 NPU 推理的输入空间）。

注意:

- `frame_output` 同时决定推理输入与对外回调的图像空间；当启用 `npu.enable_osd` 时，示例程序会把检测框从 `frame_output` 空间映射回解码原图空间再绘制 OSD。

字段说明（可显式配置；也可完全省略 `frame_output` 走默认行为）：

- `format`（可选，string）：输出像素格式
  - 可选值：
    - `"nv12"`（或 `"NV12"`）
    - `"rgb"` / `"rgb24"`（或大写）
    - `"bgr"` / `"bgr24"`（或大写）
  - 默认：跟随解码输出（通常为 NV12）
- `width` / `height`（可选，int）
  - 省略或传 `0`：跟随解码原始分辨率
  - `> 0`：输出 resize 到目标尺寸（建议 `width/height` 同时设置）
- `resize`（可选，object）：resize 策略
  - `mode`：`"stretch"`（默认）或 `"keep_aspect"` / `"keep_aspect_ratio"`
  - `horizontal_align` / `vertical_align`：`"start"` / `"center"`（默认）/ `"end"`
  - `background_color`：留边颜色（`0xRRGGBB`，默认 `0`）

常见用法示例：

- 直接拿到 NV12 原图（低开销；最常见）：省略 `frame_output` 字段即可；或显式写成：

```jsonc
"frame_output": { "format": "nv12" }
```

- 输出 NV12 并缩放到 960x544（保持比例、黑边填充）：

```jsonc
"frame_output": {
  "format": "nv12",
  "width": 960,
  "height": 544,
  "resize": { "mode": "keep_aspect", "background_color": 0 }
}
```

### 跟踪：主程序 vs 插件内

`ax_pipeline_app` 支持两种跟踪方式（二选一即可）：

- 主程序 ByteTrack：由主程序在 detector 输出上做跟踪（`npu.enable_tracking=true`，并使用 `npu.track_buffer`）
- 插件内 ByteTrack：由插件内部完成跟踪并输出 `det.track_id`
  - 目前内置 `yolov5/yolov8/yolov8_split` 三个插件均支持
  - 通过 `npu.ax_plugin_init_info.enable_tracking=true` 开启，并可选配置 `track_fps/track_buffer/track_min_score`
  - 建议同时保持 `npu.enable_tracking=false`，避免重复跟踪导致 ID 混乱或性能下降
  - 自定义插件也可以在插件内直接复用 `axpipeline::tracking::ByteTrack`

### `npu.ax_plugin_isolation`

- `"inproc"`: 在当前进程 `dlopen` 插件运行(最快)
  - 插件返回错误/抛异常不会影响 pipeline(只会导致该帧推理失败)
  - 但如果插件发生 `segfault/abort`，会导致整个 `ax_pipeline_app` 进程退出
- `"process"`: 在子进程运行插件(崩溃隔离)
  - 子进程崩溃时主 pipeline 会继续跑，只是短时间没有检测结果；随后会自动重启插件子进程
  - 可能有额外拷贝开销(例如 AXCL device->host)，性能可能低于 `"inproc"`
