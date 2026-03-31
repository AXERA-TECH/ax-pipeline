cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

project(ax_pipeline LANGUAGES C CXX)

if(" ${CMAKE_SOURCE_DIR}" STREQUAL " ${CMAKE_BINARY_DIR}")
    message(FATAL_ERROR "In-place builds are not allowed. Use a separate build directory.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(AXP_BUILD_NPU_SMOKE_TESTS "Build NPU smoke tests (requires HW at runtime)" OFF)

# ---- ax-video-sdk (submodule) ----
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/deps/ax-video-sdk/CMakeLists.txt")
    message(FATAL_ERROR "Missing submodule deps/ax-video-sdk. Run: git submodule update --init --recursive")
endif()

# Force shared build so consumers (this app) don't need to link MSP libs explicitly.
set(AXSDK_BUILD_SHARED ON CACHE BOOL "" FORCE)
set(AXSDK_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(AXSDK_BUILD_SMOKE_TESTS OFF CACHE BOOL "" FORCE)

# ax-video-sdk expects these cache variables:
# - AXSDK_CHIP_TYPE
# - AXSDK_MSP_DIR (non-AXCL) OR AXSDK_AXCL_DIR (AXCL)
add_subdirectory(deps/ax-video-sdk deps/ax-video-sdk-build)

# ---- third-party (ByteTrack) ----
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third-party/ByteTrack/CMakeLists.txt")
    add_subdirectory(third-party/ByteTrack)
else()
    message(FATAL_ERROR "Missing third-party/ByteTrack")
endif()

# ---- app ----
set(AXP_NPU_SOURCES
    src/npu/async_detector.cpp
    src/npu/models/ax_model_base.cpp
    src/npu/models/ax_model_det.cpp
    src/npu/runner/logger.cpp
)

if(DEFINED AXSDK_CHIP_TYPE)
    string(TOLOWER "${AXSDK_CHIP_TYPE}" AXP_CHIP_FOR_NPU)
    if(AXP_CHIP_FOR_NPU STREQUAL "axcl")
        list(APPEND AXP_NPU_SOURCES
            src/npu/runner/axcl/ax_model_runner_axcl.cpp
            src/npu/runner/axcl/axcl_manager.cpp
        )
    else()
        list(APPEND AXP_NPU_SOURCES
            src/npu/runner/ax650/ax_model_runner_ax650.cpp
        )
    endif()
else()
    list(APPEND AXP_NPU_SOURCES
        src/npu/runner/ax650/ax_model_runner_ax650.cpp
    )
endif()

add_library(ax_pipeline_npu STATIC ${AXP_NPU_SOURCES})
target_include_directories(ax_pipeline_npu
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/deps/ax-video-sdk/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include/npu
        ${CMAKE_CURRENT_SOURCE_DIR}/src/npu/runner
        ${CMAKE_CURRENT_SOURCE_DIR}/src/npu/runner/utils
)
target_compile_options(ax_pipeline_npu PRIVATE
    -Wall -Wextra -Wno-unused-parameter
)
target_link_libraries(ax_pipeline_npu PUBLIC ax_video_sdk ax_video_sdk::ax_msp)

if(DEFINED AXSDK_CHIP_TYPE)
    string(TOLOWER "${AXSDK_CHIP_TYPE}" AXP_CHIP_NORMALIZED_NPU)
    if(AXP_CHIP_NORMALIZED_NPU STREQUAL "axcl")
        target_compile_definitions(ax_pipeline_npu PUBLIC AXPIPELINE_HAVE_AXCL=1 AXPIPELINE_DEFAULT_BACKEND_AXCL=1)
        target_link_libraries(ax_pipeline_npu PUBLIC axcl_rt)
    else()
        target_compile_definitions(ax_pipeline_npu PUBLIC AXPIPELINE_HAVE_MSP=1)
        target_link_libraries(ax_pipeline_npu PUBLIC ax_engine ax_sys)
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
            target_link_options(ax_pipeline_npu PUBLIC -Wl,--allow-shlib-undefined)
        endif()
    endif()
endif()

add_executable(ax_pipeline_app
    src/main.cpp
    src/tracking/ax_bytetrack.cpp
)

target_include_directories(ax_pipeline_app
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/deps/ax-video-sdk/include
        ${CMAKE_CURRENT_SOURCE_DIR}/third-party/ByteTrack/include
        ${CMAKE_CURRENT_SOURCE_DIR}/third-party/cmdline
        ${CMAKE_CURRENT_SOURCE_DIR}/third-party/json
)

target_compile_options(ax_pipeline_app PRIVATE
    -Wall -Wextra -Wno-unused-parameter
)

target_link_libraries(ax_pipeline_app PRIVATE ax_pipeline_npu ByteTrack)

install(TARGETS ax_pipeline_app RUNTIME DESTINATION bin)
install(DIRECTORY configs DESTINATION .)

# ---- model runtime test (requires HW at runtime) ----
add_executable(ax_pipeline_model_test
    src/tests/test_model_det_images.cpp
)
target_include_directories(ax_pipeline_model_test
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/deps/ax-video-sdk/include
        ${CMAKE_CURRENT_SOURCE_DIR}/third-party/cmdline
)
target_compile_options(ax_pipeline_model_test PRIVATE
    -Wall -Wextra -Wno-unused-parameter
)
target_link_libraries(ax_pipeline_model_test PRIVATE ax_pipeline_npu)

# ---- debug tool: dump MP4 video samples to AnnexB elementary stream ----
# Used to validate demux output against board samples (e.g. /opt/bin/sample_vdec).
add_executable(ax_mp4_dump_annexb
    src/mp4_dump_annexb.cpp
)

target_include_directories(ax_mp4_dump_annexb
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/deps/ax-video-sdk/include
        ${CMAKE_CURRENT_SOURCE_DIR}/deps/ax-video-sdk/src/codec
        ${CMAKE_CURRENT_SOURCE_DIR}/third-party/cmdline
)

target_compile_options(ax_mp4_dump_annexb PRIVATE
    -Wall -Wextra -Wno-unused-parameter
)

target_link_libraries(ax_mp4_dump_annexb PRIVATE ax_video_sdk)

if(DEFINED AXSDK_CHIP_TYPE)
    string(TOLOWER "${AXSDK_CHIP_TYPE}" AXP_CHIP_NORMALIZED_TOOL)
    if(NOT AXP_CHIP_NORMALIZED_TOOL STREQUAL "axcl")
        target_link_libraries(ax_mp4_dump_annexb PRIVATE ax_video_sdk::ax_msp)
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
            target_link_options(ax_mp4_dump_annexb PRIVATE -Wl,--allow-shlib-undefined)
        endif()
    endif()
endif()

install(TARGETS ax_mp4_dump_annexb RUNTIME DESTINATION bin)
