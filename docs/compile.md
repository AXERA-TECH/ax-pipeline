# 源码编译（Linux）

ax-pipeline 的源码编译目前有两种实现路径：

- 基于 AX-Pi 的本地编译，因为 AX-Pi 上集成的完成了软件开发环境，操作简单；
- 嵌入式 Linux 交叉编译。

## 本地编译

### 硬件需求

- AX-Pi（基于 AX620A，面向社区开发者的高性价比开发板）

### 编译环境
- cmake 版本大于等于 3.13
- AX620A/U 配套的交叉编译工具链 `arm-linux-gnueabihf-gxx` 已添加到环境变量中

### 编译过程
- 新手用户请确保以下每一步的所有命令都成功运行无误再进行下一步命令
- 如果单独某一部分的命令出错，请及时在技术交流 QQ 群反馈，QQ 群:139953715
- 反馈时请清楚的描述遇到的问题以及错误

#### AX620A/U
1、git clone 下载源码，进入 ax-pipeline 根目录

```shell
git clone https://github.com/AXERA-TECH/ax-pipeline.git
cd ax-pipeline
```
2、下载子模块（主要是 [axpi_bsp_sdk](https://github.com/sipeed/axpi_bsp_sdk) 部分，如果已经单独下载，可直接放到本目录下，并跳过本步骤）
```shell
git submodule update --init
```
3、创建 3rdparty，下载opencv
```shell
mkdir 3rdparty
cd 3rdparty
wget https://github.com/AXERA-TECH/ax-samples/releases/download/v0.1/opencv-arm-linux-gnueabihf-gcc-7.5.0.zip
apt install unzip -y
unzip opencv-arm-linux-gnueabihf-gcc-7.5.0.zip
```
4、下载并配置交叉编译工具链（如果已经配置并确定可用，这一部分可以跳过）
```shell
wget http://releases.linaro.org/components/toolchain/binaries/7.5-2019.12/arm-linux-gnueabihf/gcc-linaro-7.5.0-2019.12-x86_64_arm-linux-gnueabihf.tar.xz
tar -xvf gcc-linaro-7.5.0-2019.12-x86_64_arm-linux-gnueabihf.tar.xz
export PATH=$PATH:$PWD/gcc-linaro-7.5.0-2019.12-x86_64_arm-linux-gnueabihf/bin/
```
5、创建 build 目录，并创建 cmake 编译任务
```shell
cd ..
mkdir build
cd build
cmake -DAXERA_TARGET_CHIP=AX620 -DSIPY_BUILD=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=../toolchains/arm-linux-gnueabihf.toolchain.cmake -DCMAKE_INSTALL_PREFIX=install ..
make $(expr `nproc` - 1)
make install
```

6、编译完成后，生成的可执行示例存放在 `ax-pipeline/build/install/bin/` 路径下：

```shell
ax-pipeline/build$ tree install
install/
├── bin
│   ├── config
│   │   ├── ax_bvc_det.json
│   │   ├── ax_person_det.json
│   │   ├── ax_pose.json
│   │   ├── ax_pose_yolov5s.json
│   │   ├── ax_pose_yolov8.json
│   │   ├── crowdcount.json
│   │   ├── hand_pose.json
│   │   ├── hand_pose_yolov7_palm.json
│   │   ├── hrnet_animal_pose.json
│   │   ├── hrnet_pose.json
│   │   ├── hrnet_pose_ax_det.json
│   │   ├── hrnet_pose_yolov8.json
│   │   ├── license_plate_recognition.json
│   │   ├── nanodet.json
│   │   ├── palm_hand_detection.json
│   │   ├── pp_human_seg.json
│   │   ├── scrfd.json
│   │   ├── yolo_fastbody.json
│   │   ├── yolopv2.json
│   │   ├── yolov5_seg.json
│   │   ├── yolov5s.json
│   │   ├── yolov5s_face.json
│   │   ├── yolov5s_face_recognition.json
│   │   ├── yolov5s_license_plate.json
│   │   ├── yolov6.json
│   │   ├── yolov7.json
│   │   ├── yolov7_face.json
│   │   ├── yolov7_palm_hand.json
│   │   ├── yolov8.json
│   │   ├── yolov8_plate.json
│   │   ├── yolov8_pose.json
│   │   ├── yolov8_seg.json
│   │   └── yolox.json
│   ├── sample_demux_ivps_joint_rtsp
│   ├── sample_demux_ivps_joint_rtsp_vo
│   ├── sample_demux_ivps_joint_vo
│   ├── sample_multi_demux_ivps_joint_multi_rtsp
│   ├── sample_v4l2_ivps_joint_vo
│   ├── sample_v4l2_user_ivps_joint_vo
│   ├── sample_vin_ivps_joint_venc_rtsp
│   ├── sample_vin_ivps_joint_venc_rtsp_vo
│   ├── sample_vin_ivps_joint_vo
│   └── sample_vin_ivps_joint_vo_h265
```

#### AX650
1、git clone 下载源码，进入 ax-pipeline 根目录

```shell
git clone https://github.com/AXERA-TECH/ax-pipeline.git
cd ax-pipeline
```
2、子模块（[axpi_bsp_sdk](https://github.com/sipeed/axpi_bsp_sdk) 不适用于AX650，后续可能会开源适用于 AX650 的 BSP，目前 BSP 仅可通过 FAE 获取）
```shell
git submodule update --init
```
3、创建 3rdparty，下载opencv(暂时需要用户自己编译对应平台的opencv)
```shell

```
4、下载并配置交叉编译工具链（如果已经配置并确定可用，这一部分可以跳过）
```shell
wget https://developer.arm.com/-/media/Files/downloads/gnu-a/9.2-2019.12/binrel/gcc-arm-9.2-2019.12-x86_64-aarch64-none-linux-gnu.tar.xz
tar -xvf gcc-arm-9.2-2019.12-x86_64-aarch64-none-linux-gnu.tar.xz
export PATH=$PATH:$PWD/gcc-arm-9.2-2019.12-x86_64-aarch64-none-linux-gnu/bin/
```
5、创建 build 目录，并创建 cmake 编译任务
```shell
cd ..
mkdir build
cd build
cmake -DAXERA_TARGET_CHIP=AX650 -DSIPY_BUILD=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=../toolchains/aarch64-none-linux-gnu.toolchain.cmake -DCMAKE_INSTALL_PREFIX=install ..
make $(expr `nproc` - 1)
make install
```

6、编译完成后，生成的可执行示例存放在 `ax-pipeline/build/install/bin/` 路径下：

```shell
ax-pipeline/build$ tree install
install
├── bin
│   ├── config
│   │   ├── dinov2.json
│   │   ├── scrfd.json
│   │   ├── yolov5_seg.json
│   │   ├── yolov5s_650.json
│   │   ├── yolov5s_face.json
│   │   ├── yolov6.json
│   │   ├── yolov7.json
│   │   ├── yolov7_face.json
│   │   ├── yolov8_pose.json
│   │   └── yolox.json
│   ├── sample_demux_ivps_joint_hdmi_vo
│   ├── sample_demux_ivps_joint_rtsp
│   ├── sample_demux_ivps_joint_rtsp_hdmi_vo
│   ├── sample_multi_demux_ivps_joint_hdmi_vo
│   ├── sample_multi_demux_ivps_joint_multi_rtsp
│   └── sample_multi_demux_ivps_joint_multi_rtsp_hdmi_vo
```
