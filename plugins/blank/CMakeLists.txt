cmake_minimum_required(VERSION 3.16)

add_library(ax_plugin_blank SHARED
    plugin_blank.cpp
)

target_compile_definitions(ax_plugin_blank PRIVATE AX_PLUGIN_BUILD_DLL=1)

target_include_directories(ax_plugin_blank
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)

target_compile_options(ax_plugin_blank PRIVATE
    -Wall -Wextra -Wno-unused-parameter
)

set_property(GLOBAL APPEND PROPERTY AXP_PLUGIN_TARGETS ax_plugin_blank)
