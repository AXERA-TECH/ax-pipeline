# ax_plugin_yolov8 (YOLOv8 detector)

Generic YOLOv8 object detection plugin for `ax-pipeline` (anchor-free DFL head,
"native" concat outputs).

Configurable strides / `reg_max` / class count so it fits any YOLOv8-native
`.axmodel`. Defaults target COCO YOLOv8. (For split cls/reg outputs use
`yolov8_split`; for the fixed YOLO11 preset use `yolo11`.)

- Model source (download `.axmodel`): `https://huggingface.co/AXERA-TECH/YOLOv8`
- This repository **does not** ship the model file (`models/` is git-ignored).

## Model I/O (probed on AX650N)

- Input: `images` `[1, H, W, 3]`, **NHWC**, U8 (e.g. `[1,640,640,3]`, or `[1,544,960,3]`).
- Output: 3 concat tensors, **NHWC** `[1, H, W, 4*reg_max + num_classes]`
  (`= 64 + 80 = 144` for COCO, bbox channels first):
  - yolov8s: `[1,80,80,144]`, `[1,40,40,144]`, `[1,20,20,144]` (640×640).

## Config example

```json
{
  "enable": true,
  "ax_plugin_path": "lib/plugins/libax_plugin_yolov8.so",
  "ax_plugin_isolation": "inproc",
  "ax_plugin_init_info": {
    "model_path": "/path/to/yolov8s.axmodel",
    "num_classes": 80,
    "conf_threshold": 0.25,
    "nms_threshold": 0.45,
    "strides": [8, 16, 32],
    "yolov8_reg_max": 16
  }
}
```

For a standard COCO YOLOv8 only `model_path` is strictly required (defaults:
`num_classes=80`, `strides=[8,16,32]`, `reg_max=16`).

## Plugin init options

- `model_path` (string, required): path to `.axmodel`.
- `device_id` (int, optional): overrides the device id passed from the app.
- `npu_affinity` (int|string, optional): affinity mask (`0b001/0b010/0b100`) or `"rr"`. See `docs/config.md`.
- `num_classes` (int, default `80`)
- `conf_threshold` (number, default `0.25`), `nms_threshold` (number, default `0.45`)
- `pre_nms_topk` (int, default `1000`): top-K by class logit before DFL decode (fast path).
- `max_det` (int, default `300`), `class_agnostic_nms` (bool, default `false`)
- `strides` (int array, default `[8,16,32]`), `yolov8_reg_max` (int, default `16`)
- `resize_mode` / `horizontal_align` / `vertical_align` / `background_color`: preprocess (default letterbox, centered, black pad).

Optional tracking (plugin-side ByteTrack): `enable_tracking`, `track_fps`, `track_buffer`, `track_min_score`.

## Validation (COCO bus.jpg, AX650N)

`yolov8s.axmodel` on `bus.jpg`:

| class | score |
|---|---:|
| person | 0.88 |
| bus | 0.85 |
| person | 0.85 |
| person | 0.85 |
| person | 0.55 |

→ 1 bus + 4 people, all boxes correctly placed.

## Speed

Single-stream inference latency via `axengine` on an **AXCL AX650N** card
(includes a fixed ~13–14 ms PCIe host↔device round-trip; on-chip is far faster).

| Model | Input | ms/infer (AXCL) | FPS (AXCL) |
|---|---|---:|---:|
| yolov8s | 640×640 | 22.4 | 45 |
| yolov8n | 960×544 | 25.1 | 40 |

Postprocess uses a top-K fast path (top-`pre_nms_topk` cells by class logit,
DFL decoded only for those), so per-frame decode is sub-millisecond.
