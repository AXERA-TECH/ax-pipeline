cmake_minimum_required(VERSION 3.16)

add_library(ax_plugin_yolov8 SHARED
    plugin_yolov8.cpp
)

target_compile_definitions(ax_plugin_yolov8 PRIVATE AX_PLUGIN_BUILD_DLL=1)

target_include_directories(ax_plugin_yolov8
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../third-party/json
        ${CMAKE_CURRENT_SOURCE_DIR}/../common/npu/include
)

target_compile_options(ax_plugin_yolov8 PRIVATE
    -Wall -Wextra -Wno-unused-parameter
)

target_link_libraries(ax_plugin_yolov8 PRIVATE ax_plugin_npu_common)

set_property(GLOBAL APPEND PROPERTY AXP_PLUGIN_TARGETS ax_plugin_yolov8)

