cmake_minimum_required(VERSION 3.5)
project(version1_0)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "-D__STDC_CONSTANT_MACROS")

find_path(AVCODEC_INCLUDE_DIR libavcodec/avcodec.h)

find_path(AVFORMAT_INCLUDE_DIR libavformat/avformat.h)

find_path(AVUTIL_INCLUDE_DIR libavutil/avutil.h)

find_path(AVDEVICE_INCLUDE_DIR libavdevice/avdevice.h)

find_library(AVFORMAT_LIBRARY avformat)

add_executable(version1_0 main.cpp )
target_include_directories(version1_0 PRIVATE ${AVCODEC_INCLUDE_DIR} ${AVFORMAT_INCLUDE_DIR} ${AVUTIL_INCLUDE_DIR} ${AVDEVICE_INCLUDE_DIR})
target_link_libraries(version1_0 PRIVATE ${AVFORMAT_LIBRARY})

