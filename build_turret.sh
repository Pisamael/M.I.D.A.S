#!/bin/bash
export PATH=$PWD/../luckfox-pico/tools/linux/toolchain/arm-rockchip830-linux-uclibcgnueabihf/bin:$PATH

RKNN_INC=$(find ../luckfox_pico_rknn_example/3rdparty -type d -name "include" | grep librknn_api | head -n 1)
RKNN_LIB=$(find ../luckfox_pico_rknn_example/3rdparty -type d -name "armhf" | grep librknn_api | head -n 1)
CV_INC=$(find ../luckfox_pico_rknn_example/3rdparty -type d -name "include" | grep opencv | head -n 1)
CV_LIB=$(find ../luckfox_pico_rknn_example/3rdparty -type d -name "lib" | grep opencv | grep armhf | head -n 1)

cat << CMAKE_EOF > CMakeLists.txt
cmake_minimum_required(VERSION 3.10)
project(turret_vision)
set(CMAKE_C_COMPILER "arm-rockchip830-linux-uclibcgnueabihf-gcc")
set(CMAKE_CXX_COMPILER "arm-rockchip830-linux-uclibcgnueabihf-g++")
set(CMAKE_CXX_STANDARD 14)

include_directories(${RKNN_INC} ${CV_INC})
link_directories(${RKNN_LIB} ${CV_LIB})

add_executable(turret_vision main.cpp)
target_link_libraries(turret_vision rknnrt opencv_core opencv_videoio opencv_imgproc opencv_imgcodecs pthread)
CMAKE_EOF

cd build
cmake ..
make -j4
