# 源码编译（AX620A）

ax-samples 的源码编译目前有两种实现路径：

- **本地编译**：由于开发板集成了完整的Linux系统，可以预装必要的 gcc、cmake 等开发环境，因此可以在开发板上直接完成源码编译；
- **交叉编译**：在 x86 PC 的常规开发环境中，通过对应的交叉编译工具链完成对源码的编译。

## 本地编译

### 硬件需求

- 爱芯派（基于 AX620A）

### 编译环境

- cmake 版本大于等于 3.13
- AX620A/U 配套的交叉编译工具链 `arm-linux-gnueabihf-gxx` 已添加到环境变量中

### 编译过程

- 新手用户请确保以下每一步的所有命令都成功运行无误再进行下一步命令
- 如果单独某一部分的命令出错，请及时在技术交流 QQ 群反馈，QQ 群:139953715
- 反馈时请清楚的描述遇到的问题以及错误

1、git clone 下载源码，进入 ax-pipeline 根目录

```shell
git clone https://github.com/AXERA-TECH/ax-pipeline.git
cd ax-pipeline
```

2、下载子模块

```shell
git submodule update --init
./download_ax_bsp.sh ax620
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
