project(CameraStreamer/streaming)

cmake_minimum_required(VERSION 2.6)

if(COMMAND cmake_policy)
  cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

include_directories(${CMAKE_SOURCE_DIR}/nvidia/include)
LINK_DIRECTORIES(/usr/lib/${CMAKE_LIBRARY_ARCHITECTURE}/tegra)
set(CAMERA_DEPS imx377 nvidia_util v4l2 nvbuf_utils argus) 

find_package(PkgConfig)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0)
message(STATUS "GStreamer include: ${GSTREAMER_INCLUDE_DIRS}")
message(STATUS "GStreamer lib: ${GSTREAMER_LIBRARIES}")
include_directories( ${GSTREAMER_INCLUDE_DIRS} )

include_directories(../include)

add_library(imx377 STATIC
        imx377camera.cpp)

add_library(logger STATIC
        log.cpp)

add_library(stepper STATIC
        motordriver.cpp
)

add_executable(camerastreamer
		camerastreamer.cpp
		datasource.cpp
		datasource_camera.cpp
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

add_executable(testMotor test_motor.cpp)
target_link_libraries(testMotor
    stepper
    logger
    ${BOOST_DEPS}
)

install(TARGETS testMotor DESTINATION usr/local/bin COMPONENT camerastreamer)

