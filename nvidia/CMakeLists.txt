CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
project(nvidia)

include_directories(../include ${CMAKE_SOURCE_DIR}/bllibs/c++/include include include/libjpeg-8b)

add_library(nvidia_util
    NvElementProfiler.cpp
    NvElement.cpp
    NvApplicationProfiler.cpp
    NvVideoDecoder.cpp
    NvBuffer.cpp
    NvJpegEncoder.cpp
    NvVideoConverter.cpp
    NvLogging.cpp
    NvEglRenderer.cpp
    NvUtils.cpp
    NvJpegDecoder.cpp
    NvVideoEncoder.cpp
    NvV4l2ElementPlane.cpp
    NvV4l2Element.cpp
    Thread.cpp)
