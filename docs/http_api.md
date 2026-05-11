# HTTP API（动态编辑 pipelines）

`ax_pipeline_app` 支持可选的 HTTP 控制面，用于在程序运行期间动态增删/编辑 pipeline，并获取预览图。

注意：**动态编辑存在稳定性风险**（涉及编解码/RTSP/NPU 等模块的 Stop/Close/Open/Start），建议在你的目标平台/驱动版本上做充分压测后再用于生产。

## 启动

```bash
./ax_pipeline_app -c configs/example.json -t 0 --http_port=25000 --http_addr=127.0.0.1
```

可选鉴权（Bearer Token）：

```bash
./ax_pipeline_app -c configs/example.json -t 0 --http_port=25000 --http_token=YOUR_TOKEN
```

此时请求需带 header：

```bash
Authorization: Bearer YOUR_TOKEN
```

## API 列表（v1）

- `GET /api/v1/health`
- `GET /api/v1/pipelines`
- `GET /api/v1/pipelines/:name`
- `POST /api/v1/pipelines`（新增）
- `DELETE /api/v1/pipelines/:name`（删除）
- `POST /api/v1/pipelines/:name/start`
- `POST /api/v1/pipelines/:name/stop`
- `PUT /api/v1/pipelines/:name`（替换整条 pipeline 配置）
- `PUT /api/v1/pipelines/:name/npu`（仅更新 NPU/plugin 相关配置）
- `POST /api/v1/pipelines/:name/outputs`（动态新增编码输出）
- `DELETE /api/v1/pipelines/:name/outputs/:index`（动态删除编码输出）
- `GET /api/v1/pipelines/:name/preview.jpg`（预览 JPEG）

## 关键语义 / 边界

- `PUT /api/v1/pipelines/:name` 会执行 **Stop -> Close -> Open -> (可选 Start)** 的“重开”流程：
  - 期间会丢帧/中断 RTSP 输出（属于预期）
  - 若新配置 `Open` 失败，会 best-effort 回滚到旧配置并尝试恢复旧的 running 状态
- `PUT /api/v1/pipelines/:name/npu` 默认 **不重开解码/编码**，只会重建 NPU worker 与回调
  - 若你同时修改了 `frame_output`（会改变推理输入空间），建议走整条 pipeline 的 `PUT /pipelines/:name`
- `POST /pipelines/:name/outputs` / `DELETE /pipelines/:name/outputs/:index` 会在不重开 demux/vdec 的情况下，
  动态创建/销毁 `VENC + mux` 分支
  - pipeline 运行中新增输出时，可能会在新增瞬间产生短暂的码流中断/丢帧（属于预期）
  - 需要系统初始化时已启用 `system.enable_venc=true`（否则无法在运行中“凭空启用”VENC 模块）
- 预览图 `preview.jpg` 是 **静态快照**（基于 `GetLatestFrame()`），适合低频展示；不建议高频轮询做视频播放。
  - 预览 JPEG 需要系统初始化时启用 `system.enable_venc=true`（用于 JPEG 编码）以及 `system.enable_ivps=true`（用于缩放/格式转换/画框）

## 示例

### 1) 列出 pipelines

```bash
curl -s http://127.0.0.1:25000/api/v1/pipelines
```

### 2) 新增 pipeline

```bash
curl -s -X POST http://127.0.0.1:25000/api/v1/pipelines \\
  -H 'Content-Type: application/json' \\
  -d '{
    "autostart": true,
    "pipeline": {
      "name": "p1",
      "device_id": 0,
      "uri": "rtsp://...",
      "realtime_playback": true,
      "loop_playback": false,
      "outputs": [],
      "npu": { "enable": false }
    }
  }'
```

### 3) 获取预览图

```bash
curl -o preview.jpg "http://127.0.0.1:25000/api/v1/pipelines/p1/preview.jpg?quality=85&max_w=640&max_h=360&with_boxes=1"
```

### 4) 动态新增/删除编码输出

新增一个输出（例如写 mp4 文件或推 RTSP）：

```bash
curl -s -X POST http://127.0.0.1:25000/api/v1/pipelines/p1/outputs \\
  -H 'Content-Type: application/json' \\
  -d '{
    "output": {
      "codec": "h264",
      "width": 1920,
      "height": 1080,
      "uris": ["out.mp4"]
    }
  }'
```

删除输出（index 从 `GET /pipelines/:name` 的 `outputs[]` 顺序对应）：

```bash
curl -s -X DELETE http://127.0.0.1:25000/api/v1/pipelines/p1/outputs/0
```
