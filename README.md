# ax-pipeline

基于 `ax-video-sdk` 的多路 pipeline 运行器示例工程。

本仓库目标是提供一个简单清晰的运行入口：

- 多个 `pipeline` 并行运行
- 每个 pipeline 固定为 `1 demux + N mux`
- NPU 节点当前为 stub：
  - 从 pipeline 获取 `BGR 640x640` 的 letterbox 图像
  - 打印图像信息
  - 通过 `npu_max_fps` 做吞吐限速（模拟 NPU 处理能力上限）
- 通过 JSON 配置定义 pipeline 行为
- 命令行参数使用 `cmdline`

## 依赖

- 子模块：`deps/ax-video-sdk`
- JSON：`third-party/json/json.hpp`
- cmdline：`third-party/cmdline/cmdline.hpp`

## 构建

本项目构建方式对齐 `ax-video-sdk`：

- 板端：`AX650`、`AX620E(AX630C/AX620Q/AX620QP)`
- AXCL：x86_64 / aarch64

构建脚本与 CI 会自动准备 MSP/AXCL SDK 和 toolchain（后续补齐）。

## 运行

示例配置文件：`configs/example.json`

```bash
./ax_pipeline_app -c configs/example.json -t 20
```

`-t 0` 表示一直运行直到 `Ctrl+C`。

`configs/example.json` 中的 `uri` 默认是占位路径，需要你改成真实的 `mp4` 文件路径或 `rtsp://` 地址。

## 配置格式（简化）

```json
{
  "system": { "device_id": -1 },
  "pipelines": [
    {
      "name": "p0",
      "device_id": -1,
      "uri": "xxx.mp4 或 rtsp://...",
      "realtime_playback": false,
      "loop_playback": false,
      "frame_output": {
        "format": "bgr",
        "width": 640,
        "height": 640,
        "resize": { "mode": "keep_aspect", "background_color": 0 }
      },
      "outputs": [
        { "codec": "h264", "uris": ["/tmp/out.mp4", "rtsp://..."] }
      ],
      "npu_max_fps": 20,
      "log_every_n_frames": 30
    }
  ]
}
```
