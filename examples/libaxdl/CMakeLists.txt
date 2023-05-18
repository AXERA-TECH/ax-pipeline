set(SampleName axdl)



function(targetlink name)
    target_include_directories(${name} PRIVATE ${OpenCV_INCLUDE_DIRS})
    target_include_directories(${name} PRIVATE include)
    target_include_directories(${name} PRIVATE ../utilities)
    target_include_directories(${name} PRIVATE ${BSP_MSP_DIR}/include)
    target_include_directories(${name} PRIVATE ${BSP_MSP_DIR}/include/npu_cv_kit)
    target_include_directories(${name} PRIVATE ${CMAKE_SOURCE_DIR}/third-party/ByteTrack/include)
endfunction()

file(GLOB SRC_LIST "src/*.cpp" "src/utilities/*.cpp")

list(APPEND SRC_BASE_LIST
    ${SRC_LIST}
)

add_compile_options(-fPIC -fopenmp)

add_library(${SampleName} STATIC
    ${SRC_BASE_LIST}
)
targetlink(${SampleName})
install(TARGETS ${SampleName} DESTINATION lib)