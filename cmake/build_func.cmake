function(ax_include_link name input_type)
    target_include_directories(${name} PRIVATE ${OpenCV_INCLUDE_DIRS})
    target_link_libraries(${name} PRIVATE ${OpenCV_LIBS})

    target_include_directories(${name} PRIVATE ${CMAKE_SOURCE_DIR}/examples/common/common_pipeline)
    target_include_directories(${name} PRIVATE ${BSP_MSP_DIR}/../sample/common)
    target_include_directories(${name} PRIVATE ${BSP_MSP_DIR}/include)
    target_include_directories(${name} PRIVATE ${BSP_MSP_DIR}/include/npu_cv_kit)
    target_link_directories(${name} PRIVATE ${BSP_MSP_DIR}/lib)

    target_link_libraries(${name} PRIVATE pthread dl) # ax620a use this

    target_link_libraries(${name} PRIVATE axdl common_pipeline)

    # onnxruntime
    if(ONNXRUNTIME_DIR)
        target_include_directories(${name} PRIVATE ${ONNXRUNTIME_DIR}/include)
        target_link_directories(${name} PRIVATE ${ONNXRUNTIME_DIR}/lib)
        target_link_libraries(${name} PRIVATE onnxruntime)
    endif()

    if(AXERA_TARGET_CHIP MATCHES "AX650")
        # drm
        target_link_directories(${name} PRIVATE ${BSP_MSP_DIR}/../../third-party/drm/lib)
        target_link_libraries(${name} PRIVATE drm)
        target_link_libraries(${name} PRIVATE ax_interpreter ax_sys ax_venc ax_vdec ax_ivps ax_ive ax_engine ax_vo gomp stdc++fs)
        if(input_type MATCHES "vin")
            target_link_libraries(${name} PRIVATE ax_proton ax_3a ax_mipi ax_nt_stream ax_nt_ctrl)
        endif()
    elseif(AXERA_TARGET_CHIP MATCHES "AX620E")
        target_link_libraries(${name} PRIVATE ax_interpreter ax_sys ax_venc ax_vdec ax_ivps ax_ive ax_engine ax_vo ax_proton ax_ae ax_af ax_awb ax_mipi ax_nt_stream ax_nt_ctrl gomp stdc++fs)

    elseif(AXERA_TARGET_CHIP MATCHES "AX620A")
        # drm
        target_link_directories(${name} PRIVATE ${BSP_MSP_DIR}/../../third-party/drm/lib)
        target_link_libraries(${name} PRIVATE drm)
        # target_link_libraries(sample_vin_ivps_joint_venc_rtsp PRIVATE pthread dl stdc++fs) # ax620u use this
        target_link_libraries(${name} PRIVATE ax_run_joint ax_interpreter_external ax_interpreter ax_sys axsyslog stdc++fs)
        target_link_libraries(${name} PRIVATE ax_venc ax_vdec ax_vo ax_ivps ax_npu_cv_kit ax_3a ax_proton ax_mipi gomp)

        if(input_type MATCHES "vin")
            target_link_libraries(${name} PRIVATE sns_os04a10 sns_os04a10_master sns_os04a10_slave sns_gc4653)
        endif()
    endif()

    # openssl
    target_link_directories(${name} PRIVATE ${BSP_MSP_DIR}/../../third-party/openssl/lib)

    # rtsp
    # target_include_directories(${name} PRIVATE ../rtsp/inc)
    target_link_libraries(${name} PRIVATE RtspServer ByteTrack)

    if(input_type MATCHES "demux")
        target_include_directories(${name} PRIVATE ${CMAKE_SOURCE_DIR}/third-party/RTSP/include)
        target_link_libraries(${name} PRIVATE rtspclisvr Mp4Demuxer)
    elseif(input_type MATCHES "v4l2")
        target_include_directories(${name} PRIVATE ${CMAKE_SOURCE_DIR}/third-party/libv4l2cpp/inc)

        target_include_directories(${name} PRIVATE ${CMAKE_SOURCE_DIR}/third-party/libyuv/include)
        target_link_directories(${name} PRIVATE ${CMAKE_SOURCE_DIR}/third-party/libyuv/lib)
        target_link_libraries(${name} PRIVATE yuv jpeg)
    endif()
endfunction()