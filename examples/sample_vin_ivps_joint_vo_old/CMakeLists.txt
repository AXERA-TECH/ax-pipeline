set(SampleName sample_vin_ivps_joint_vo_old)

# opencv
set(OpenCV_DIR ${CMAKE_SOURCE_DIR}/3rdparty/opencv-arm-linux/lib/cmake/opencv4)
find_package(OpenCV REQUIRED)

function(targetlink name)
    target_include_directories(${name} PRIVATE ${OpenCV_INCLUDE_DIRS})
    target_link_libraries(${name} PRIVATE ${OpenCV_LIBS})

    # drm
    target_link_directories(${name} PRIVATE ${BSP_MSP_DIR}/../../third-party/drm/lib)
    target_link_libraries(${name} PRIVATE drm)

    target_include_directories(${name} PRIVATE ${BSP_MSP_DIR}/../sample/common)
    target_include_directories(${name} PRIVATE ${BSP_MSP_DIR}/include)
    target_include_directories(${name} PRIVATE ${BSP_MSP_DIR}/include/npu_cv_kit)
    target_link_directories(${name} PRIVATE ${BSP_MSP_DIR}/lib)

    target_link_libraries(${name} PRIVATE pthread dl) # ax620a use this

    target_link_libraries(${name} PRIVATE sample_run_joint)

    # target_link_libraries (sample_vin_ivps_joint_vo PRIVATE pthread dl stdc++fs) # ax620u use this
    target_link_libraries(${name} PRIVATE ax_run_joint ax_interpreter_external ax_interpreter ax_sys axsyslog stdc++fs)
    target_link_libraries(${name} PRIVATE ax_venc ax_ivps ax_npu_cv_kit ax_3a ax_proton ax_mipi gomp ax_vo)
    target_link_libraries(${name} PRIVATE sns_os04a10 sns_os04a10_master sns_os04a10_slave sns_gc4653)

endfunction()

file(GLOB SRC_LIST "*.c" "pipe/*.c" "../utilities/osd_utils_old.cpp")
file(GLOB SRC_LIST_COMMON "../common/*.c" "${BSP_MSP_DIR}/../sample/common/*.c" "${BSP_MSP_DIR}/../sample/common/common_codec/*.c" "${BSP_MSP_DIR}/../sample/common/vo/*.c")
list(APPEND SRC_BASE_LIST
    ${SRC_LIST}
    ${SRC_LIST_COMMON}
)

add_executable(${SampleName}
    ${SRC_BASE_LIST}
)
targetlink(${SampleName})
install(TARGETS ${SampleName} DESTINATION bin)

if(SIPY_BUILD)
    set(SampleName ${SampleName}_sipy)
    add_library(${SampleName} SHARED
        ${SRC_BASE_LIST}
    )
    SET_TARGET_PROPERTIES(${SampleName} PROPERTIES VERSION ${SIPY_VERSION} SOVERSION ${SIPY_SOVERSION})
    SET_TARGET_PROPERTIES(${SampleName} PROPERTIES OUTPUT_NAME ${SampleName})
    targetlink(${SampleName})
    install(TARGETS ${SampleName} DESTINATION lib)
endif()
