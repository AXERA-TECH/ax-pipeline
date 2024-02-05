set(SampleName sample_vin_ivps_npu_venc_rtsp)

file(GLOB SRC_BASE_LIST "*.cpp" "../utilities/*.c")
if(AXERA_TARGET_CHIP MATCHES "ax650" OR AXERA_TARGET_CHIP MATCHES "AX650")
    file(GLOB SRC_LIST_COMMON   "${BSP_MSP_DIR}/../sample/common/common_sys.c"
                                "${BSP_MSP_DIR}/../sample/common/common_cam.c"
                                "${BSP_MSP_DIR}/../sample/common/common_vin.c"
                                "${BSP_MSP_DIR}/../sample/common/common_nt.c"
                                "${BSP_MSP_DIR}/../sample/common/common_isp.c"
                                "${BSP_MSP_DIR}/../sample/common/common_hw.c"
                                "${BSP_MSP_DIR}/../sample/common/vo/*.c"
                                "../common/common_func.c" "../common/common_check.cpp")
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
    file(GLOB SRC_LIST_COMMON "../common/common_func.c" "../common/common_check.cpp" "${BSP_MSP_DIR}/../sample/common/*.c" "${BSP_MSP_DIR}/../sample/common/common_codec/*.c" "${BSP_MSP_DIR}/../sample/common/vo/*.c")
endif()
    # file(GLOB SRC_LIST_RTSP "../rtsp/src/*.c")
list(APPEND SRC_BASE_LIST
    ${SRC_LIST}
    ${SRC_LIST_COMMON}
    # ${SRC_LIST_RTSP}
)

add_executable(${SampleName}
    ${SRC_BASE_LIST}
)
ax_include_link(${SampleName} "vin")
install(TARGETS ${SampleName} DESTINATION bin)


set(SampleName ${SampleName}_sipy)

if (SIPY_BUILD)
    add_library(${SampleName} SHARED
        ${SRC_BASE_LIST}
    )
    SET_TARGET_PROPERTIES(${SampleName} PROPERTIES VERSION ${SIPY_VERSION} SOVERSION ${SIPY_SOVERSION})
    SET_TARGET_PROPERTIES(${SampleName} PROPERTIES OUTPUT_NAME ${SampleName})
    ax_include_link(${SampleName} "vin")
    install(TARGETS ${SampleName} DESTINATION lib)
endif()
