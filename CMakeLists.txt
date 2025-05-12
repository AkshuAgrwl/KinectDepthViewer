cmake_minimum_required(VERSION 3.15)
project(KinectDepthViewer)

set(CMAKE_CXX_STANDARD 17)

set(KINECT_SDK_INC "C:/Program Files/Microsoft SDKs/Kinect/v1.8/inc")
set(KINECT_SDK_LIB "C:/Program Files/Microsoft SDKs/Kinect/v1.8/lib/amd64")

add_executable(KinectDepthViewer src/main.cpp)
set_target_properties(KinectDepthViewer PROPERTIES WIN32_EXECUTABLE TRUE)

target_include_directories(KinectDepthViewer PRIVATE
    ${KINECT_SDK_INC}
)

target_link_directories(KinectDepthViewer PRIVATE
    ${KINECT_SDK_LIB}
)

target_link_libraries(KinectDepthViewer
    Kinect10
)

if (MSVC)
    target_compile_definitions(KinectDepthViewer PRIVATE)
endif()
