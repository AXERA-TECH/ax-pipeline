# 源码编译（AX650N）

**注意：由于不可抗拒的原因，请务必保证 ax-pipeline、ax650n_bsp_sdk、board_bsp 三者版本一致（例如，当前 爱芯派Pro 的 BSP 版本为 3.6.2），否则将出现各种意想不到的异常问题:（**

ax-samples 的源码编译目前有两种实现路径：

- **本地编译**：由于开发板集成了完整的Linux系统，可以预装必要的 gcc、cmake 等开发环境，因此可以在开发板上直接完成源码编译；
- **交叉编译**：在 x86 PC 的常规开发环境中，通过对应的交叉编译工具链完成对源码的编译。

## 1 本地编译(WIP)

### 1.1 已支持硬件板卡

- 爱芯派Pro（基于 AX650N）

### 1.2 环境搭建

在开发板上安装必要的软件开发环境

```bash
apt update
apt install build-essential libopencv-dev cmake
```

### 1.3 下载源码

git clone 下载源码，进入 ax-pipeline 根目录

```shell
git clone https://github.com/AXERA-TECH/ax-pipeline.git
cd ax-pipeline
```

### 1.4 准备子模块

```shell
git submodule update --init
wget https://github.com/ivanshi1108/assets/releases/download/3.6.2/ax650n_bsp_sdk.tar.gz
tar xzvf ax650n_bsp_sdk.tar.gz && rm ax650n_bsp_sdk.tar.gz
ln -s /soc/lib/ ax650n_bsp_sdk/msp/out/lib
```

### 1.5 源码编译

```shell
mkdir build && cd build
cmake -DAXERA_TARGET_CHIP=AX650 -DBSP_MSP_DIR=$PWD/../ax650n_bsp_sdk/msp/out -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=install ..
make -j$(expr `nproc` - 2)
make install
```

编译完成后，生成的可执行示例存放在 `ax-pipeline/build/install/bin/` 路径下：

```shell
ax-pipeline/build$ tree install
install
|-- bin
|   |-- config
|   |   |-- clip.json
|   |   |-- crowdcount.json
|   |   |-- custom_model.json
|   |   |-- depth_anything.json
|   |   |-- dinov2.json
|   |   |-- dinov2_depth.json
|   |   |-- glpdepth.json
|   |   |-- owlvit.json
|   |   |-- ppyoloe.json
|   |   |-- scrfd.json
|   |   |-- scrfd_recognition.json
|   |   |-- yolo_nas.json
|   |   |-- yolov5_seg.json
|   |   |-- yolov5s.json
|   |   |-- yolov5s_face.json
|   |   |-- yolov5s_face_recognition.json
|   |   |-- yolov6.json
|   |   |-- yolov7.json
|   |   |-- yolov7_face.json
|   |   |-- yolov8.json
|   |   |-- yolov8_pose.json
|   |   |-- yolov8_seg.json
|   |   `-- yolox.json
|   |-- sample_demux_ivps_npu_hdmi_vo
|   |-- sample_demux_ivps_npu_rtsp
|   |-- sample_demux_ivps_npu_rtsp_hdmi_vo
|   |-- sample_multi_demux_ivps_npu_hdmi_vo
|   |-- sample_multi_demux_ivps_npu_multi_rtsp
|   `-- sample_multi_demux_ivps_npu_multi_rtsp_hdmi_vo
```

## 2 交叉编译

### 2.1 下载源码

git clone 下载源码，进入 ax-pipeline 根目录

```shell
git clone https://github.com/AXERA-TECH/ax-pipeline.git
cd ax-pipeline
```

### 2.2 准备子模块

```shell
git submodule update --init
wget https://github.com/ivanshi1108/assets/releases/download/3.6.2/ax650n_bsp_sdk.tar.gz
tar xzvf ax650n_bsp_sdk.tar.gz && rm ax650n_bsp_sdk.tar.gz
cd ax650n_bsp_sdk/msp/out/
wget https://github.com/ivanshi1108/assets/releases/download/3.6.2/lib.tar.gz
tar xzvf lib.tar.gz && rm lib.tar.gz
cd ../../../
cd third-party
wget https://github.com/ivanshi1108/assets/releases/download/3.6.2/drm.tar.gz
tar xzvf drm.tar.gz && rm drm.tar.gz
wget https://github.com/ivanshi1108/assets/releases/download/3.6.2/libexif.tar.gz
tar xzvf libexif.tar.gz && rm libexif.tar.gz
cd ../
```

### 2.3 创建 3rdparty，下载opencv

```shell
mkdir 3rdparty && cd 3rdparty
wget https://github.com/ivanshi1108/assets/releases/download/3.6.2/opencv_4.5.5.tar.gz
tar xzvf opencv_4.5.5.tar.gz && rm opencv_4.5.5.tar.gz
cd ../
```

### 2.4 编译环境

- cmake 版本大于等于 3.13
- 已配置交叉编译工具链，若未配置请参考以下操作

```shell
wget https://developer.arm.com/-/media/Files/downloads/gnu-a/9.2-2019.12/binrel/gcc-arm-9.2-2019.12-x86_64-aarch64-none-linux-gnu.tar.xz
tar -xvf gcc-arm-9.2-2019.12-x86_64-aarch64-none-linux-gnu.tar.xz
export PATH=$PATH:$PWD/gcc-arm-9.2-2019.12-x86_64-aarch64-none-linux-gnu/bin/
```

### 2.5 源码编译

```shell
mkdir build && cd build
cmake -DAXERA_TARGET_CHIP=AX650 -DBSP_MSP_DIR=$PWD/../ax650n_bsp_sdk/msp/out -DOpenCV_DIR=$PWD/../3rdparty/opencv_4.5.5/lib/cmake/opencv4 -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=../toolchains/aarch64-none-linux-gnu.toolchain.cmake -DCMAKE_INSTALL_PREFIX=install ..
make -j$(expr `nproc` - 2)
make install
```

编译完成后，生成的可执行示例存放在 `ax-pipeline/build/install/bin/` 路径下：

```shell
ax-pipeline/build$ tree install
install
|-- bin
|   |-- config
|   |   |-- clip.json
|   |   |-- crowdcount.json
|   |   |-- custom_model.json
|   |   |-- depth_anything.json
|   |   |-- dinov2.json
|   |   |-- dinov2_depth.json
|   |   |-- glpdepth.json
|   |   |-- owlvit.json
|   |   |-- ppyoloe.json
|   |   |-- scrfd.json
|   |   |-- scrfd_recognition.json
|   |   |-- yolo_nas.json
|   |   |-- yolov5_seg.json
|   |   |-- yolov5s.json
|   |   |-- yolov5s_face.json
|   |   |-- yolov5s_face_recognition.json
|   |   |-- yolov6.json
|   |   |-- yolov7.json
|   |   |-- yolov7_face.json
|   |   |-- yolov8.json
|   |   |-- yolov8_pose.json
|   |   |-- yolov8_seg.json
|   |   `-- yolox.json
|   |-- sample_demux_ivps_npu_hdmi_vo
|   |-- sample_demux_ivps_npu_rtsp
|   |-- sample_demux_ivps_npu_rtsp_hdmi_vo
|   |-- sample_multi_demux_ivps_npu_hdmi_vo
|   |-- sample_multi_demux_ivps_npu_multi_rtsp
|   `-- sample_multi_demux_ivps_npu_multi_rtsp_hdmi_vo
```
