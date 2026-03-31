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

# ---- plugins ----
add_subdirectory(plugins)

# ---- app ----
add_executable(ax_pipeline_app
    src/main.cpp
    src/ai/async_infer.cpp
    src/ax_plugin/ax_plugin_client.cpp
    src/ax_plugin/ax_plugin_loader.cpp
    src/ax_plugin/ax_plugin_process.cpp
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

if(UNIX AND NOT APPLE)
    target_link_libraries(ax_pipeline_app PRIVATE dl)
endif()

target_link_libraries(ax_pipeline_app PRIVATE ax_video_sdk ax_video_sdk::ax_msp ByteTrack)

if(DEFINED AXSDK_CHIP_TYPE)
    string(TOLOWER "${AXSDK_CHIP_TYPE}" AXP_CHIP_APP_PLATFORM)
    if(AXP_CHIP_APP_PLATFORM STREQUAL "axcl")
        target_compile_definitions(ax_pipeline_app PRIVATE AXPIPELINE_APP_PLATFORM_AXCL=1)
        # Needed by subprocess isolation path to copy AXCL device frames to host.
        target_link_libraries(ax_pipeline_app PRIVATE axcl_rt)
    else()
        # When cross-building, allow unresolved MSP symbols inside dependent shared objects.
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
            target_link_options(ax_pipeline_app PRIVATE -Wl,--allow-shlib-undefined)
        endif()
    endif()
endif()

install(TARGETS ax_pipeline_app RUNTIME DESTINATION bin)

# ---- plugin host (subprocess isolation) ----
add_executable(ax_plugin_host
    src/ax_plugin/ax_plugin_host.cpp
)

target_include_directories(ax_plugin_host
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(ax_plugin_host PRIVATE
    -Wall -Wextra -Wno-unused-parameter
)

if(UNIX AND NOT APPLE)
    target_link_libraries(ax_plugin_host PRIVATE dl)
endif()

install(TARGETS ax_plugin_host RUNTIME DESTINATION bin)
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
target_link_libraries(ax_pipeline_model_test PRIVATE ax_plugin_npu_common)

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

# ---- unit tests (host-only) ----
enable_testing()

add_executable(ax_plugin_client_test
    src/tests/test_plugin_client.cpp
    src/ax_plugin/ax_plugin_client.cpp
    src/ax_plugin/ax_plugin_loader.cpp
    src/ax_plugin/ax_plugin_process.cpp
)

target_include_directories(ax_plugin_client_test
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/deps/ax-video-sdk/include
)

target_compile_options(ax_plugin_client_test PRIVATE
    -Wall -Wextra -Wno-unused-parameter
)

if(UNIX AND NOT APPLE)
    target_link_libraries(ax_plugin_client_test PRIVATE dl)
endif()

target_link_libraries(ax_plugin_client_test PRIVATE ax_video_sdk ax_video_sdk::ax_msp)

# AXCL: test binary links axcl_rt for device->host copy helper when built for AXCL.
if(DEFINED AXSDK_CHIP_TYPE)
    string(TOLOWER "${AXSDK_CHIP_TYPE}" AXP_CHIP_TEST_PLATFORM)
    if(AXP_CHIP_TEST_PLATFORM STREQUAL "axcl")
        target_link_libraries(ax_plugin_client_test PRIVATE axcl_rt)
    endif()
endif()

# In-proc plugin smoke: should work everywhere.
add_test(NAME plugin_blank_inproc
    COMMAND ax_plugin_client_test
        --mode=inproc
        --so=$<TARGET_FILE:ax_plugin_blank>
)

add_test(NAME plugin_blank_inproc_reopen
    COMMAND ax_plugin_client_test
        --mode=inproc
        --iters=10
        --so=$<TARGET_FILE:ax_plugin_blank>
)

# Subprocess plugin smoke (unix only).
if(UNIX AND NOT APPLE)
    add_test(NAME plugin_blank_process
        COMMAND ax_plugin_client_test
            --mode=process
            --so=$<TARGET_FILE:ax_plugin_blank>
    )

    add_test(NAME plugin_blank_process_reopen
        COMMAND ax_plugin_client_test
            --mode=process
            --iters=5
            --so=$<TARGET_FILE:ax_plugin_blank>
    )

    # Crash isolation: blank plugin aborts under env var; main test process should survive and recover.
    add_test(NAME plugin_blank_process_crash_isolated
        COMMAND ax_plugin_client_test
            --mode=process
            --crash-test=1
            --so=$<TARGET_FILE:ax_plugin_blank>
    )
endif()
