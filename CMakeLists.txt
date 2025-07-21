# check CMake version
cmake_minimum_required(VERSION 3.13 FATAL_ERROR)

# disable inplace builds to prevent source tree corruption.
if(" ${CMAKE_SOURCE_DIR}" STREQUAL " ${CMAKE_BINARY_DIR}")
    message(FATAL_ERROR "FATAL: Building inplace are not allowed. You should create a separate directory for Building.")
endif()

# set cmake_install_prefix path
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "Installation Directory")
endif()
message(STATUS "CMAKE_INSTALL_PREFIX = ${CMAKE_INSTALL_PREFIX}")

# check if building type is not set
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build" FORCE)
endif()


# default compile for ax620
if(NOT AXERA_TARGET_CHIP)
    set(AXERA_TARGET_CHIP "ax650")
endif()
if(AXERA_TARGET_CHIP MATCHES "ax650" OR AXERA_TARGET_CHIP MATCHES "AX650")
    set(AXERA_TARGET_CHIP "AX650")
    message(STATUS "SET AXERA_TARGET_CHIP = ${AXERA_TARGET_CHIP}")
elseif(AXERA_TARGET_CHIP MATCHES "ax620e"
    OR AXERA_TARGET_CHIP MATCHES "AX620E"
    OR AXERA_TARGET_CHIP MATCHES "AX620e"
    OR AXERA_TARGET_CHIP MATCHES "AX620Q"
    OR AXERA_TARGET_CHIP MATCHES "AX620q"
    OR AXERA_TARGET_CHIP MATCHES "ax620q"
    OR AXERA_TARGET_CHIP MATCHES "AX630C"
    OR AXERA_TARGET_CHIP MATCHES "ax630C"
    OR AXERA_TARGET_CHIP MATCHES "ax630c"
    )
    set(AXERA_TARGET_CHIP "AX620E")
    message(STATUS "SET AXERA_TARGET_CHIP = ${AXERA_TARGET_CHIP}")
else()
    message(FATAL_ERROR "Not support AXERA_TARGET_CHIP: ${AXERA_TARGET_CHIP}, choose from [ AX650, AX620E]")
endif()
message(STATUS "AXERA_TARGET_CHIP = ${AXERA_TARGET_CHIP}")

# bsp
if(NOT BSP_MSP_DIR)
    set(BSP_MSP_DIR ${CMAKE_SOURCE_DIR}/axpi_bsp_sdk/msp/out)
endif()
message(STATUS "BSP_MSP_DIR = ${BSP_MSP_DIR}")

# check bsp exist
if(NOT EXISTS ${BSP_MSP_DIR})
    message(FATAL_ERROR "FATAL: BSP_MSP_DIR ${BSP_MSP_DIR} not exist")
endif()


set(MSP_LIB_PATHS
    "${BSP_MSP_DIR}/lib"
    "${BSP_MSP_DIR}/arm_glibc/lib"
    "${BSP_MSP_DIR}/arm64_glibc/lib"
)

set(BSP_VERSION "")
foreach(PATH ${MSP_LIB_PATHS})
    if(NOT BSP_VERSION)
        set(CMD_GET_BSP_VERSION "strings ${PATH}/libax_sys.so | grep 'Axera version' | awk '{print $4}'")
        message(STATUS "CMD_GET_BSP_VERSION = ${CMD_GET_BSP_VERSION}")
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E env "SHELL=/bin/bash" bash -c "${CMD_GET_BSP_VERSION}"
            TIMEOUT 5
            RESULT_VARIABLE CMD_RESULT
            OUTPUT_VARIABLE BSP_VERSION
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        if(NOT CMD_RESULT EQUAL 0)
            message(STATUS "Command execution failed for ${PATH}")
        endif()
    endif()
endforeach()

if(NOT BSP_VERSION)
    message(FATAL_ERROR "FATAL: BSP_VERSION ${BSP_VERSION} not found")
endif()
message(STATUS "BSP_VERSION = ${BSP_VERSION}")
add_definitions(-DAXERA_BSP_VERSION="${BSP_VERSION}")

# onnxruntime for owlvit
if(ONNXRUNTIME_DIR)
    add_compile_options(-DBUILT_WITH_ONNXRUNTIME)
    message(STATUS "ONNXRUNTIME_DIR = ${ONNXRUNTIME_DIR}")
else()
    message(STATUS "ONNXRUNTIME_DIR = NOT SET")
endif()

# check toolchain about
message(STATUS "CMAKE_TOOLCHAIN_FILE = ${CMAKE_TOOLCHAIN_FILE}")

project(ax_pipeline)
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -Ofast -Wall -s -fPIC -Wno-unused-function -Wno-unused-label")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Ofast -Wall -s -fPIC -Wno-unused-function -Wno-unused-label")

add_subdirectory(third-party/RTSP)
add_subdirectory(third-party/RtspServer)    
add_subdirectory(third-party/ByteTrack)
add_subdirectory(third-party/mp4demux)

# 添加 FLAGS 检查代码是否有明显 bug
include(cmake/overlook.cmake)
# 编译的函数
include(cmake/build_func.cmake)

if(AXERA_TARGET_CHIP MATCHES "AX650")
    add_compile_options(-DAXERA_TARGET_CHIP_AX650)
    add_compile_options(-UAXERA_TARGET_CHIP_AX620E)
    add_compile_options(-UAXERA_TARGET_CHIP_AX620)
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -march=armv8.2-a")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=armv8.2-a")

    # opencv
    if(NOT OpenCV_DIR)
        set(OpenCV_DIR ${CMAKE_SOURCE_DIR}/3rdparty/libopencv-4.5.5-aarch64/lib/cmake/opencv4)
    endif()
    message(STATUS "OpenCV_DIR = ${OpenCV_DIR}")
    find_package(OpenCV REQUIRED)
    
    add_subdirectory(examples/common/common_pipeline)
    add_subdirectory(examples/libaxdl)
    add_subdirectory(examples/sample_demux_ivps_npu_rtsp)
    add_subdirectory(examples/sample_multi_demux_ivps_npu_hdmi_vo)
    add_subdirectory(examples/sample_multi_demux_ivps_npu_multi_rtsp)
    add_subdirectory(examples/sample_multi_demux_ivps_npu_multi_rtsp_hdmi_vo)
    add_subdirectory(examples/sample_demux_ivps_npu_rtsp_hdmi_vo)
    add_subdirectory(examples/sample_demux_ivps_npu_hdmi_vo)
    file(GLOB SAMPLE_RUN_JOINT_CONFIG_FILES "${CMAKE_SOURCE_DIR}/examples/libaxdl/config/ax650/*")
    install(FILES ${SAMPLE_RUN_JOINT_CONFIG_FILES} DESTINATION bin/config)
elseif(AXERA_TARGET_CHIP MATCHES "AX620E")
    add_compile_options(-DAXERA_TARGET_CHIP_AX620E)
    add_compile_options(-UAXERA_TARGET_CHIP_AX650)
    add_compile_options(-UAXERA_TARGET_CHIP_AX620)
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -march=armv8.2-a")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=armv8.2-a")

    # opencv
    if(NOT OpenCV_DIR)
        set(OpenCV_DIR ${CMAKE_SOURCE_DIR}/3rdparty/libopencv-4.5.5-aarch64/lib/cmake/opencv4)
    endif()
    message(STATUS "OpenCV_DIR = ${OpenCV_DIR}")
    find_package(OpenCV REQUIRED)
    
    add_subdirectory(examples/common/common_pipeline)
    add_subdirectory(examples/libaxdl)
    add_subdirectory(examples/sample_demux_ivps_npu_rtsp)
    add_subdirectory(examples/sample_multi_demux_ivps_npu_multi_rtsp)
    add_subdirectory(examples/sample_vin_ivps_npu_venc_rtsp)
    file(GLOB SAMPLE_RUN_JOINT_CONFIG_FILES "${CMAKE_SOURCE_DIR}/examples/libaxdl/config/ax650/*")
    install(FILES ${SAMPLE_RUN_JOINT_CONFIG_FILES} DESTINATION bin/config)
endif()
