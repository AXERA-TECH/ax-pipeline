# 性能压测记录（RTSP 拉流）- 2026-04-23

## 环境

- AXCL 主机：同时充当 RTSP server / 推流端 / 拉流压测端（同机）
- AX650 板端：作为拉流压测端（从 AXCL 主机拉 RTSP）

## RTSP 源（MediaMTX + FFmpeg）

视频源（示例）：

- 1080p / 30fps / HEVC 的 MP4（约 5Mbps）

启动 MediaMTX：

```bash
mediamtx <config.yml>
```

FFmpeg 循环推送 RTSP（HEVC 直拷贝；`$VIDEO` 为本地视频路径）：

```bash
ffmpeg -stream_loop -1 -re -i "$VIDEO" -an -c copy \
  -f rtsp -rtsp_transport tcp rtsp://localhost:8554/ped
```

RTSP URL（客户端拉流）：

- `rtsp://<rtsp-server>:8554/ped`

## 统计口径

`ax_pipeline_app` 每秒打印一次：

- `[stats idx=N] decoded=...`：累计解码帧数（每路）

将相邻两秒的 `decoded` 做差得到 `delta`，近似为该路的 FPS（每秒新增解码帧数）。

本记录的汇总指标：

- 每路丢弃前/后各 3 个 `delta` 采样（去掉起停抖动）
- 忽略负 `delta`（个别路在关闭/重连时计数会重置）
- 统计所有路所有采样的 `avg / p5 / min`

注意：日志中偶尔会出现 `RtspClient close ...` 插入到 stdout 行中导致一行被破坏；汇总时只解析以 `"[stats idx="` 开头的完整行。

## 结果（AX650 板端）

### 仅解码（无 outputs）

汇总（decoded delta）：

| 路数 | avg | p5 | min |
|---:|---:|---:|---:|
| 32 | 31.22 | 30.0 | 28 |
| 34 | 31.49 | 29.0 | 27 |
| 36 | 31.64 | 27.0 | 25 |
| 38 | 31.38 | 30.0 | 26 |
| 40 | 29.76 | 29.0 | 28 |
| 48 | 24.83 | 24.0 | 23 |

结论（按“基本 30fps、不明显掉速”的口径）：约 **38 路**；40 路开始边缘，48 路明显掉速。

### 解码 + 编码（每路 1080p H264；输出写入空 sink，不落盘）

汇总（decoded/out0.submitted delta）：

| 路数 | decoded avg | decoded p5 | decoded min |
|---:|---:|---:|---:|
| 16 | 32.05 | 27.0 | 26 |
| 17 | 30.37 | 28.0 | 27 |
| 18 | 28.88 | 27.0 | 25 |

结论：约 **17 路**；18 路开始明显掉速。

## 结果（本机 AXCL）

### 仅解码（无 outputs）

汇总（decoded delta）：

| 路数 | avg | p5 | min |
|---:|---:|---:|---:|
| 32 | 33.30 | 29.0 | 29 |
| 36 | 32.47 | 29.6 | 28 |
| 38 | 30.79 | 30.0 | 28 |
| 40 | 29.22 | 28.0 | 28 |
| 42 | 27.85 | 27.0 | 26 |

结论：约 **38 路**；40 路开始边缘，42 路明显掉速。

### 解码 + 编码（每路 1080p H264；输出写入空 sink，不落盘）

汇总（decoded/out0.submitted delta）：

| 路数 | decoded avg | decoded p5 | decoded min |
|---:|---:|---:|---:|
| 16 | 31.39 | 27.0 | 23 |
| 17 | 25.77 | 21.0 | 19 |

备注：本机同时承担 `mediamtx + ffmpeg 推流 + ax_pipeline_app 拉流`，17 路时更容易被 RTSP 端或网络栈拖慢；若需要严谨验证 AXCL 的 VENC 上限，建议：

- 用本地文件做输入源（避免 RTSP server 成为瓶颈），或
- 把 RTSP server/推流端搬到另一台机器上，或用多路源分摊推流压力。

## “无 outputs + AI 插件”验证

- 现象：日志里能看到周期性的 `[npu pipeline=...] dets=...`，说明 **无 outputs** 时仍可正常走 `frame_output -> NPU` 回调链路
- 注意：因为没有编码输出，`npu.enable_osd=true` 也不会产生可播放的带框视频流（OSD 只会作用在编码路径）
