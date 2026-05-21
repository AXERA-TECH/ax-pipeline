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

