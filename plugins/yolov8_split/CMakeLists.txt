cmake_minimum_required(VERSION 3.16)

add_library(ax_plugin_yolov8_split SHARED
    plugin_yolov8_split.cpp
)

target_compile_definitions(ax_plugin_yolov8_split PRIVATE AX_PLUGIN_BUILD_DLL=1)

target_include_directories(ax_plugin_yolov8_split
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../third-party/json
        ${CMAKE_CURRENT_SOURCE_DIR}/../common/npu/include
)

target_compile_options(ax_plugin_yolov8_split PRIVATE
    -Wall -Wextra -Wno-unused-parameter
)

target_link_libraries(ax_plugin_yolov8_split PRIVATE ax_plugin_npu_common ax_pipeline_tracking)

set_property(GLOBAL APPEND PROPERTY AXP_PLUGIN_TARGETS ax_plugin_yolov8_split)
