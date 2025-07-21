set(SampleName common_pipeline)

function(targetlink name)
target_include_directories(${name} PRIVATE ${OpenCV_INCLUDE_DIRS})
    target_include_directories(${name} PRIVATE .)
    target_include_directories(${name} PRIVATE ../utilities)
    target_include_directories(${name} PRIVATE ${BSP_MSP_DIR}/include)
    target_include_directories(${name} PRIVATE ${BSP_MSP_DIR}/arm_glibc/include)
    target_include_directories(${name} PRIVATE ${BSP_MSP_DIR}/arm64_glibc/include)
    target_include_directories(${name} PRIVATE ${BSP_MSP_DIR}/../sample/common)
endfunction()

if(AXERA_TARGET_CHIP MATCHES "AX650")
    file(GLOB SRC_LIST "ax650/*.cpp" "../../utilities/ax_version_check.cpp")
elseif(AXERA_TARGET_CHIP MATCHES "AX620E")
    file(GLOB SRC_LIST "ax620e/*.cpp" 
                    "${BSP_MSP_DIR}/../sample/common/common_venc.c" 
                    "${BSP_MSP_DIR}/../sample/common/common_venc_log.c" 
                    "../../utilities/ax_version_check.cpp")
endif()

list(APPEND SRC_BASE_LIST
    ${SRC_LIST}
)

add_compile_options(-fPIC)

add_library(${SampleName} STATIC
    ${SRC_BASE_LIST}
)
targetlink(${SampleName})
install(TARGETS ${SampleName} DESTINATION lib)