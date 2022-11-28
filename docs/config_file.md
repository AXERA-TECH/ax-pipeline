# 配置文件说明
注意，配置文件中的节点，代码中会进行判断是否存在，如果不存在，则不会更新对应的信息。
## 节点说明
### 通用节点
```MODEL_TYPE``` 模型类型：可以是整形数字或字符串，设置值对应见枚举 [SAMPLE_RUN_JOINT_MODEL_TYPE](examples/sample_run_joint/sample_run_joint_post_process.h)，具体映射关系可以看代码中定义的 [std::map<std::string, int> ModelTypeTable](../examples/sample_run_joint/sample_run_joint_post_process.cpp)

```MODEL_PATH``` 模型路径：如果用户在运行时使用了 ```-m``` 以指定模型路径，则后被指定的模型路径会覆盖前设置的模型路径。如果是二级模型的配置文件，则配置文件中，第一层的 ```MODEL_PATH``` 会被第二层的 ```MODEL_PATH``` 覆盖。如下所示
```shell
{
    "MODEL_TYPE": "MT_MLM_HUMAN_POSE",
    "SAMPLE_IVPS_ALGO_WIDTH": 960,
    "SAMPLE_IVPS_ALGO_HEIGHT": 540,
    "MODEL_PATH": "/root/test/install/bin/yolov5s.joint", //第一层节点
    "MODEL_MINOR": {
        "MODEL_PATH": "/root/test/install/bin/hrnet_256x192.joint"
    },
    "MODEL_MAJOR": {
        "MODEL_TYPE": "MT_DET_YOLOV5",
        "MODEL_PATH": "/root/test/install/bin/yolov5s.joint",  //第二层节点，会覆盖第一层的 MODEL_PATH
        "ANCHORS": [
```
### 二级模型节点
```SAMPLE_IVPS_ALGO_WIDTH``` ```SAMPLE_IVPS_ALGO_HEIGHT``` ：二级模型需要设置的值，用作设置输入到算法的图像大小，单独设置分辨率是为了能够有分辨率足够大的图片进行裁切图像，以进行第二级的模型推理。如果是一级模型，则直接通过管道直接输出模型对应大小的图像。

```MODEL_MAJOR```：二级模型中的第一级模型，一般是检测模型，例如 [hrnet_pose.json](../examples/sample_run_joint/config/hrnet_pose.json) 中，此节点跟 [yolov5s.json](../examples/sample_run_joint/config/yolov5s.json) 内容是几乎一样的。

```MODEL_MAJOR```：二级模型中的第二级模型，设置一些第二级模型需要的参数。比如现在命令行参数中去设置第二级模型的路径非常不优雅，所以 [hrnet_pose.json](../examples/sample_run_joint/config/hrnet_pose.json) 中，此节点设置了第二级模型的路径。

### 检测模型节点
```ANCHORS```：常见于基于锚点的目标检测，如果是 free-anchor 的模型，则可以不设置，例如 [yolox.json](../examples/sample_run_joint/config/yolox.json)

```CLASS_NUM```：检测的类别，一些目标检测的模型需要此参数进行后处理。

```CLASS_NAMES```：数量必须等于```CLASS_NUM```

```NMS_THRESHOLD``` ```PROB_THRESHOLD```：略