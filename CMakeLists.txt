project(ax_pipeline)
# check CMake version
cmake_minimum_required (VERSION 3.13 FATAL_ERROR)

# disable inplace builds to prevent source tree corruption.
if (" ${CMAKE_SOURCE_DIR}" STREQUAL " ${CMAKE_BINARY_DIR}")
    message (FATAL_ERROR "FATAL: Building inplace are not allowed. You should create a separate directory for Building.")
endif ()

# set cmake_install_prefix path
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "Installation Directory")
endif()
message(STATUS "CMAKE_INSTALL_PREFIX = ${CMAKE_INSTALL_PREFIX}")


# enable the languages which in use
enable_language (C CXX)

# check toolchain about
if(CMAKE_TOOLCHAIN_FILE)
    set(LIBRARY_OUTPUT_PATH_ROOT ${CMAKE_BINARY_DIR} CACHE PATH "root for library output, set this to change where android libs are compiled to")
    # get absolute path, but get_filename_component ABSOLUTE only refer with source dir, so find_file here :(
    get_filename_component(CMAKE_TOOLCHAIN_FILE_NAME ${CMAKE_TOOLCHAIN_FILE} NAME)
    find_file(CMAKE_TOOLCHAIN_FILE ${CMAKE_TOOLCHAIN_FILE_NAME} PATHS ${CMAKE_SOURCE_DIR} NO_DEFAULT_PATH)
    message(STATUS "CMAKE_TOOLCHAIN_FILE = ${CMAKE_TOOLCHAIN_FILE}")
endif()

# check if building type is not set
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build" FORCE)
endif()

# bsp
if (NOT BSP_MSP_DIR)
    set (BSP_MSP_DIR ${CMAKE_SOURCE_DIR}/axpi_bsp_sdk/msp/out)
endif ()
message(STATUS "BSP_MSP_DIR = ${BSP_MSP_DIR}")

option(SIPY_BUILD "build *.so for sipeed python package" OFF)
if (SIPY_BUILD)
    set(SIPY_SOVERSION 1)
    set(SIPY_VERSION ${SIPY_SOVERSION}.0.7)
endif()

set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O1 -Wall -s -fPIC")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O1 -Wall -s -fPIC")

add_subdirectory(examples/libaxdl)
add_subdirectory(examples/sample_run_joint)
add_subdirectory(examples/sample_vin_ivps_joint_venc_rtsp_vo)
# add_subdirectory(examples/sample_vin_ivps_joint_venc_rtsp_old)
add_subdirectory(examples/sample_vin_ivps_joint_venc_rtsp)
# add_subdirectory(examples/sample_vin_ivps_joint_vo_old)
add_subdirectory(examples/sample_vin_ivps_joint_vo)
# add_subdirectory(examples/sample_vin_joint)
add_subdirectory(examples/sample_h264_ivps_joint_vo)
add_subdirectory(examples/sample_vin_ivps_joint_vo_h265)
add_subdirectory(examples/sample_v4l2_ivps_joint_vo)
add_subdirectory(examples/sample_rtsp_ivps_joint_vo)
add_subdirectory(examples/sample_rtsp_ivps_joint_rtsp_vo)
add_subdirectory(examples/sample_v4l2_user_ivps_joint_vo)

file(GLOB SAMPLE_RUN_JOINT_CONFIG_FILES "${CMAKE_SOURCE_DIR}/examples/libaxdl/config/*")
install (FILES ${SAMPLE_RUN_JOINT_CONFIG_FILES} DESTINATION bin/config)