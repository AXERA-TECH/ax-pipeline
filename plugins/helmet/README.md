# ax_plugin_helmet (Helmet / head / e-bike detector)

Helmet-detection plugin for `ax-pipeline`, wrapping the **Helmet-axera** model
(YOLOv5-style head, 4 classes, 2 strides). A foolproof preset: the head spec
(classes, strides, anchors) is fixed, so you only set `model_path`.

- Model source (download `.axmodel`): `https://huggingface.co/AXERA-TECH/Helmet-axera`
- This repository **does not** ship the model file (`models/` is git-ignored).

## Classes

| id | name |
|---:|---|
| 0 | helmet |
| 1 | head |
| 2 | e-bike |
| 3 | bike |

## Model I/O (probed on AX650N, `*_rgb_nhwc` variant)

- Input: `images` `[1, 256, 192, 3]`, **NHWC**, RGB, U8.
- Output: 2 tensors, **NCHW** `[1, 27, H, W]` (`27 = 3 anchors * (num_classes(4) + 5)`):
  - `[1, 27, 32, 24]` (stride 8), `[1, 27, 16, 12]` (stride 16).
- Strides `[8, 16]` only (no stride-32 head). Anchors (pixel-space, 3 per stride):
  - stride 8: `(31,28) (38,32) (60,83)`
  - stride 16: `(84,110) (133,118) (200,113)`

The plugin's decoder is layout-agnostic (handles this NCHW output as well as NHWC).

## Config example

```json
{
  "enable": true,
  "ax_plugin_path": "lib/plugins/libax_plugin_helmet.so",
  "ax_plugin_isolation": "inproc",
  "ax_plugin_init_info": {
    "model_path": "/path/to/ax_ax650_hel_algo_rgb_nhwc_V1.0.0.axmodel"
  }
}
```

## Plugin init options

- `model_path` (string, required): path to `.axmodel`.
- `device_id` (int, optional): overrides the device id passed from the app.
- `npu_affinity` (int|string, optional): affinity mask (`0b001/0b010/0b100`) or `"rr"`. See `docs/config.md`.
- `conf_threshold` (number, default `0.25`), `nms_threshold` (number, default `0.45`)
- `pre_nms_topk` (int, default `1000`), `max_det` (int, default `50`), `class_agnostic_nms` (bool, default `false`)

Head spec (`num_classes=4`, `strides=[8,16]`, anchors) is fixed and not user-tunable.

Optional tracking (plugin-side ByteTrack): `enable_tracking`, `track_fps`, `track_buffer`, `track_min_score`.

## Validation

Decoding shares the `DecodeYolov5One` path used by `ax_plugin_yolov5`, which is
validated end-to-end on AX650N (COCO bus.jpg → 1 bus + 4 people, correct boxes).
The Helmet-axera model outputs the 4 classes above for helmet/head/e-bike/bike
scenes.

## Speed

The head is smaller (256×192, 2 strides) than the 640×640 YOLO models, so both
inference and postprocess are lighter. Measure on your target with
`ax_plugin_init_info.debug_timing = true`.
