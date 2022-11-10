file(GLOB SRC_LIST "*.c" "pipe/*.c" "../utilities/*.cpp" "../utilities/*.c")
file(GLOB SRC_LIST_COMMON "${BSP_MSP_DIR}/../sample/common/*.c" "${BSP_MSP_DIR}/../sample/common/common_codec/*.c")
file(GLOB SRC_LIST_SAMPLE_RUN_JOINT "../sample_run_joint/*.cpp")
file(GLOB SRC_LIST_RTSP "../rtsp/src/*.c")

add_executable(sample_vin_ivps_joint_venc_rtsp 
                ${SRC_LIST}
                ${SRC_LIST_COMMON}
                ${SRC_LIST_SAMPLE_RUN_JOINT}
                ${SRC_LIST_RTSP}
                )

# opencv
set (OpenCV_DIR ${CMAKE_SOURCE_DIR}/3rdparty/opencv-arm-linux/lib/cmake/opencv4)
find_package(OpenCV REQUIRED)
target_include_directories (sample_vin_ivps_joint_venc_rtsp PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(sample_vin_ivps_joint_venc_rtsp PRIVATE ${OpenCV_LIBS})

target_include_directories (sample_vin_ivps_joint_venc_rtsp PRIVATE ${BSP_MSP_DIR}/../sample/common)
target_include_directories (sample_vin_ivps_joint_venc_rtsp PRIVATE ${BSP_MSP_DIR}/include)
target_include_directories (sample_vin_ivps_joint_venc_rtsp PRIVATE ${BSP_MSP_DIR}/include/npu_cv_kit)
target_link_directories (sample_vin_ivps_joint_venc_rtsp PRIVATE ${BSP_MSP_DIR}/lib)

target_link_libraries (sample_vin_ivps_joint_venc_rtsp PRIVATE pthread dl) # ax620a use this
# target_link_libraries (sample_vin_ivps_joint_venc_rtsp PRIVATE pthread dl stdc++fs) # ax620u use this
target_link_libraries (sample_vin_ivps_joint_venc_rtsp PRIVATE ax_run_joint ax_interpreter_external ax_interpreter ax_sys axsyslog stdc++fs)
target_link_libraries (sample_vin_ivps_joint_venc_rtsp PRIVATE ax_venc ax_ivps ax_npu_cv_kit ax_3a ax_proton ax_mipi gomp)
target_link_libraries (sample_vin_ivps_joint_venc_rtsp PRIVATE sns_os04a10 sns_os04a10_master sns_os04a10_slave sns_gc4653)

# openssl
target_link_directories (sample_vin_ivps_joint_venc_rtsp PRIVATE ${BSP_MSP_DIR}/../../third-party/openssl/lib)

# rtsp
target_include_directories (sample_vin_ivps_joint_venc_rtsp PRIVATE ../rtsp/inc)


install (TARGETS sample_vin_ivps_joint_venc_rtsp DESTINATION bin)