# 如何更换自己训练的 yolov5 模型？

### 如何将自己训练的模型转换成 AX620 所用的 Joint 模型
- 阅读知乎文章 [爱芯元智AX620A部署yolov5 6.0模型实录](https://zhuanlan.zhihu.com/p/569083585)
- 根据上述文章，搭建模型转换的环境，修改模型结构
- 量化数据集请自行下载 coco dataset，从中随机提取一千张图片打包成 tar 文件，或者从训练的测试集抽一千张图片进行打包

最终得到的配置文件（可以直接复制用作 yolov5 的模型转换）
```
# yolov5s.prototxt

# 基本配置参数：输入输出
input_type: INPUT_TYPE_ONNX

output_type: OUTPUT_TYPE_JOINT

# 选择硬件平台
target_hardware: TARGET_HARDWARE_AX620

# CPU 后端选择，默认采用 AXE
cpu_backend_settings {
    onnx_setting {
        mode: DISABLED
    }
    axe_setting {
        mode: ENABLED
        axe_param {
            optimize_slim_model: true
        }
    }
}

# input
src_input_tensors {
    color_space: TENSOR_COLOR_SPACE_RGB
}

# 可选 TENSOR_COLOR_SPACE_NV12 / TENSOR_COLOR_SPACE_RGB
dst_input_tensors {
    color_space: TENSOR_COLOR_SPACE_NV12
}

# neuwizard 工具的配置参数
neuwizard_conf {
    operator_conf {
        input_conf_items {
            attributes {
                input_modifications {
                    affine_preprocess {
                        slope: 1
                        slope_divisor: 255
                        bias: 0
                    }
                }
                # input_modifications {
                #     input_normalization {
                #         mean: [0,0,0]
                #         std: [0.5,0.5,0.5]
                #     }
                # }
            }
        }
    }
    dataset_conf_calibration {
        path: "/data/dataset/coco_1000.tar" # 数据集图片的 tar 包，用于编译过程中对模型校准
        type: DATASET_TYPE_TAR         # 数据集类型：tar 包
        size: 256                      # 编译过程中校准所需要的实际数据个数为 256
    }
    dataset_conf_error_measurement {
        path: "/data/dataset/coco_1000.tar" # 用于编译过程中对分
        type: DATASET_TYPE_TAR
        size: 4                        # 对分过程所需实际数据个数为 4
        batch_size: 1
    }
    
}

dst_output_tensors {
    tensor_layout:NHWC
}

# pulsar compiler 的配置参数
pulsar_conf {
    ax620_virtual_npu: AX620_VIRTUAL_NPU_MODE_111
    batch_size: 1
    debug : false
}
```

### 修改 yolov5 的推理参数
默认的 [参数文件](../examples/sample_run_joint/config/yolov5s.json) 适配的是 yolov5 官方的预训练模型，如果需要修改类别数、类别名、ANCHORS、检测阈值、NMS阈值等参数，请复制该参数文件，并修改自定义的类别数、类别名、ANCHORS、检测阈值、NMS阈值等参数。