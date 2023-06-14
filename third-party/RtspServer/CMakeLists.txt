project(RtspServer)

set(LIBRARY_NAME RtspServer)

# check CMake version
cmake_minimum_required(VERSION 3.13 FATAL_ERROR)

file(GLOB SRC_LIST
    "RtspServer/src/net/*.cpp"
    "RtspServer/src/xop/*.cpp"
    "RtspServerWarpper.cpp"
    "rtsp/src/*.c")

add_library(${LIBRARY_NAME} STATIC
    ${SRC_LIST}
)

# add_definitions(-DLINUX)
add_definitions(-fPIC)

if(RTSP_SERVER_VERSION MATCHES "OLD" OR RTSP_SERVER_VERSION MATCHES "old" )
    message(STATUS "build old version rtsp server")
    add_definitions(-URTSP_SERVER_PHZ76)
else()
    message(STATUS "build phz76 version rtsp server")
    #define RTSP_SERVER_PHZ76
    add_definitions(-DRTSP_SERVER_PHZ76)
ENDIF()

include_directories(${LIBRARY_NAME} PRIVATE RtspServer/src)
include_directories(${LIBRARY_NAME} PRIVATE RtspServer/src/3rdpart)
include_directories(${LIBRARY_NAME} PRIVATE rtsp/inc)

add_executable(rtsp_h264_file
    RtspServer/example/rtsp_h264_file.cpp
)
add_executable(rtsp_pusher
    RtspServer/example/rtsp_pusher.cpp
)

add_executable(rtsp_server
    RtspServer/example/rtsp_server.cpp
)

target_link_libraries(rtsp_h264_file RtspServer pthread)
target_link_libraries(rtsp_pusher RtspServer pthread)
target_link_libraries(rtsp_server RtspServer pthread)

install(TARGETS ${LIBRARY_NAME} DESTINATION lib)
# install(TARGETS rtsp_h264_file DESTINATION bin)
# install(TARGETS rtsp_pusher DESTINATION bin)
# install(TARGETS rtsp_server DESTINATION bin)
file(GLOB HEADER RtspServerWarpper.h)
install(FILES ${HEADER} DESTINATION include/libRtspServer)
