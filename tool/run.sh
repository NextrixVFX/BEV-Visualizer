#!/bin/bash
. tool/environment.sh

if [ "$ConfigurationStatus" != "Success" ]; then
    echo "Exit due to configure failure."
    exit
fi

set -e

mkdir -p build

cd build
cmake .. -DCUDA_ENABLED=ON -DCUDA_NVCC_FLAGS="--std c++14"
make -j$(( $(nproc) + 1 ))
cd ..

./build/fastbev --video example-data/Videos --model $DEBUG_MODEL --precision $DEBUG_PRECISION




