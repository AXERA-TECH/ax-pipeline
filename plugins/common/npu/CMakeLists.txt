cmake_minimum_required(VERSION 3.16)

set(AXP_PLUGIN_NPU_SOURCES
    src/npu/models/ax_model_base.cpp
    src/npu/models/ax_model_det.cpp
    src/npu/runner/logger.cpp
)

if(DEFINED AXSDK_CHIP_TYPE)
    string(TOLOWER "${AXSDK_CHIP_TYPE}" AXP_CHIP_FOR_PLUGIN_NPU)
    if(AXP_CHIP_FOR_PLUGIN_NPU STREQUAL "axcl")
        list(APPEND AXP_PLUGIN_NPU_SOURCES
            src/npu/runner/axcl/ax_model_runner_axcl.cpp
            src/npu/runner/axcl/axcl_manager.cpp
        )
    else()
        list(APPEND AXP_PLUGIN_NPU_SOURCES
            src/npu/runner/ax650/ax_model_runner_ax650.cpp
        )
    endif()
else()
    list(APPEND AXP_PLUGIN_NPU_SOURCES
        src/npu/runner/ax650/ax_model_runner_ax650.cpp
    )
endif()

add_library(ax_plugin_npu_common STATIC ${AXP_PLUGIN_NPU_SOURCES})

target_include_directories(ax_plugin_npu_common
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/deps/ax-video-sdk/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/npu/runner
)

target_compile_options(ax_plugin_npu_common PRIVATE
    -Wall -Wextra -Wno-unused-parameter
)

target_link_libraries(ax_plugin_npu_common PUBLIC ax_video_sdk ax_video_sdk::ax_msp)

if(DEFINED AXSDK_CHIP_TYPE)
    string(TOLOWER "${AXSDK_CHIP_TYPE}" AXP_CHIP_NORMALIZED_PLUGIN_NPU)
    if(AXP_CHIP_NORMALIZED_PLUGIN_NPU STREQUAL "axcl")
        target_compile_definitions(ax_plugin_npu_common PUBLIC AXPIPELINE_HAVE_AXCL=1 AXPIPELINE_DEFAULT_BACKEND_AXCL=1)
        # AXCL engine API (AXCL_ENGINE_*) lives in libaxcl_npu.so.
        target_link_libraries(ax_plugin_npu_common PUBLIC axcl_rt axcl_npu)
    else()
        target_compile_definitions(ax_plugin_npu_common PUBLIC AXPIPELINE_HAVE_MSP=1)
        target_link_libraries(ax_plugin_npu_common PUBLIC ax_engine ax_sys)
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
            target_link_options(ax_plugin_npu_common PUBLIC -Wl,--allow-shlib-undefined)
        endif()
    endif()
endif()
