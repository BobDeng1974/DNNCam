project(DNNCam)

cmake_minimum_required(VERSION 2.6)

if(COMMAND cmake_policy)
  cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

set(CMAKE_FIND_LIBRARY_SUFFIXES ".so;.a")

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_EXPORT_COMPILE_COMMANDS "ON")

set(OPENCV_DEPS
  opencv_highgui
  opencv_video
  opencv_imgproc
  opencv_ml
  opencv_core
  gthread-2.0
  )
if(EXISTS /usr/include/va)
  set(OPENCV_DEPS ${OPENCV_DEPS} va)
endif(EXISTS /usr/include/va)

# Set up OpenCV header directory. A default is suggested below,
# but the variable gets cached so user can change it in cmake-gui.
#set(OPENCV_INCLUDE /opt/OpenCV/opencv-2.4.8/install/include CACHE FILEPATH "Path to OpenCV include directory")
set(OPENCV_INCLUDE /usr/include/opencv2/ CACHE FILEPATH "Path to OpenCV include directory")
include_directories(${OPENCV_INCLUDE})
if(NOT EXISTS ${OPENCV_INCLUDE})
  message(FATAL_ERROR "OPENCV_INCLUDE directory does not exist.")
endif()

# Set up OpenCV library directory. A default is suggested below,
# but the variable gets cached so user can change it in cmake-gui.
set(OPENCV_LIBDIR /usr/lib/${CMAKE_LIBRARY_ARCHITECTURE}/ CACHE FILEPATH "Path to OpenCV lib directory")
link_directories(${OPENCV_LIBDIR})
if(NOT EXISTS ${OPENCV_LIBDIR})
  message(FATAL_ERROR "OPENCV_LIB directory does not exist.")
endif()

add_definitions(-D_FILE_OFFSET_BITS=64 -DUSE_BOOST_FILESYSTEM_3 -DBL_TIMER_BACKEND=BL_TIMER_BACKEND_USEC)

set(BOOST_DEPS
    boost_program_options
    boost_thread
    boost_date_time
    boost_serialization
    boost_filesystem
    boost_system
    pthread)

set( CMAKE_EXE_LINKER_FLAGS ${BOOST_LIBRARY} )

add_subdirectory(nvidia)
add_subdirectory(src)

install(PROGRAMS ${files} DESTINATION usr/local/camerastreamer/bin COMPONENT camerastreamer)
install(FILES ${PROJECT_SOURCE_DIR}/package/camerastreamer.service DESTINATION etc/systemd/system COMPONENT camerastreamer)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/web/finder.css
  ${CMAKE_CURRENT_SOURCE_DIR}/web/index.html
  DESTINATION /var/www/html)
FILE(GLOB images "${CMAKE_CURRENT_SOURCE_DIR}/web/images/*.png")
install(FILES ${images} DESTINATION /var/www/html/images)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/cgi-bin/list ${CMAKE_CURRENT_SOURCE_DIR}/cgi-bin/delete ${CMAKE_CURRENT_SOURCE_DIR}/cgi-bin/rpc ${CMAKE_CURRENT_SOURCE_DIR}/cgi-bin/temps ${CMAKE_CURRENT_SOURCE_DIR}/cgi-bin/setroi DESTINATION /usr/lib/cgi-bin PERMISSIONS WORLD_READ WORLD_EXECUTE)

include("Version.cmake")

SET(CPACK_PACKAGE_NAME "dnncam")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "DNNCam convenience library")
set(CPACK_GENERATOR "DEB")
SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "andreas@boulderlabs.com")
SET(CPACK_PACKAGE_VERSION "${VERSION}")
SET(CPACK_PACKAGE_VERSION_MAJOR "${VERSION_MAJOR}")
SET(CPACK_PACKAGE_VERSION_MINOR "${VERSION_MINOR}")
SET(CPACK_PACKAGE_VERSION_PATCH "${VERSION_PATCH}")
SET(CPACK_PACKAGING_INSTALL_PREFIX "/")
SET(CPACK_STRIP_FILES true)
SET(CPACK_DEB_COMPONENT_INSTALL 1)
SET(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${PROJECT_SOURCE_DIR}/package/postinst")

include(CPack)

