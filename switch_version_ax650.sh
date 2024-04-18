#!/bin/sh
echo "\e[32mTips: \n   switch sdk version support for pipeline\e[0m\n"
version=$1

if [ "$version" = "1.27" ]
then
    git checkout 26e89428c848080e249eb50421a41291de709673 examples/common/common_pipeline/ax650

    git checkout 7b8dc8eccda3e897d8f8e4447b5b5256e8d38d29 examples/libaxdl/include/ax_osd_drawer.hpp
    git checkout 7b8dc8eccda3e897d8f8e4447b5b5256e8d38d29 examples/utilities/ax_version_check.cpp
    git checkout 7b8dc8eccda3e897d8f8e4447b5b5256e8d38d29 examples/sample_vin_ivps_npu_hdmi_vo/CMakeLists.txt
    git checkout 7b8dc8eccda3e897d8f8e4447b5b5256e8d38d29 examples/sample_vin_ivps_npu_venc_rtsp/CMakeLists.txt
    git checkout 7b8dc8eccda3e897d8f8e4447b5b5256e8d38d29 examples/common/common_func.h
    git checkout 7b8dc8eccda3e897d8f8e4447b5b5256e8d38d29 examples/common/common_func.c
    git checkout 7b8dc8eccda3e897d8f8e4447b5b5256e8d38d29 cmake/build_func.cmake
elif [ "$version" = "1.40" ]
then
    git checkout 45df52f464c13d1f6adac36787296736f42c6bcc examples/common/common_pipeline/ax650

    git checkout 7b8dc8eccda3e897d8f8e4447b5b5256e8d38d29 examples/libaxdl/include/ax_osd_drawer.hpp
    git checkout 7b8dc8eccda3e897d8f8e4447b5b5256e8d38d29 examples/utilities/ax_version_check.cpp
    git checkout 7b8dc8eccda3e897d8f8e4447b5b5256e8d38d29 examples/sample_vin_ivps_npu_hdmi_vo/CMakeLists.txt
    git checkout 7b8dc8eccda3e897d8f8e4447b5b5256e8d38d29 examples/sample_vin_ivps_npu_venc_rtsp/CMakeLists.txt
    git checkout 7b8dc8eccda3e897d8f8e4447b5b5256e8d38d29 examples/common/common_func.h
    git checkout 7b8dc8eccda3e897d8f8e4447b5b5256e8d38d29 examples/common/common_func.c
    git checkout 7b8dc8eccda3e897d8f8e4447b5b5256e8d38d29 cmake/build_func.cmake
elif [ "$version" = "1.45" ]
then
    git checkout 7b8dc8eccda3e897d8f8e4447b5b5256e8d38d29 examples/common/common_pipeline/ax650

    git checkout 7b8dc8eccda3e897d8f8e4447b5b5256e8d38d29 examples/libaxdl/include/ax_osd_drawer.hpp
    git checkout 7b8dc8eccda3e897d8f8e4447b5b5256e8d38d29 examples/utilities/ax_version_check.cpp
    git checkout 7b8dc8eccda3e897d8f8e4447b5b5256e8d38d29 examples/sample_vin_ivps_npu_hdmi_vo/CMakeLists.txt
    git checkout 7b8dc8eccda3e897d8f8e4447b5b5256e8d38d29 examples/sample_vin_ivps_npu_venc_rtsp/CMakeLists.txt
    git checkout 7b8dc8eccda3e897d8f8e4447b5b5256e8d38d29 examples/common/common_func.h
    git checkout 7b8dc8eccda3e897d8f8e4447b5b5256e8d38d29 examples/common/common_func.c
    git checkout 7b8dc8eccda3e897d8f8e4447b5b5256e8d38d29 cmake/build_func.cmake
elif [ "$version" = "2.0" ]
then
    echo "the newest version is 2.0, no need to switch, if you have switched to other version, please checkout to nearest commit"
    echo "command to check the nearest version,the red print is the nearest commit hash code: \n   "
    echo "\e[34m$ git log\e[0m
    commit \e[31mc88e03865aff4181be26461aaffd4d1d36aae8c9\e[0m (HEAD, osd_depth_distance)
    Author: ZHEQIUSHUI <qiushui.zhe@gmail.com>
    Date:   Thu Oct 12 15:42:55 2023 +0800

    auto download drm

    commit c1e5d7835300b0226456c8da5b8408fc2fe4f424 (ax/main)
    Author: ZHEQIUSHUI <46700201+ZHEQIUSHUI@users.noreply.github.com>
    Date:   Tue Oct 10 16:54:29 2023 +0800"
    echo "\e[34m$ git checkout \e[31mc88e03865aff4181be26461aaffd4d1d36aae8c9\e[34m examples/common/common_pipeline/ax650\n$ git checkout \e[31mc88e03865aff4181be26461aaffd4d1d36aae8c9\e[34m examples/libaxdl/include/ax_osd_drawer.hpp\n...\e[0m
    Updated 2 paths from 6c5a2ef"
else
    echo "supported versions: \n   \e[34m[1.27, 1.40, 1.45, 2.0]\e[0m\n"
    echo "for example: \n   \e[34m$ ./switch_version_ax650.sh 1.27\e[0m"
fi