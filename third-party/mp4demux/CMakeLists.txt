project(Mp4Demuxer)

set(LIBRARY_NAME Mp4Demuxer)

# check CMake version
cmake_minimum_required(VERSION 3.13 FATAL_ERROR)

file(GLOB SRC_LIST
    "Mp4Demuxer.cpp")

add_library(${LIBRARY_NAME} STATIC
    ${SRC_LIST}
)
include_directories(include)
# add_definitions(-DLINUX)
add_definitions(-fPIC -std=c++17)

install(TARGETS ${LIBRARY_NAME} DESTINATION lib)
file(GLOB HEADER Mp4Demuxer.h)
install(FILES ${HEADER} DESTINATION include/Mp4Demuxer)
