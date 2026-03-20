cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

project(ax_pipeline LANGUAGES C CXX)

if(" ${CMAKE_SOURCE_DIR}" STREQUAL " ${CMAKE_BINARY_DIR}")
    message(FATAL_ERROR "In-place builds are not allowed. Use a separate build directory.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(AXP_BUILD_BYTETRACK "Build ByteTrack dependency (unused by default)" OFF)

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

# ---- optional third-party ----
if(AXP_BUILD_BYTETRACK AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third-party/ByteTrack/CMakeLists.txt")
    add_subdirectory(third-party/ByteTrack)
endif()

# ---- app ----
add_executable(ax_pipeline_app
    src/main.cpp
)

target_include_directories(ax_pipeline_app
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/deps/ax-video-sdk/include
        ${CMAKE_CURRENT_SOURCE_DIR}/third-party/cmdline
        ${CMAKE_CURRENT_SOURCE_DIR}/third-party/json
)

target_compile_options(ax_pipeline_app PRIVATE
    -Wall -Wextra -Wno-unused-parameter
)

target_link_libraries(ax_pipeline_app PRIVATE ax_video_sdk)

if(DEFINED AXSDK_CHIP_TYPE)
    string(TOLOWER "${AXSDK_CHIP_TYPE}" AXP_CHIP_NORMALIZED)
    if(NOT AXP_CHIP_NORMALIZED STREQUAL "axcl")
        # Provide MSP -L paths for the link step; do not pull MSP libs into the app link line.
        target_link_libraries(ax_pipeline_app PRIVATE ax_video_sdk::ax_msp)

        # MSP shared objects sometimes have unresolved DT_NEEDED chains (e.g. optional libexif).
        # Allow undefined symbols in shared libs at link time; they will be resolved on target.
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
            target_link_options(ax_pipeline_app PRIVATE -Wl,--allow-shlib-undefined)
        endif()
    endif()
endif()

install(TARGETS ax_pipeline_app RUNTIME DESTINATION bin)
install(DIRECTORY configs DESTINATION .)

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
