file(GLOB SRC_LIST "*.c" "pipe/*.c" "../utilities/*.cpp")
file(GLOB SRC_LIST_COMMON "${BSP_MSP_DIR}/../sample/common/*.c" "${BSP_MSP_DIR}/../sample/common/common_codec/*.c" "${BSP_MSP_DIR}/../sample/common/vo/*.c")
file(GLOB SRC_LIST_SAMPLE_RUN_JOINT "../sample_run_joint/*.cpp")

add_executable(sample_vin_ivps_joint_venc_rtsp_vo_onvif_mp4v2
                ${SRC_LIST}
                ${SRC_LIST_COMMON}
                ${SRC_LIST_SAMPLE_RUN_JOINT}
                )

# opencv
set (OpenCV_DIR ${CMAKE_SOURCE_DIR}/3rdparty/opencv-arm-linux/lib/cmake/opencv4)
find_package(OpenCV REQUIRED)
target_include_directories (sample_vin_ivps_joint_venc_rtsp_vo_onvif_mp4v2 PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(sample_vin_ivps_joint_venc_rtsp_vo_onvif_mp4v2 PRIVATE ${OpenCV_LIBS})


# drm
target_link_directories (sample_vin_ivps_joint_venc_rtsp_vo_onvif_mp4v2 PRIVATE ${BSP_MSP_DIR}/../../third-party/drm/lib)
target_link_libraries (sample_vin_ivps_joint_venc_rtsp_vo_onvif_mp4v2 PRIVATE drm)

target_include_directories (sample_vin_ivps_joint_venc_rtsp_vo_onvif_mp4v2 PRIVATE ${BSP_MSP_DIR}/../sample/common)
target_include_directories (sample_vin_ivps_joint_venc_rtsp_vo_onvif_mp4v2 PRIVATE ${BSP_MSP_DIR}/include)
target_include_directories (sample_vin_ivps_joint_venc_rtsp_vo_onvif_mp4v2 PRIVATE ${BSP_MSP_DIR}/include/npu_cv_kit)
target_link_directories (sample_vin_ivps_joint_venc_rtsp_vo_onvif_mp4v2 PRIVATE ${BSP_MSP_DIR}/lib)

target_link_libraries (sample_vin_ivps_joint_venc_rtsp_vo_onvif_mp4v2 PRIVATE pthread dl) # ax620a use this
# target_link_libraries (sample_vin_ivps_joint_venc_rtsp_vo_onvif_mp4v2 PRIVATE pthread dl stdc++fs) # ax620u use this
target_link_libraries (sample_vin_ivps_joint_venc_rtsp_vo_onvif_mp4v2 PRIVATE ax_run_joint ax_interpreter_external ax_interpreter ax_sys axsyslog stdc++fs)
target_link_libraries (sample_vin_ivps_joint_venc_rtsp_vo_onvif_mp4v2 PRIVATE ax_venc ax_ivps ax_npu_cv_kit ax_3a ax_proton ax_mipi gomp ax_vo)
target_link_libraries (sample_vin_ivps_joint_venc_rtsp_vo_onvif_mp4v2 PRIVATE sns_os04a10 sns_os04a10_master sns_os04a10_slave sns_gc4653)

# openssl
target_link_directories (sample_vin_ivps_joint_venc_rtsp_vo_onvif_mp4v2 PRIVATE ${BSP_MSP_DIR}/../../third-party/openssl/lib)

# rtsp
# target_include_directories (sample_vin_ivps_joint_venc_rtsp_vo_onvif_mp4v2 PRIVATE ../rtsp/inc)
# target_link_directories (sample_vin_ivps_joint_venc_rtsp_vo_onvif_mp4v2 PRIVATE ../rtsp/lib)

target_include_directories (sample_vin_ivps_joint_venc_rtsp_vo_onvif_mp4v2 PRIVATE ./rtsp/inc)
target_link_directories (sample_vin_ivps_joint_venc_rtsp_vo_onvif_mp4v2 PRIVATE ./rtsp/lib)

# onvif
target_include_directories (sample_vin_ivps_joint_venc_rtsp_vo_onvif_mp4v2 PRIVATE ./onvif/inc)
target_link_directories (sample_vin_ivps_joint_venc_rtsp_vo_onvif_mp4v2 PRIVATE ./onvif/lib)

# mp4v2
target_include_directories (sample_vin_ivps_joint_venc_rtsp_vo_onvif_mp4v2 PRIVATE ./mp4v2/inc)
target_link_directories (sample_vin_ivps_joint_venc_rtsp_vo_onvif_mp4v2 PRIVATE ./mp4v2/lib)

# target_link_libraries (sample_vin_ivps_joint_venc_rtsp_vo_onvif_mp4v2 PRIVATE ax_rtsp ssl crypto)
target_link_libraries (sample_vin_ivps_joint_venc_rtsp_vo_onvif_mp4v2 PRIVATE rtsp onvif mp4v2 ssl crypto)


install (TARGETS sample_vin_ivps_joint_venc_rtsp_vo_onvif_mp4v2 DESTINATION bin)