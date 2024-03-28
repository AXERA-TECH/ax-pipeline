set(SampleName sample_demux_ivps_npu_rtsp)

file(GLOB SRC_BASE_LIST "*.cpp" "../utilities/*.c")

if(AXERA_TARGET_CHIP MATCHES "AX650" OR AXERA_TARGET_CHIP MATCHES "AX620E")
    file(GLOB SRC_LIST_COMMON   "${BSP_MSP_DIR}/../sample/common/common_sys.c" 
                                # "${BSP_MSP_DIR}/../sample/common/vo/*.c"
                                "../common/common_check.cpp")
# elseif(AXERA_TARGET_CHIP MATCHES "AX620E")
#     file(GLOB SRC_LIST_COMMON   "${BSP_MSP_DIR}/../sample/common/common_sys.c" 
#                                 # "${BSP_MSP_DIR}/../sample/common/vo/*.c"
#                                 "../common/common_check.cpp")
elseif(AXERA_TARGET_CHIP MATCHES "AX620A")
    file(GLOB SRC_LIST_COMMON   "${BSP_MSP_DIR}/../sample/common/common_sys.c" 
                                "${BSP_MSP_DIR}/../sample/common/vo/*.c"
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
