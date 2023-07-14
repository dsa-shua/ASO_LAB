wget https://github.com/onnx/models/raw/b9a54e89508f101a1611cd64f4ef56b9cb62c7cf/vision/classification/resnet/model/resnet50-v2-7.onnx

# This may take several minutes depending on your machine
tvmc compile \
--target "llvm" \
--input-shapes "data:[1,3,224,224]" \
--output resnet50-v2-7-tvm.tar \
resnet50-v2-7.onnx

mkdir model
tar -xvf resnet50-v2-7-tvm.tar -C model
ls model

python preprocess.py

tvmc run \
--inputs imagenet_cat.npz \
--output predictions.npz \
resnet50-v2-7-tvm.tar

python postprocess.py
# class='n02123045 tabby, tabby cat' with probability=0.610553
# class='n02123159 tiger cat' with probability=0.367179
# class='n02124075 Egyptian cat' with probability=0.019365
# class='n02129604 tiger, Panthera tigris' with probability=0.001273
# class='n04040759 radiator' with probability=0.000261

# The default search algorithm requires xgboost, see below for further
# details on tuning search algorithms
pip install xgboost

tvmc tune \
--target "llvm" \
--output resnet50-v2-7-autotuner_records.json \
resnet50-v2-7.onnx

tvmc tune \
--target "llvm -mcpu=broadwell" \
--output resnet50-v2-7-autotuner_records.json \
resnet50-v2-7.onnx
# [Task  1/24]  Current/Best:    9.65/  23.16 GFLOPS | Progress: (60/1000) | 130.74 s Done.
# [Task  1/24]  Current/Best:    3.56/  23.16 GFLOPS | Progress: (192/1000) | 381.32 s Done.
# [Task  2/24]  Current/Best:   13.13/  58.61 GFLOPS | Progress: (960/1000) | 1190.59 s Done.
# [Task  3/24]  Current/Best:   31.93/  59.52 GFLOPS | Progress: (800/1000) | 727.85 s Done.
# [Task  4/24]  Current/Best:   16.42/  57.80 GFLOPS | Progress: (960/1000) | 559.74 s Done.
# [Task  5/24]  Current/Best:   12.42/  57.92 GFLOPS | Progress: (800/1000) | 766.63 s Done.
# [Task  6/24]  Current/Best:   20.66/  59.25 GFLOPS | Progress: (1000/1000) | 673.61 s Done.
# [Task  7/24]  Current/Best:   15.48/  59.60 GFLOPS | Progress: (1000/1000) | 953.04 s Done.
# [Task  8/24]  Current/Best:   31.97/  59.33 GFLOPS | Progress: (972/1000) | 559.57 s Done.
# [Task  9/24]  Current/Best:   34.14/  60.09 GFLOPS | Progress: (1000/1000) | 479.32 s Done.
# [Task 10/24]  Current/Best:   12.53/  58.97 GFLOPS | Progress: (972/1000) | 642.34 s Done.
# [Task 11/24]  Current/Best:   30.94/  58.47 GFLOPS | Progress: (1000/1000) | 648.26 s Done.
# [Task 12/24]  Current/Best:   23.66/  58.63 GFLOPS | Progress: (1000/1000) | 851.59 s Done.
# [Task 13/24]  Current/Best:   25.44/  59.76 GFLOPS | Progress: (1000/1000) | 534.58 s Done.
# [Task 14/24]  Current/Best:   26.83/  58.51 GFLOPS | Progress: (1000/1000) | 491.67 s Done.
# [Task 15/24]  Current/Best:   33.64/  58.55 GFLOPS | Progress: (1000/1000) | 529.85 s Done.
# [Task 16/24]  Current/Best:   14.93/  57.94 GFLOPS | Progress: (1000/1000) | 645.55 s Done.
# [Task 17/24]  Current/Best:   28.70/  58.19 GFLOPS | Progress: (1000/1000) | 756.88 s Done.
# [Task 18/24]  Current/Best:   19.01/  60.43 GFLOPS | Progress: (980/1000) | 514.69 s Done.
# [Task 19/24]  Current/Best:   14.61/  57.30 GFLOPS | Progress: (1000/1000) | 614.44 s Done.
# [Task 20/24]  Current/Best:   10.47/  57.68 GFLOPS | Progress: (980/1000) | 479.80 s Done.
# [Task 21/24]  Current/Best:   34.37/  58.28 GFLOPS | Progress: (308/1000) | 225.37 s Done.
# [Task 22/24]  Current/Best:   15.75/  57.71 GFLOPS | Progress: (1000/1000) | 1024.05 s Done.
# [Task 23/24]  Current/Best:   23.23/  58.92 GFLOPS | Progress: (1000/1000) | 999.34 s Done.
# [Task 24/24]  Current/Best:   17.27/  55.25 GFLOPS | Progress: (1000/1000) | 1428.74 s Done.

tvmc compile \
--target "llvm" \
--tuning-records resnet50-v2-7-autotuner_records.json  \
--output resnet50-v2-7-tvm_autotuned.tar \
resnet50-v2-7.onnx


tvmc run \
--inputs imagenet_cat.npz \
--output predictions.npz \
resnet50-v2-7-tvm_autotuned.tar

python postprocess.py

# class='n02123045 tabby, tabby cat' with probability=0.610550
# class='n02123159 tiger cat' with probability=0.367181
# class='n02124075 Egyptian cat' with probability=0.019365
# class='n02129604 tiger, Panthera tigris' with probability=0.001273
# class='n04040759 radiator' with probability=0.000261

tvmc run \
--inputs imagenet_cat.npz \
--output predictions.npz  \
--print-time \
--repeat 100 \
resnet50-v2-7-tvm_autotuned.tar

# Execution time summary:
# mean (ms)   max (ms)    min (ms)    std (ms)
#     92.19     115.73       89.85        3.15

tvmc run \
--inputs imagenet_cat.npz \
--output predictions.npz  \
--print-time \
--repeat 100 \
resnet50-v2-7-tvm.tar

# Execution time summary:
# mean (ms)   max (ms)    min (ms)    std (ms)
#    193.32     219.97      185.04        7.11


