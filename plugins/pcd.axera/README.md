# ax_plugin_pcd (Person/Car Detector)

This plugin wraps the **Person_car-axera** detector model as an `ax-pipeline` NPU plugin.

- Model source (download `.axmodel` + demo assets): `https://huggingface.co/AXERA-TECH/Person_car-axera`
- This repository **does not** ship the model file.

## Config example

Add the following into your `pipelines[i].npu` section:

```json
{
  "enable": true,
  "ax_plugin_path": "lib/plugins/libax_plugin_pcd.so",
  "ax_plugin_isolation": "inproc",
  "ax_plugin_init_info": {
    "model_path": "/path/to/Person_car-axera.axmodel",
    "num_classes": 3,
    "conf_threshold": 0.25,
    "nms_threshold": 0.45,
    "max_det": 50,
    "strides": [16, 32],
    "resize_mode": "keep_aspect",
    "horizontal_align": "center",
    "vertical_align": "center",
    "background_color": 0
  }
}
```

## Plugin init options

- `model_path` (string, required): path to `.axmodel`.
- `device_id` (int, optional): overrides the device id passed from the app.
- `npu_affinity` (int|string, optional): affinity mask (`0b001/0b010/0b100`) or `"rr"` (round-robin).
- `num_classes` (int, default `3`)
- `conf_threshold` (number, default `0.25`): anchor score threshold (before NMS).
- `nms_threshold` (number, default `0.45`): IoU threshold for NMS.
- `max_det` (int, default `50`): keep top-K candidates before NMS.
- `strides` (int array, default `[16, 32]`)

Optional tracking (plugin-side ByteTrack, same behavior as `yolov5/yolov8` plugins):
- `enable_tracking` (bool, default `false`)
- `track_fps` (int, default `30`)
- `track_buffer` (int, default `30`)
- `track_min_score` (number, default `0.0`)

## Benchmark (10x 1080p decode + encode + NPU)

Tested with `ax_pipeline_app -t 60` (MP4 input in `realtime_playback=true`), 10 pipelines, each pipeline:

- Input: 1920x1080@30 HEVC
- Output: H.265 encode + RTSP publish (no client required)
- NPU: `ax_plugin_pcd`, OSD/tracking disabled, `npu_max_fps=0` (unlimited)
- Model: NHWC RGB input (`ax_ax650_pcd_tiny_algo_rgb_nhwc_V2.0.0.axmodel`)

FPS calculation:

- Decode FPS / channel: `decoded / 60`
- NPU FPS / channel: `npu_ok / 60`

| Platform | Channels | Decode FPS / ch (avg) | NPU FPS / ch (avg / min / max) |
|---|---:|---:|---:|
| AXCL (x86_64) | 10 | 29.63 | 29.63 / 29.55 / 29.72 |
| AX650 (MSP) | 10 | 29.63 | 29.63 / 29.57 / 29.67 |
