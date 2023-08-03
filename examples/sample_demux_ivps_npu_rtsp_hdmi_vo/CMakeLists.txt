set(SampleName sample_demux_ivps_npu_rtsp_hdmi_vo)

file(GLOB SRC_BASE_LIST "*.cpp" "../utilities/*.c")
file(GLOB SRC_LIST_COMMON   "${BSP_MSP_DIR}/../sample/common/common_sys.c"
                            "${BSP_MSP_DIR}/../sample/common/vo/*.c")
list(APPEND SRC_BASE_LIST
    ${SRC_LIST}
    ${SRC_LIST_COMMON}
)

add_executable(${SampleName}
    ${SRC_BASE_LIST}
)
ax_include_link(${SampleName} "demux")
install(TARGETS ${SampleName} DESTINATION bin)


set(SampleName ${SampleName}_sipy)

if (SIPY_BUILD)
    add_library(${SampleName} SHARED
        ${SRC_BASE_LIST}
    )
    SET_TARGET_PROPERTIES(${SampleName} PROPERTIES VERSION ${SIPY_VERSION} SOVERSION ${SIPY_SOVERSION})
    SET_TARGET_PROPERTIES(${SampleName} PROPERTIES OUTPUT_NAME ${SampleName})
    ax_include_link(${SampleName} "demux")
    install(TARGETS ${SampleName} DESTINATION lib)
endif()
