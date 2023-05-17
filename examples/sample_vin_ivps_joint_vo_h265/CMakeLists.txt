set(SampleName sample_vin_ivps_joint_vo_h265)

include(../build_func.cmake)

file(GLOB SRC_BASE_LIST "*.cpp" "../utilities/*.c")
file(GLOB SRC_LIST_COMMON "../common/common_func.c" "${BSP_MSP_DIR}/../sample/common/*.c" "${BSP_MSP_DIR}/../sample/common/common_codec/*.c" "${BSP_MSP_DIR}/../sample/common/vo/*.c")
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
