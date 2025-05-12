cmake_minimum_required(VERSION 3.15)
project(KinectDepthViewer)

set(CMAKE_CXX_STANDARD 17)

add_executable(KinectDepthViewer src/main.cpp)
set_target_properties(KinectDepthViewer PROPERTIES WIN32_EXECUTABLE TRUE)

if (MSVC)
    target_compile_definitions(KinectDepthViewer PRIVATE)
endif()
