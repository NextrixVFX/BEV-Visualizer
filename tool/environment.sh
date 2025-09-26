#!/bin/bash

export TensorRT_Lib=/home/nextrix/code/TensorRT-8.5.1.7/lib
export TensorRT_Inc=/home/nextrix/code/TensorRT-8.5.1.7/include
export TensorRT_Bin=/home/nextrix/code/TensorRT-8.5.1.7/bin

export CUDA_Lib=/usr/local/cuda/lib64
export CUDA_Inc=/usr/local/cuda/include
export CUDA_Bin=/usr/local/cuda/bin
export CUDA_HOME=/usr/local/cuda/

export CUDNN_Lib=/usr/local/cuda/

# resnet50/resnet50int8/swint
export DEBUG_MODEL=resnet18int8

# fp16/int8
export DEBUG_PRECISION=int8
export DEBUG_DATA=example-data

export ConfigurationStatus=Failed
if [ ! -f "${TensorRT_Bin}/trtexec" ]; then
    echo "Can not find ${TensorRT_Bin}/trtexec, there may be a mistake in the directory you configured."
    return
fi

if [ ! -f "${CUDA_Bin}/nvcc" ]; then
    echo "Can not find ${CUDA_Bin}/nvcc, there may be a mistake in the directory you configured."
    return
fi

echo "=========================================================="
echo "||  MODEL: $DEBUG_MODEL"
echo "||  PRECISION: $DEBUG_PRECISION"
echo "||  DATA: $DEBUG_DATA"
echo "||"
echo "||  TensorRT: $TensorRT_Lib"
echo "||  CUDA: $CUDA_HOME"
echo "||  CUDNN: $CUDNN_Lib"
echo "=========================================================="

BuildDirectory=`pwd`/build

export PATH=$TensorRT_Bin:$CUDA_Bin:$PATH
export LD_LIBRARY_PATH=$TensorRT_Lib:$CUDA_Lib:$CUDNN_Lib:$BuildDirectory:$LD_LIBRARY_PATH
export ConfigurationStatus=Success
export CUDASM="89"

echo Configuration done!