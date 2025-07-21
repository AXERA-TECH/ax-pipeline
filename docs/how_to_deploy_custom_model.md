# 如何部署自己的其他模型

### 阅读文档

仔细阅读 
- [AXera Pulsar2 工具链指导手册](https://pulsar2-docs.readthedocs.io/zh-cn/latest/) (AX650N/AX650A/AX630C/AX620E)\
  学习如何将自定义的 onnx 模型，转换成 AX620/AX650/AX620E 中所用的 Joint/Axmodel 模型。

### 插入后处理代码
  
ax-pipeline 的模型推理基本都是比较标准的操作，前处理是不需要用户在代码中配置的。

用户基本只需要关注后处理、OSD 部分即可，ax-pipeline 定义了一个用户可以随意修改的自定义模型类 [ax_model_custom](../examples/libaxdl/src/ax_model_custom.hpp)，当用户需要使用自定义模型的时候，只需要
- 在 [AX620/custom_model.json](../examples/libaxdl/config/ax620/custom_model.json) / [AX650/custom_model.json](../examples/libaxdl/config/ax620/custom_model.json) 中，填充模型文件路径 ```MODEL_PATH```
- 在 [ax_model_custom.hpp](../examples/libaxdl/src/ax_model_custom.hpp) 类别里定义需要的自定义属性
- 在 [ax_model_custom.cpp](../examples/libaxdl/src/ax_model_custom.cpp) 内根据注释实现后处理以及OSD代码
- OSD 部分，可以阅读其他相同类型的模型代码进行参考，或自行实现




OSD 超出图像范围的内容会丢失，请注意绘图时 ```坐标、mask``` 是否超出了图像范围，如下图所示：

![](OSD.png)
