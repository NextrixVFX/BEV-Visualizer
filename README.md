# BEV-Visualizer
- Fast Tesla-AI Style BEV Model + Visualizer. Built with TensorRT and Unity.
- Note: This uses strictly cameras. No lidar needed; Lidar is used for training, not inference.

# Requirements
* Most requirements can be found on the original repo for [CUDA-FastBev](https://github.com/Mandylove1993/CUDA-FastBEV). However, some information is left out.
- TensorRT v8.5.1.7
- Cudnn v8.2
- Cuda v11.8
- libprotobuf-dev v3.6.1
- python3.10 (optional)
- [Lidar Solution](https://github.com/NVIDIA-AI-IOT/Lidar_AI_Solution)
- [Extras](https://files.nextrix.xyz/share/g5QqMsTF) (mp4s, models, deps)

# My Setup
* I'm running the model on a WSL2 container running Ubuntu 22.04
* My host machine is running Windows11 24H2
* The requirements I put is based off what I have installed (on wsl2) precisely.

# How to use
1. Follow through with the [original repo](https://github.com/Mandylove1993/CUDA-FastBEV) first but use the modified ```.sh``` files in this repo rather than cuda-fastbev (if you have problems)
2. Set your proper [Compute Capability](https://developer.nvidia.com/cuda-gpus#compute) at the bottom of ```tool/environment.sh``` at ```ln:62 ; export CUDASM="89"``` (change 89 to ur gpu)
3. Make sure you put both the ```libraries``` and ```dependencies``` folders (from [Lidar Solution](https://github.com/NVIDIA-AI-IOT/Lidar_AI_Solution))  inside the BEV-Visualizer folder.
4. do ```mkdir code``` in ur user folder and drag in tensorrt tar.gz (not deb) + extract
5. Follow instructions for cuda installation on nvidia's website
6. Download cudnn, unzip, and drag its contents into your cuda folder
7. Your workspace should look like this:
   ```
   # $HOME/Bev-Visualizer
   nextrix@john:~/BEV-Visualizer$ ls
    CMakeLists.txt   README.md    dependencies    example-data    libraries    model   src   tool

   # $HOME/code/TensorRT-8.5.1.7
   nextrix@john:~/code$ ls
    TensorRT-8.5.1.7

   # Merged Cudnn with Cuda
   nextrix@john:/usr/local/cuda$ ls
    DOCS      README  compute-sanitizer  extras  include  libnvvp           nvml  share  targets  version.json
    EULA.txt  bin     doc                gds     lib64    nsightee_plugins  nvvm  src    tools
   ```
8. Run ```bash tool/build_trt_engine.sh``` and wait for it to build both paths
9. After their both built, run ```bash tool/run.sh``` to start inferencing

# Project Structure
```

# nuScenes
nextrix@john:~/BEV-Visualizer/example-data$ ls
  0-FRONT.jpg        2-FRONT_LEFT.jpg  4-BACK_LEFT.jpg   Videos          example-data.pth  valid_c_idx.tensor  y.tensor
  1-FRONT_RIGHT.jpg  3-BACK.jpg        5-BACK_RIGHT.jpg  anchors.tensor  images.tensor     x.tensor
nextrix@john:~/BEV-Visualizer/example-data$ ls Videos/
  CAM_BACK.mp4  CAM_BACK_LEFT.mp4  CAM_BACK_RIGHT.mp4  CAM_FRONT.mp4  CAM_FRONT_LEFT.mp4  CAM_FRONT_RIGHT.mp4

# model
nextrix@john:~/BEV-Visualizer/model$ ls
  resnet18  resnet18int8  resnet18int8head
nextrix@john:~/CUDA-FastBEV/model$ ls resnet18int8
  fastbev_post_trt_decode.onnx  fastbev_pre_trt.onnx  fastbev_ptq.pth  build
nextrix@john:~/CUDA-FastBEV/model$ ls resnet18int8/build/
  fastbev_post_trt_decode.json  fastbev_post_trt_decode.plan  fastbev_pre_trt.log
  fastbev_post_trt_decode.log   fastbev_pre_trt.json          fastbev_pre_trt.plan

# Libs and Deps
nextrix@john:~/BEV-Visualizer/libraries$ ls
  3DSparseConvolution  cuOSD   spconv
nextrix@john:~/CUDA-FastBEV/dependencies$ ls
  dlpack  stb

# Tools
nextrix@john:~/BEV-Visualizer/tool$ ls
  build_trt_engine.sh  environment.sh  run.sh
```

# Performance (Laptop 4070) [Resnet18int8 -- fp16]
```
=== Inference Statistics ===
Total frames processed: 404
Average inference time: 10.374 ms
Min inference time: 8.4918 ms
Max inference time: 171.994 ms
Average FPS: 96.3948
```

# Plans
- General optimizations like how content is loaded
- Optimize memory management
- Use cheaper methods for simular quality
- Deploy on a 2022 Tesla Model 3 LR w/ nVidia Orin
  
# Acknowledgements
[CUDA-FastBev](https://github.com/Mandylove1993/CUDA-FastBEV)
