set(SampleName sample_multi_demux_ivps_npu_multi_rtsp)

file(GLOB SRC_BASE_LIST "*.cpp" "../utilities/*.c")
# 
if(AXERA_TARGET_CHIP MATCHES "AX650" OR AXERA_TARGET_CHIP MATCHES "AX620E")
    file(GLOB SRC_LIST_COMMON   "${BSP_MSP_DIR}/../sample/common/common_sys.c" 
                                # "${BSP_MSP_DIR}/../sample/common/vo/*.c"
                                "../common/common_check.cpp")
endif()
list(APPEND SRC_BASE_LIST
    ${SRC_LIST}
    ${SRC_LIST_COMMON}
)

add_executable(${SampleName}
    ${SRC_BASE_LIST}
)
ax_include_link(${SampleName} "demux")
install(TARGETS ${SampleName} DESTINATION bin)
