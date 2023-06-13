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
    set(AXERA_TARGET_CHIP "ax620a")
endif()
message(STATUS "AXERA_TARGET_CHIP = ${AXERA_TARGET_CHIP}")

# bsp
if(NOT BSP_MSP_DIR)
    set(BSP_MSP_DIR ${CMAKE_SOURCE_DIR}/axpi_bsp_sdk/msp/out)
endif()
message(STATUS "BSP_MSP_DIR = ${BSP_MSP_DIR}")

option(BUILD_ON_BOARD "build in axpi" OFF)
option(SIPY_BUILD "build *.so for sipeed python package" OFF)
if(SIPY_BUILD)
    set(SIPY_SOVERSION 1)
    set(SIPY_VERSION ${SIPY_SOVERSION}.1.2)
endif()

# check toolchain about
if(CMAKE_TOOLCHAIN_FILE)
    set(LIBRARY_OUTPUT_PATH_ROOT ${CMAKE_BINARY_DIR} CACHE PATH "root for library output, set this to change where android libs are compiled to")

    # get absolute path, but get_filename_component ABSOLUTE only refer with source dir, so find_file here :(
    get_filename_component(CMAKE_TOOLCHAIN_FILE_NAME ${CMAKE_TOOLCHAIN_FILE} NAME)
    find_file(CMAKE_TOOLCHAIN_FILE ${CMAKE_TOOLCHAIN_FILE_NAME} PATHS ${CMAKE_SOURCE_DIR} NO_DEFAULT_PATH)
else()
    if(NOT BUILD_ON_BOARD)
        if(AXERA_TARGET_CHIP MATCHES "ax650" 
            OR AXERA_TARGET_CHIP MATCHES "AX650"
            )
            set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/toolchains/aarch64-none-linux-gnu.toolchain.cmake)
        elseif(AXERA_TARGET_CHIP MATCHES "ax620a"
            OR AXERA_TARGET_CHIP MATCHES "ax620"
            OR AXERA_TARGET_CHIP MATCHES "AX620"
            OR AXERA_TARGET_CHIP MATCHES "AX620A"
            OR AXERA_TARGET_CHIP MATCHES "AX620a"
            )
            set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/toolchains/arm-linux-gnueabihf.toolchain.cmake)
        elseif(AXERA_TARGET_CHIP MATCHES "ax630a"
            OR AXERA_TARGET_CHIP MATCHES "ax630"
            OR AXERA_TARGET_CHIP MATCHES "AX630"
            OR AXERA_TARGET_CHIP MATCHES "AX630A"
            OR AXERA_TARGET_CHIP MATCHES "AX630a"
            )
            set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/toolchains/aarch64-linux-gnu.toolchain.cmake)
        endif()
        message(STATUS "build cross platform")
    else()
        message(STATUS "build on board")
    endif()
endif()
message(STATUS "CMAKE_TOOLCHAIN_FILE = ${CMAKE_TOOLCHAIN_FILE}")

project(ax_pipeline)

add_subdirectory(third-party/RTSP)
add_subdirectory(third-party/RtspServer)    
add_subdirectory(third-party/ByteTrack)
add_subdirectory(third-party/mp4demux)

# 添加 FLAGS 检查代码是否有明显 bug
include(cmake/overlook.cmake)
# 编译的函数
include(cmake/build_func.cmake)

if(AXERA_TARGET_CHIP MATCHES "ax650" OR AXERA_TARGET_CHIP MATCHES "AX650")
    add_compile_options(-DAXERA_TARGET_CHIP_AX650)
    add_compile_options(-UAXERA_TARGET_CHIP_AX620)
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O1 -Wall -s -fPIC")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O1 -Wall -s -fPIC")

    # opencv
    if(NOT OpenCV_DIR)
        set(OpenCV_DIR ${CMAKE_SOURCE_DIR}/3rdparty/libopencv-4.5.5-aarch64/lib/cmake/opencv4)
    endif()
    find_package(OpenCV REQUIRED)
    
    add_subdirectory(examples/common/common_pipeline)
    add_subdirectory(examples/libaxdl)
    add_subdirectory(examples/sample_demux_ivps_joint_rtsp)
    # add_subdirectory(examples/sample_mp4_ivps_joint_rtsp)
    add_subdirectory(examples/sample_multi_demux_ivps_joint_hdmi_vo)
    add_subdirectory(examples/sample_multi_demux_ivps_joint_multi_rtsp)
    add_subdirectory(examples/sample_multi_demux_ivps_joint_multi_rtsp_hdmi_vo)
    add_subdirectory(examples/sample_demux_ivps_joint_rtsp_hdmi_vo)
    add_subdirectory(examples/sample_demux_ivps_joint_hdmi_vo)
    file(GLOB SAMPLE_RUN_JOINT_CONFIG_FILES "${CMAKE_SOURCE_DIR}/examples/libaxdl/config/ax650/*")
    install(FILES ${SAMPLE_RUN_JOINT_CONFIG_FILES} DESTINATION bin/config)
elseif(AXERA_TARGET_CHIP MATCHES "ax620a"
    OR AXERA_TARGET_CHIP MATCHES "ax620"
    OR AXERA_TARGET_CHIP MATCHES "AX620"
    OR AXERA_TARGET_CHIP MATCHES "AX620A"
    OR AXERA_TARGET_CHIP MATCHES "AX620a"
    OR AXERA_TARGET_CHIP MATCHES "ax630a"
    OR AXERA_TARGET_CHIP MATCHES "ax630"
    OR AXERA_TARGET_CHIP MATCHES "AX630"
    OR AXERA_TARGET_CHIP MATCHES "AX630A"
    OR AXERA_TARGET_CHIP MATCHES "AX630a"
    )
    add_compile_options(-DAXERA_TARGET_CHIP_AX620)
    add_compile_options(-UAXERA_TARGET_CHIP_AX650)
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O1 -Wall -s -fPIC -march=armv7-a -mfloat-abi=hard -mfpu=neon")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O1 -Wall -s -fPIC -march=armv7-a -mfloat-abi=hard -mfpu=neon")

    # opencv
    if(NOT OpenCV_DIR)
        set(OpenCV_DIR ${CMAKE_SOURCE_DIR}/3rdparty/opencv-arm-linux/lib/cmake/opencv4)
    endif()
    find_package(OpenCV REQUIRED)

    add_subdirectory(examples/common/common_pipeline)
    add_subdirectory(examples/libaxdl)
    # deprecated
    # add_subdirectory(examples/sample_run_joint)
    # add_subdirectory(examples/sample_vin_joint)
    # add_subdirectory(examples/sample_vin_ivps_joint_vo_old)
    # add_subdirectory(examples/sample_vin_ivps_joint_venc_rtsp_old)
    add_subdirectory(examples/sample_vin_ivps_joint_venc_rtsp_vo)
    add_subdirectory(examples/sample_vin_ivps_joint_venc_rtsp)
    add_subdirectory(examples/sample_vin_ivps_joint_vo)
    add_subdirectory(examples/sample_vin_ivps_joint_vo_h265)
    add_subdirectory(examples/sample_v4l2_ivps_joint_vo)
    add_subdirectory(examples/sample_v4l2_user_ivps_joint_vo)
    # add_subdirectory(examples/sample_h264_ivps_joint_vo)
    # add_subdirectory(examples/sample_rtsp_ivps_joint_vo)
    # add_subdirectory(examples/sample_rtsp_ivps_joint_rtsp)
    add_subdirectory(examples/sample_demux_ivps_joint_vo)
    add_subdirectory(examples/sample_demux_ivps_joint_rtsp)
    add_subdirectory(examples/sample_demux_ivps_joint_rtsp_vo)
    add_subdirectory(examples/sample_multi_demux_ivps_joint_multi_rtsp)

    file(GLOB SAMPLE_RUN_JOINT_CONFIG_FILES "${CMAKE_SOURCE_DIR}/examples/libaxdl/config/ax620/*")
    install(FILES ${SAMPLE_RUN_JOINT_CONFIG_FILES} DESTINATION bin/config)
endif()
