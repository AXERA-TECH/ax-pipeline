project(ByteTrack)

set(LIBRARY_NAME ByteTrack)

# check CMake version
cmake_minimum_required(VERSION 3.13 FATAL_ERROR)

file(GLOB SRC_LIST
    "src/*.cpp")

add_library(${LIBRARY_NAME} STATIC
    ${SRC_LIST}
)
include_directories(include)
# add_definitions(-DLINUX)
add_definitions(-fPIC -std=c++17)

install(TARGETS ${LIBRARY_NAME} DESTINATION lib)
file(GLOB HEADER include/bytetrack.h)
install(FILES ${HEADER} DESTINATION include/ByteTrack)
