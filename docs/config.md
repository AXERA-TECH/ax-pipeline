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

`system.vnpu_mode`（可选，string）：

- 控制虚拟 NPU(VNPU) 初始化模式（不同平台/SDK 支持的取值不同）
- 本项目当前支持的字符串取值：
  - `disable`：关闭 VNPU（所有平台都支持）
  - `std`：标准模式（仅当当前平台 SDK 头文件中存在对应枚举时生效；AXCL 会按 `enable` 处理）
  - `enable`：开启 VNPU（部分 MSP 平台没有 `std` 枚举时会使用该值；AXCL 的标准开启模式）
  - `big_little` / `little_big`：多核调度模式（仅部分平台/SDK 支持）
  - 也支持数字：`0/1/2/3`（分别对应 `disable/enable/big_little/little_big`）
- 不配置/配置无效时的默认行为：
  - AX650/板端（MSP，`AX_ENGINE_Init`）：使用 SDK 默认（优先 `std`，若 SDK 无 `std` 则用 `enable`；若初始化失败会回退到 `disable`）
  - AXCL（`axclrtEngineInit`）：默认使用 `disable`；若指定的模式初始化失败会回退到 `disable`
  - 若填了当前平台/SDK 不支持的值（例如没有对应枚举/不认识的字符串），会忽略该值并走上述默认/回退逻辑

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
  "uri": "xxx.mp4 / xxx.mov 或 rtsp://...",
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
    - 注意:此时 demux 不受节奏限制,编码器跟不上就按 `overflow_policy` 丢帧 ——
      1080p30 单路默认配置实测丢掉 407/998 帧。**离线转码要一帧不丢,把
      `outputs[].input_queue_depth` 加大**(实测 64 即可,998 帧全编、`dropped=0`)。
      别用 `"overflow_policy": "block"`:背压会让编码器一直持有解码帧、把解码池耗尽,
      实测解码停在 ~590 帧不再推进(已知限制)
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
- `qpmap`（可选，object）：QP 图码控。两件事：**按目标码率闭环调 QP**，以及**目标区域差异化量化**
  - `enable`（bool，默认 `false`）：开启后码控从 CBR 切到 ABS QPMAP 模式，
    `bitrate_kbps` 不再直接决定码率，实际码率由下面的 QP 决定
  - `i_qp`（int，默认 38）：I 帧 QP
  - `p_qp`（int，默认 40）：P/B 帧背景 QP
  - `roi_qp_delta`（int，默认 -4）：目标区域相对背景的 QP 偏移，负值更清晰；
    实际为 `clamp(p_qp + roi_qp_delta, 0, 51)`。**它是质量再分配，不是省码率手段**，
    代价见下面的实测；开启闭环后背景封顶时这个 delta 会被自动收窄
  - `target_bitrate_kbps`（int，默认 0）：**码率闭环**。非 0 时不再用固定 QP，而是
    按实测输出码率自动调整背景 QP 去逼近这个目标；`i_qp` / `p_qp` 退化成闭环起点，
    两者的差值保持不变。ROI 始终按 `roi_qp_delta` 相对背景加细，所以"目标清晰、
    背景够用"的结构不变
  - `min_qp` / `max_qp`（int，默认 24 / 46）：闭环允许的背景 QP 范围。下界防止码率
    目标过宽时白花码率，上界兜住画质底线（超过 46 的背景已经明显糊）
  - ROI 来自 NPU 检测框，自动下发到开启了 `qpmap` 的分支（需要 `npu.enable`）；
    输出被缩放时 ROI 坐标会自动换算
  - 仅 AX650 / AXCL 支持；AX620E 系列会打印告警并回退 CBR（回退时 `target_bitrate_kbps`
    会作为 CBR 码率生效）

  ```json
  { "codec": "h264", "uris": ["/tmp/out.mp4"],
    "qpmap": { "enable": true, "i_qp": 38, "p_qp": 40, "roi_qp_delta": -10 } }
  ```

  **差异化 QP 的真实代价(务必先读)**。把码率目标固定、只改 `roi_qp_delta`,
  在 AXCL 上做了四组同码率对比(1080p,ffmpeg PSNR 按帧序配对,画面切 3x3 分块):

  | 场景 | 配置 | 码率 | 目标最密集区 | 其余区块 |
  | --- | --- | --- | --- | --- |
  | 静态背景(行人街道,998 帧) | 运动 ROI,delta -10 vs 0 | +4.6% | −0.65 dB | −0.29 ~ −1.52 dB |
  | 繁忙路口(1781 帧) | 检测框 ROI,delta -10 vs 0 | +5.7% | −0.03 dB | −0.16 ~ −1.46 dB |
  | 繁忙路口,极低码率 | 背景 QP51 / ROI QP41 vs 均匀 QP45 | −7% | −0.01 dB | −0.17 ~ −2.44 dB |
  | 繁忙路口 | `max_coverage_percent` 57%→20% | −2% | — | — |

  也就是说:**同码率下差异化 QP 在 PSNR 意义上全面不如均匀 QP**,连 ROI 区自己都不赢。
  原因是 H.264/265 的运动补偿已经天然把码率花在变化的地方了 —— 静止背景在 P 帧里
  残差接近 0、本来就几乎免费,提高它的 QP 省不下码率;而给 ROI 降 QP 是实打实地花钱。
  人为再叠一层 QP 差异,只是把码率从编码器的最优分配上挪走。
  同理 `max_coverage_percent` 收紧覆盖面也省不下码率(被裁掉的格子帧差最小、本就不耗码率)。

  所以:**想省码率就用闭环 + `roi_qp_delta: 0`**(见 `compression_ratio`)。
  `roi_qp_delta < 0` 只在"目标区域的保真度比整帧均匀性更重要"时才值得开 ——
  下游是车牌/人脸二次识别,或者人眼只盯目标、背景糊无妨。那种价值 PSNR 测不出来,
  要用下游任务的指标验证,别拿整帧 PSNR 当依据。

  检测是异步的,检测框相对正在编码的帧存在滞后(见 `GET /api/v1/pipelines` 的
  `det_lag`,16 路满载实测均值 2.5 帧、峰值 8 帧),但**不需要按滞后量外扩 ROI**:
  实测把滞后峰值从 1 帧放大到 6 帧,整帧 PSNR 只差 0.013~0.019 dB,只裁车流密集区
  看也只有 0.056 dB。检测框本身远大于帧间位移,QPMAP 的 16px 单元又吸收了亚单元
  位移,而动得快的目标帧差大、必然已被 `motion_roi` 覆盖。

- `compression_ratio`（可选，double，默认 0 = 不启用）：码率目标按**相对输入码流压缩
  多少倍**给，例如 `4.0` = 输出压到输入码率的 1/4。需要 `qpmap.enable`，非 0 时覆盖
  `qpmap.target_bitrate_kbps`，并随实测输入码率滚动更新（RTSP 源的码率本身是变的）。
  部署现场关心的是"能不能省下 3/4 的带宽和存储"，不是 QP 取多少。

  ```json
  { "codec": "h264", "uris": ["rtsp://0.0.0.0:8554/live"],
    "compression_ratio": 4.0,
    "qpmap": { "enable": true, "roi_qp_delta": -10, "min_qp": 26, "max_qp": 44 } }
  ```

  起步第一秒还没有输入码率样本，此时按 `i_qp` / `p_qp` 的固定值编码，之后每秒结一次账。
  闭环按"QP 每 +6 码率减半"换算需要的步长，再限幅 ±2 / 窗口并加 0.6 阻尼 ——
  一个窗口只有 1 秒码流、样本噪声大，走快了画质抖动肉眼可见。码率按**帧数**算而不是
  墙上时钟，所以离线超实时转码和实时推流算出来的是同一个数。实际码率和当前背景 QP
  都在 `GET /api/v1/pipelines` 里（`in_kbps` / `outputs[].out_kbps` / `outputs[].base_qp`），
  控制台 `[stats]` 行同样打印。

  实测（AXCL，`pedestrian_street` 1080p30，输入约 4.6 Mbps，纯运动 ROI）：

  | 要求 | 实得 | 稳态背景 QP | 收敛 |
  | --- | --- | --- | --- |
  | 2× | 2.26× | 34 | 3~4 秒 |
  | 4× | 4.28× | 40 → 48 | 3~4 秒 |
  | 8× | 7.31× | 48 | 3~4 秒 |

  **和 `roi_qp_delta` 的互动**：闭环只能调背景 QP，背景撞到 `max_qp` 之后还压不到目标时，
  多出来的压力会自动**收窄 `roi_qp_delta`**（向 0 靠），必要时一路退化成均匀 QP ——
  码率目标优先于 ROI 优待。不这么做的后果实测过：繁忙路口 `delta: -10` + 覆盖 40% 时，
  背景封顶在 QP50，码率卡在目标的 1.4 倍下不来，闭环形同失效。
  如果反过来希望"宁可超码率也保住目标清晰"，就别用闭环，用固定 `p_qp` + `roi_qp_delta`。

### `motion_roi`（pipeline 级，配合 `qpmap` 使用）

QPMAP 只保护检测器**认识**的目标。画面里检测器不认识但在动的东西（动物、落物、
推车、晃动的招牌…）会落进背景，被高 QP 糊掉。`motion_roi` 把运动区域也并入 ROI，
最终 `ROI = 检测框 ∪ 运动区域`。

```json
"motion_roi": { "enable": true, "sensitivity": 12, "hold_frames": 15,
                "detect_interval": 3, "max_coverage_percent": 40 }
```

- `enable`（bool，默认 `false`）
- `sensitivity`（int，默认 12）：格子亮度差超过它算运动（0~255），越小越灵敏
- `hold_frames`（int，默认 15）：命中后保持多少帧，防闪烁，同时覆盖目标去向
- `detect_interval`（int，默认 3）：每隔几帧算一次；运动区域变化慢，不必逐帧
- `max_coverage_percent`（int，默认 40，0 = 不限）：运动 ROI 面积占画面的**硬上限**。
  超出时只保留帧差最大的那些格子，其余退回背景 QP。实现是对最终结果直接做预算裁剪
  （取第 N 大的帧差当门槛），立刻生效，不像"自动调灵敏度"那样要等几帧收敛、还会震荡。
  **注意它约束的是面积语义，不是码率**：繁忙路口实测把峰值覆盖从 57% 收到 20%，
  输出只小了 2% —— 被裁掉的格子帧差最小、本来就不怎么耗码率。想控码率用
  `compression_ratio`
- `grid_cols` / `grid_rows`（int，默认 0 = 自动取原图 1/8）
  - **会被钳到不低于 1/8**：IVPS 单次缩放到 1/16 会失败、1/32 会卡死硬件

当前覆盖率在 `GET /api/v1/pipelines` 的 `motion_cover` 里（百分比），控制台
`[stats]` 行也会打印 `motion=NN%`。

原理是把整帧用 IVPS 缩成小灰度网格做帧差，读回只有几十 KB。实测 8 路 1080p
开启后整机 CPU 从 3% 升到 4%（`detect_interval: 3`）。若 IVPS 连续失败会自动
关闭该功能并打印告警，不会静默失效。

实测（AXCL，1080p 路口场景，`p_qp: 40`，`roi_qp_delta: -10`，1781 帧）：

| ROI 来源 | 输出大小 | PSNR |
| --- | --- | --- |
| 无 ROI（全图 QP40） | 16.5 MB | — |
| 仅检测框 | 22.5 MB | 27.94 dB |
| 检测框 + 运动区域 | 30.6 MB | 28.49 dB |

注意这是**繁忙路口**场景，几乎所有内容都在动，所以运动区域覆盖很广、压缩收益被摊薄。
静态背景场景（周界、仓库、停车场、楼道）才是这套方案的主战场 —— 同参数下
`pedestrian_street`（998 帧，行人为主）只要 2.45 KB/帧，而路口要 9.3 KB/帧：

| ROI 来源 | 输出大小 | PSNR |
| --- | --- | --- |
| 无 ROI（全图 QP40） | 2.4 MB | 33.11 dB |
| 仅检测框 | 4.5 MB | 33.41 dB |
| 检测框 + 运动区域 | 5.4 MB | 33.65 dB |

覆盖面可用 `sensitivity`、`hold_frames` 调，上限用 `max_coverage_percent` 兜。

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
