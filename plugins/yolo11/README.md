# ax_plugin_yolo11 (YOLO11 detector)

YOLO11 object detection plugin for `ax-pipeline`.

YOLO11's detection head is identical to **YOLOv8-native** (DFL: `4*reg_max` box
channels + `num_classes` class channels, concatenated per stride and emitted as
NHWC by the AXERA "cut" export). This plugin is a **foolproof preset**: the head
spec (strides `{8,16,32}`, `reg_max=16`, letterbox preprocess) is fixed, so you
only point it at a model — set `num_classes` only for custom-trained models.

- Model source (download `.axmodel`): `https://huggingface.co/AXERA-TECH/YOLO11`
- This repository **does not** ship the model file.

## Model I/O (probed on AX650N)

- Input: `images` `[1, 640, 640, 3]`, **NHWC**, BGR, U8.
- Output: 3 concat tensors, **NHWC** `[1, H, W, 144]` (`144 = 4*reg_max(64) + num_classes(80)`, bbox first):
  - `/model.23/Concat_output_0`   `[1, 80, 80, 144]` (stride 8)
  - `/model.23/Concat_1_output_0` `[1, 40, 40, 144]` (stride 16)
  - `/model.23/Concat_2_output_0` `[1, 20, 20, 144]` (stride 32)

## Config example

Add the following into your `pipelines[i].npu` section:

```json
{
  "enable": true,
  "ax_plugin_path": "lib/plugins/libax_plugin_yolo11.so",
  "ax_plugin_isolation": "inproc",
  "ax_plugin_init_info": {
    "model_path": "/path/to/yolo11s.axmodel"
  }
}
```

That is all that is required. COCO defaults (`num_classes=80`, `conf=0.25`,
`nms=0.45`) are applied automatically.

## Plugin init options

- `model_path` (string, required): path to `.axmodel`.
- `device_id` (int, optional): overrides the device id passed from the app.
- `npu_affinity` (int|string, optional): affinity mask (`0b001/0b010/0b100`) or `"rr"` (round-robin). See `docs/config.md`.
- `num_classes` (int, default `80`): set for custom-trained models.
- `conf_threshold` (number, default `0.25`)
- `nms_threshold` (number, default `0.45`)
- `pre_nms_topk` (int, default `1000`): keep top-K by class score before decoding DFL + NMS.
- `max_det` (int, default `300`)
- `class_agnostic_nms` (bool, default `false`)
- `resize_mode` / `horizontal_align` / `vertical_align` / `background_color`: preprocess (default letterbox, centered, black pad).

Head spec (`strides`, `reg_max`) is intentionally not user-tunable — use the
`yolov8` plugin if you need to override it.

Optional tracking (plugin-side ByteTrack, same as `yolov5/yolov8`):
- `enable_tracking` (bool, default `false`), `track_fps`, `track_buffer`, `track_min_score`.

## Validation (COCO bus.jpg, AX650N)

`yolo11s.axmodel` on the classic `bus.jpg`, decoded with this plugin's recipe:

| class | score |
|---|---:|
| bus | 0.92 |
| person | 0.91 |
| person | 0.91 |
| person | 0.84 |
| person | 0.56 |

→ 1 bus + 4 people, all boxes correctly placed.

## Speed

Single-stream inference latency measured via `axengine` on an **AXCL AX650N**
card. AXCL adds a fixed ~13–14 ms PCIe host↔device round-trip per call, so these
numbers are **transfer-bound for small models** and are far higher than on-chip
latency (e.g. on an M4N-Dock / M.2 card the same model runs several times faster).

| Model | Input | ms/infer (AXCL) | FPS (AXCL) |
|---|---|---:|---:|
| yolo11s | 640×640 | 21.7 | 46 |

Postprocess uses the DFL top-K fast path in `AxModelYoloV8Native` (top-`pre_nms_topk`
cells by class logit, DFL decoded only for those), so per-frame decode cost is
sub-millisecond for typical scenes.
