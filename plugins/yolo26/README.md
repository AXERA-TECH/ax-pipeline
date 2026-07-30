# ax_plugin_yolo26 (YOLO26 detector)

YOLO26 object detection plugin for `ax-pipeline`.

YOLO26 is **anchor-free, DFL-free and NMS-free** (one-to-one head). The AXERA
export emits two NHWC tensors per stride: `cls[H,W,num_classes]` and
`box[H,W,4]` (raw `l,t,r,b` distances in grid units — no DFL). Decoded by the
`AxModelYolo26` head added in this repo. This plugin is a **foolproof preset**:
strides `{8,16,32}` and letterbox preprocess are fixed; you only set `model_path`
(and `num_classes` for custom-trained models).

- Model source (download `.axmodel`): `https://huggingface.co/AXERA-TECH/yolo26`
- This repository **does not** ship the model file.

## Model I/O (probed on AX650N, yolo26n)

- Input: `images` `[1, 640, 640, 3]`, **NHWC**, BGR, U8.
- Output: 6 tensors, **NHWC**, split cls/box per stride:
  - stride 8:  `box [1, 80, 80, 4]`,  `cls [1, 80, 80, 80]`
  - stride 16: `box [1, 40, 40, 4]`,  `cls [1, 40, 40, 80]`
  - stride 32: `box [1, 20, 20, 4]`,  `cls [1, 20, 20, 80]`

Decode: `score = sigmoid(max class logit)`; box distances `d` are multiplied by
stride: `x0 = (w+0.5 - l)*stride`, `y0 = (h+0.5 - t)*stride`, etc.

## Config example

```json
{
  "enable": true,
  "ax_plugin_path": "lib/plugins/libax_plugin_yolo26.so",
  "ax_plugin_isolation": "inproc",
  "ax_plugin_init_info": {
    "model_path": "/path/to/yolo26n.axmodel"
  }
}
```

COCO defaults (`num_classes=80`, `conf=0.25`, `nms=0.45`) are applied automatically.

## Plugin init options

- `model_path` (string, required): path to `.axmodel`.
- `device_id` (int, optional): overrides the device id passed from the app.
- `npu_affinity` (int|string, optional): affinity mask (`0b001/0b010/0b100`) or `"rr"` (round-robin). See `docs/config.md`.
- `num_classes` (int, default `80`): set for custom-trained models.
- `conf_threshold` (number, default `0.25`)
- `nms_threshold` (number, default `0.45`): the head is NMS-free, so NMS is only a safety net. Set `>= 1.0` to disable it entirely.
- `pre_nms_topk` (int, default `1000`), `max_det` (int, default `300`), `class_agnostic_nms` (bool, default `false`).
- `resize_mode` / `horizontal_align` / `vertical_align` / `background_color`: preprocess (default letterbox, centered, black pad).

Head spec (`strides`) is intentionally not user-tunable.

Optional tracking (plugin-side ByteTrack, same as `yolov5/yolov8`):
- `enable_tracking` (bool, default `false`), `track_fps`, `track_buffer`, `track_min_score`.

## Validation (COCO bus.jpg, AX650N)

`yolo26n.axmodel` on `bus.jpg`. The C++ `AxModelYolo26` decoder was verified
value-for-value against a reference Python decode on the real model outputs:

| class | score | box (source px) |
|---|---:|---|
| person | 0.94 | [51, 399, 237, 904] |
| bus | 0.86 | [6, 233, 801, 749] |
| person | 0.86 | [227, 406, 345, 861] |
| person | 0.86 | [670, 392, 810, 876] |
| person | 0.35 | [0, 553, 64, 878] |

At `conf=0.30` the head returned **5 raw detections and 5 after NMS** — i.e. no
duplicates, confirming the one-to-one (NMS-free) head.

## Speed

Single-stream inference latency measured via `axengine` on an **AXCL AX650N**
card. AXCL adds a fixed ~13–14 ms PCIe host↔device round-trip per call, so these
numbers are **transfer-bound** — a tiny model like yolo26n is dominated by the
PCIe hop, not compute.

| Model | Input | ms/infer (AXCL) | FPS (AXCL) |
|---|---|---:|---:|
| yolo26n | 640×640 | 15.0 | 67 |

**On-chip** performance (from the model card, AX650N, NPU3 mode, pure NPU):

| Model | ms | FPS |
|---|---:|---:|
| yolo26n | 1.378 | 726 |
| yolo26s | 3.166 | 316 |
| yolo26m | 8.644 | 116 |
| yolo26l | 11.174 | 90 |
| yolo26x | 20.405 | 41 |

Postprocess is minimal: no DFL (box is a direct 4-value regression) and the
one-to-one head yields only a handful of candidates per frame, so decode is
sub-millisecond.
