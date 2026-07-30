# ax_plugin_yolov5 (YOLOv5 detector)

Generic YOLOv5 object detection plugin for `ax-pipeline` (anchor-based head).

Unlike the `yolo11`/`yolo26` presets, this plugin is **configurable**: strides,
anchors and class count can all be set from the config so it fits any
YOLOv5-style `.axmodel`. Defaults target COCO YOLOv5s.

- Model source (download `.axmodel`): `https://huggingface.co/AXERA-TECH/YOLOv5`
- This repository **does not** ship the model file (`models/` is git-ignored).

## Model I/O (probed on AX650N, yolov5s)

- Input: `images` `[1, 640, 640, 3]`, **NHWC**, U8.
- Output: 3 tensors, **NHWC** `[1, H, W, 255]` (`255 = 3 anchors * (num_classes(80) + 5)`):
  - `[1, 80, 80, 255]` (stride 8), `[1, 40, 40, 255]` (stride 16), `[1, 20, 20, 255]` (stride 32).

## Config example

```json
{
  "enable": true,
  "ax_plugin_path": "lib/plugins/libax_plugin_yolov5.so",
  "ax_plugin_isolation": "inproc",
  "ax_plugin_init_info": {
    "model_path": "/path/to/yolov5s.axmodel",
    "num_classes": 80,
    "conf_threshold": 0.25,
    "nms_threshold": 0.45,
    "strides": [8, 16, 32],
    "yolov5_anchors": [
      10,13, 16,30, 33,23,
      30,61, 62,45, 59,119,
      116,90, 156,198, 373,326
    ]
  }
}
```

For a standard COCO YOLOv5s only `model_path` is strictly required — the COCO
strides/anchors above are the defaults.

## Plugin init options

- `model_path` (string, required): path to `.axmodel`.
- `device_id` (int, optional): overrides the device id passed from the app.
- `npu_affinity` (int|string, optional): affinity mask (`0b001/0b010/0b100`) or `"rr"`. See `docs/config.md`.
- `num_classes` (int, default `80`)
- `conf_threshold` (number, default `0.25`), `nms_threshold` (number, default `0.45`)
- `pre_nms_topk` (int, default `1000`), `max_det` (int, default `300`), `class_agnostic_nms` (bool, default `false`)
- `strides` (int array, default `[8,16,32]`)
- `yolov5_anchors` (number array, 3 pairs per stride, pixel-space; default = COCO anchors)
- `resize_mode` / `horizontal_align` / `vertical_align` / `background_color`: preprocess (default letterbox, centered, black pad).

Optional tracking (plugin-side ByteTrack): `enable_tracking`, `track_fps`, `track_buffer`, `track_min_score`.

## Validation (COCO bus.jpg, AX650N)

`yolov5s.axmodel` on the classic `bus.jpg`:

| class | score |
|---|---:|
| person | 0.84 |
| person | 0.83 |
| bus | 0.79 |
| person | 0.74 |
| person | 0.54 |

→ 1 bus + 4 people, all boxes correctly placed.

## Speed

Pure NPU inference latency on **AX650N**, measured with `ax_run_model`
(VNPU Disable = full 3-core; 100 runs; excludes pre/post-processing and host I/O):

| Model | Input | on-chip ms | on-chip FPS |
|---|---|---:|---:|
| yolov5s | 640×640 | 6.28 | 159 |

(Over an AXCL PCIe card the same model measures ~35 ms end-to-end via `axengine`,
dominated by the host↔device round-trip — not representative of on-chip compute.)
