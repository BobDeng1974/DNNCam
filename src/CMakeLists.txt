project(CameraStreamer/streaming)

cmake_minimum_required(VERSION 2.6)

if(COMMAND cmake_policy)
  cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

include_directories(${CMAKE_SOURCE_DIR}/nvidia/include)
LINK_DIRECTORIES(/usr/lib/${CMAKE_LIBRARY_ARCHITECTURE}/tegra)
set(CAMERA_DEPS nvidia_util v4l2 nvbuf_utils argus) 

find_package(PkgConfig)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0)
message(STATUS "GStreamer include: ${GSTREAMER_INCLUDE_DIRS}")
message(STATUS "GStreamer lib: ${GSTREAMER_LIBRARIES}")
include_directories( ${GSTREAMER_INCLUDE_DIRS} )

include_directories(../include)

set(XMLRPC_DEPS
    xmlrpc_xmltok
        xmlrpc_xmlparse
        xmlrpc_util
        xmlrpc
        xmlrpc++
        xmlrpc_server++
        xmlrpc_abyss
        xmlrpc_server
        xmlrpc_server_abyss
        xmlrpc_server_abyss++
)

add_library(logger STATIC
        log.cpp)

add_library(stepper STATIC
        motordriver.cpp
)

add_library(config STATIC
        configuration.cpp
)

add_executable(camerastreamer
		camerastreamer.cpp
        ArgusCamera.cpp
		frame_processor.cpp
		stream.cpp
)

target_link_libraries(camerastreamer
		${CAMERA_DEPS}
		logger
		${BOOST_DEPS}
		${OPENCV_DEPS}
		${STREAMER_DEPS}
		${GSTREAMER_LIBRARIES}
		${ARGUS_DEPS}
		gstrtspserver-1.0
)

install(TARGETS camerastreamer DESTINATION usr/local/bin COMPONENT camerastreamer)

add_executable(lensDriver lensDriver.cpp)
target_link_libraries(lensDriver
    stepper
    logger
    config 
    ${BOOST_DEPS}
    ${XMLRPC_DEPS}
    readline
)

install(TARGETS lensDriver DESTINATION usr/local/bin COMPONENT camerastreamer)

