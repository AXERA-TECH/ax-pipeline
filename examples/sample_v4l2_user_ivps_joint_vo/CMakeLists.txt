set(SampleName sample_v4l2_user_ivps_joint_vo)

# opencv
set(OpenCV_DIR ${CMAKE_SOURCE_DIR}/3rdparty/opencv-arm-linux/lib/cmake/opencv4)
find_package(OpenCV REQUIRED)

function(targetlink name)
    target_include_directories(${name} PRIVATE ${OpenCV_INCLUDE_DIRS})
    target_link_libraries(${name} PRIVATE ${OpenCV_LIBS})

    target_include_directories(${name} PRIVATE ${BSP_MSP_DIR}/../sample/common)
    target_include_directories(${name} PRIVATE ${BSP_MSP_DIR}/include)
    target_include_directories(${name} PRIVATE ${BSP_MSP_DIR}/include/npu_cv_kit)
    target_link_directories(${name} PRIVATE ${BSP_MSP_DIR}/lib)

    target_link_libraries(${name} PRIVATE pthread dl) # ax620a use this

    target_link_libraries(${name} PRIVATE axdl)

    # drm
    target_link_directories(${name} PRIVATE ${BSP_MSP_DIR}/../../third-party/drm/lib)
    target_link_libraries(${name} PRIVATE drm)

    # target_link_libraries (sample_vin_ivps_joint_venc_rtsp PRIVATE pthread dl stdc++fs) # ax620u use this
    target_link_libraries(${name} PRIVATE ax_run_joint ax_interpreter_external ax_interpreter ax_sys axsyslog stdc++fs)
    target_link_libraries(${name} PRIVATE ax_venc ax_vdec ax_vo ax_ivps ax_npu_cv_kit ax_3a ax_proton ax_mipi gomp)
    target_link_libraries(${name} PRIVATE sns_os04a10 sns_os04a10_master sns_os04a10_slave sns_gc4653)

    # openssl
    target_link_directories(${name} PRIVATE ${BSP_MSP_DIR}/../../third-party/openssl/lib)

    # rtsp
    target_include_directories(${name} PRIVATE ../rtsp/inc)

    target_include_directories(${name} PRIVATE ../sample_v4l2_ivps_joint_vo/libv4l2cpp/inc)

    target_include_directories(${name} PRIVATE ./libyuv/include)
    target_link_directories(${name} PRIVATE ./libyuv/lib)
    target_link_libraries(${name} PRIVATE yuv jpeg)
endfunction()

file(GLOB SRC_BASE_LIST "*.cpp" "../utilities/*.c")
file(GLOB SRC_LIST_COMMON "../common/common_func.c" "../common/common_joint.c" "../common/common_pipeline/*.cpp" "${BSP_MSP_DIR}/../sample/common/*.c" "${BSP_MSP_DIR}/../sample/common/common_codec/*.c" "${BSP_MSP_DIR}/../sample/common/vo/*.c")
file(GLOB SRC_LIST_RTSP "../rtsp/src/*.c")
file(GLOB SRC_LIST_V4L2 "../sample_v4l2_ivps_joint_vo/libv4l2cpp/src/*.cpp")
list(APPEND SRC_BASE_LIST
    ${SRC_LIST}
    ${SRC_LIST_COMMON}
    ${SRC_LIST_RTSP}
    ${SRC_LIST_V4L2}
)

add_executable(${SampleName}
    ${SRC_BASE_LIST}
)
targetlink(${SampleName})
install(TARGETS ${SampleName} DESTINATION bin)


set(SampleName ${SampleName}_sipy)

if (SIPY_BUILD)
    add_library(${SampleName} SHARED
        ${SRC_BASE_LIST}
    )
    SET_TARGET_PROPERTIES(${SampleName} PROPERTIES VERSION ${SIPY_VERSION} SOVERSION ${SIPY_SOVERSION})
    SET_TARGET_PROPERTIES(${SampleName} PROPERTIES OUTPUT_NAME ${SampleName})
    targetlink(${SampleName})
    install(TARGETS ${SampleName} DESTINATION lib)
endif()
