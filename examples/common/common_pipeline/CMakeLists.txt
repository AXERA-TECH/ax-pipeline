set(SampleName common_pipeline)

function(targetlink name)
    target_include_directories(${name} PRIVATE .)
    target_include_directories(${name} PRIVATE ../utilities)
    target_include_directories(${name} PRIVATE ${BSP_MSP_DIR}/include)
    target_include_directories(${name} PRIVATE ${BSP_MSP_DIR}/../sample/common)
endfunction()

if(AXERA_TARGET_CHIP MATCHES "ax650" OR AXERA_TARGET_CHIP MATCHES "AX650")
    file(GLOB SRC_LIST "ax650/*.cpp")
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
    file(GLOB SRC_LIST "ax620/*.cpp")
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