cmake_minimum_required(VERSION 3.16)

# pcd_vlm: 检测(转发给 libax_plugin_pcd.so)+ VLM 旁路。
# 本插件自身不做检测/NPU,只需 dlopen + pthread + json(header-only) + ax_plugin.h。
add_library(ax_plugin_pcd_vlm SHARED
    plugin_pcd_vlm.cpp
)

target_compile_definitions(ax_plugin_pcd_vlm PRIVATE AX_PLUGIN_BUILD_DLL=1)

target_include_directories(ax_plugin_pcd_vlm
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../third-party/json
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_options(ax_plugin_pcd_vlm PRIVATE
    -Wall -Wextra -Wno-unused-parameter
)

find_package(Threads REQUIRED)
# ax_video_sdk 提供 AxImage / ImageProcessor(裁ROI) / EncodeJpegToBase64(硬件JPEG);
# 检测本身由内层 libax_plugin_pcd.so(运行时 dlopen)负责,本插件不 link NPU。
target_link_libraries(ax_plugin_pcd_vlm PRIVATE
    ax_video_sdk ax_video_sdk::ax_msp
    ${CMAKE_DL_LIBS} Threads::Threads)

set_property(GLOBAL APPEND PROPERTY AXP_PLUGIN_TARGETS ax_plugin_pcd_vlm)
