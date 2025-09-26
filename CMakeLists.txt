#!/bin/bash
cmake_minimum_required(VERSION 3.5)
project(fastbev)

option(CUDA_ENABLED "Enable CUDA support" ON)

set(CMAKE_BUILD_TYPE "Release")
# set(CMAKE_BUILD_TYPE "Debug")
set(arch ${CMAKE_HOST_SYSTEM_PROCESSOR})

find_package(CUDA REQUIRED)
find_package(Protobuf REQUIRED)
find_package(OpenCV)

set(CMAKE_CXX_FLAGS_RELEASE "-std=c++14 -Wextra -Wall -Wno-missing-field-initializers -Wno-deprecated-declarations -O3 -DENABLE_TEXT_BACKEND_STB")
set(CMAKE_CXX_FLAGS_DEBUG   "-std=c++14 -O0 -g -DENABLE_TEXT_BACKEND_STB")

set(CUDA_NVCC_FLAGS_RELEASE "-Werror=all-warnings -Xcompiler -std=c++14,-Wextra,-Wall,-Wno-deprecated-declarations,-O3 -DENABLE_TEXT_BACKEND_STB")
set(CUDA_NVCC_FLAGS_DEBUG   "-std=c++14 -O0 -g -DENABLE_TEXT_BACKEND_STB")
set(CUDA_NVCC_FLAGS ${CUDA_NVCC_FLAGS}
  -gencode arch=compute_89,code=compute_89
)

include_directories(
  ${Protobuf_INCLUDE_DIRS}
  $ENV{CUDA_Inc}
  $ENV{TensorRT_Inc}
  # ${OpenCV_INCLUDE_DIRS}
  dependencies/stb
  libraries/cuOSD/src
  libraries/3DSparseConvolution/libspconv/include
  src/common
  src/
)

link_directories(
  $ENV{CUDA_Lib}
  $ENV{TensorRT_Lib}
  libraries/3DSparseConvolution/libspconv/lib/${arch}
  build
)

file(GLOB_RECURSE CORE_FILES 
  src/fastbev/*.cu 
  src/fastbev/*.cpp
  src/common/tensor.cu
  src/common/tensorrt.cpp
)

file(GLOB_RECURSE VISUALIZE_FILES 
  libraries/cuOSD/src/*.cpp
  libraries/cuOSD/src/*.cu
  src/common/visualize.cu
)

####################### libfastbev_core.so ##########################
cuda_add_library(fastbev_core SHARED 
  ${CORE_FILES}
        src/server/udp.hpp
)

target_link_libraries(fastbev_core
  libcudart.so
  libcublasLt.so
  libnvinfer.so
  # libspconv.so
  ${Protobuf_LIBRARIES}
  libnvinfer_plugin.so
  ${OpenCV_LIBS}
)
#######################################################################


####################### fastbev program #############################
cuda_add_executable(fastbev 
  src/main.cpp
  ${VISUALIZE_FILES}
        src/server/udp.hpp
        src/server/udp.cpp
        src/common/Types.hpp
)

target_link_libraries(fastbev
  fastbev_core
  libnvinfer.so
  # libspconv.so
  ${Protobuf_LIBRARIES}
  libnvinfer_plugin.so
  ${OpenCV_LIBS}
)
#######################################################################