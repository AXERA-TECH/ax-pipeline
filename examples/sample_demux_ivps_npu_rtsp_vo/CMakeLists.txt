set(SampleName sample_demux_ivps_npu_rtsp_vo)

file(GLOB SRC_BASE_LIST "*.cpp" "../utilities/*.c")
file(GLOB SRC_LIST_COMMON   "${BSP_MSP_DIR}/../sample/common/common_sys.c"
                            "${BSP_MSP_DIR}/../sample/common/vo/*.c"
                            "../common/common_check.cpp")
# file(GLOB SRC_LIST_RTSP "../rtsp/src/*.c")
list(APPEND SRC_BASE_LIST
    ${SRC_LIST}
    ${SRC_LIST_COMMON}
    # ${SRC_LIST_RTSP}
)

add_executable(${SampleName}
    ${SRC_BASE_LIST}
)
ax_include_link(${SampleName} "demux")
install(TARGETS ${SampleName} DESTINATION bin)