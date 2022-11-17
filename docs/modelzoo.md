# 模型支持的列表以及下载地址
后处理是直接扣的 [ax-samples](https://github.com/AXERA-TECH/ax-samples)，后续可能会添加一些开源的车牌识别、人体姿态的多级模型示例代码，不好合到主线的也可能在分支实现（画饼）。

|模型|[枚举值](../examples/sample_run_joint/sample_run_joint_post_process.h)|下载地址|配置文件(有则表示主线已经支持)|备注|
|-|-|-|-|-|
|yolov5|17 ```MT_DET_YOLOV5```|[bgr](https://github.com/AXERA-TECH/ax-models/raw/main/ax620/yolov5s.joint) / [nv12](https://github.com/AXERA-TECH/ax-models/raw/main/ax620/yolov5s-face.joint)|[yolov5s.json](../examples/sample_run_joint/config/yolov5s.json)|[如何更换自己训练的 yolov5 模型](../docs/how_to_deploy_custom_yolov5_model.md)|
|yolov5-face|18 ```MT_DET_YOLOV5_FACE```|[bgr](https://github.com/AXERA-TECH/ax-models/raw/main/ax620/yolov5s.joint) / [nv12](https://github.com/AXERA-TECH/ax-models/raw/main/ax620/yolov5s_face_nv12_11.joint)|[yolov5s_face.json](../examples/sample_run_joint/config/yolov5s_face.json)|[yolov5-face](https://github.com/deepcam-cn/yolov5-face)|
|yolov7|19 ```MT_DET_YOLOV7```|[bgr](https://github.com/AXERA-TECH/ax-models/raw/main/ax620/yolov7-tiny.joint)|[yolov7.json](../examples/sample_run_joint/config/yolov7.json)|-|
|yolox|20 ```MT_DET_YOLOX```|[bgr](https://github.com/AXERA-TECH/ax-models/raw/main/ax620/yolox_s.joint)|[yolox.json](../examples/sample_run_joint/config/yolox.json)|-|
|nanodet|21 ```MT_DET_NANODET```|[bgr](https://github.com/AXERA-TECH/ax-models/raw/main/ax620/nanom.joint)|[nanodet.json](../examples/sample_run_joint/config/nanodet.json)|需要改很多东西，模型转换可能比较困难|
|pp-human-seg|33 ```MT_SEG_PPHUMSEG```|[bgr](https://github.com/AXERA-TECH/ax-models/raw/main/ax620/pp_human_seg_mobile_sim.joint) / [nv12](https://github.com/AXERA-TECH/ax-models/raw/main/ax620/pp_human_seg_mobile_sim_nv12.joint)|-|在 [fork的分支](https://github.com/ZHEQIUSHUI/ax-pipeline/tree/pphumseg) 实现了，但是只在```sample_vin_ivps_joint_vo```实现了，暂时不好合到主线，可能是以后的kpi，未来可期|