set(SampleName sample_demux_ivps_npu_hdmi_vo)

file(GLOB SRC_BASE_LIST "*.cpp" "../utilities/*.c")
file(GLOB SRC_LIST_COMMON   "${BSP_MSP_DIR}/../sample/common/common_sys.c"
                            "${BSP_MSP_DIR}/../sample/common/vo/*.c"
                            "../common/common_check.cpp")
list(APPEND SRC_BASE_LIST
    ${SRC_LIST}
    ${SRC_LIST_COMMON}
)

add_executable(${SampleName}
    ${SRC_BASE_LIST}
)
ax_include_link(${SampleName} "demux")
install(TARGETS ${SampleName} DESTINATION bin)

