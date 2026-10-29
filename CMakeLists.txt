cmake_minimum_required(VERSION 3.10)
project(turret_vision)
set(CMAKE_C_COMPILER "arm-rockchip830-linux-uclibcgnueabihf-gcc")
set(CMAKE_CXX_COMPILER "arm-rockchip830-linux-uclibcgnueabihf-g++")
set(CMAKE_CXX_STANDARD 14)

include_directories( )
link_directories( )

add_executable(turret_vision main.cpp)
target_link_libraries(turret_vision rknnrt opencv_core opencv_videoio opencv_imgproc opencv_imgcodecs pthread)
