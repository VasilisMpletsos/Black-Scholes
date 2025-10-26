# Black Scholes FPGA

## Introduction

This is a repository of the work for my final assignment in the UNI for the MSc degree. Because i like trading stocks a lot i decided to implement the simple Black Scholes algorithm on CPU, GPU & FPGA to see gaining advancments!

## Todo:

- [x] Finish CPU implementation
- [x] Get a dataset
- [x] Check that fast CDF approximation is correct
- [x] Python version in order to run on GPU
- [x] Execute cuda version
- [x] Match cuda version to run with my dataset
- [x] Intergrate Xilinx Makefile to mine
- [ ] Add all FPGA code & execute
- [ ] Profiling for the algorithm with various Vitis Settings
- [ ] Contact Papaefstathiou & Aggelos after profiling

## Notes

For CPU comping with gnu compiler g++ and run

```
g++ main.cpp -o black_scholes.o
black_scholes.o
```

```
g++ host.cpp -o host.o
host.o
```

---

To see cuda specifications

```
nvcc gpu_info.cu -o gpu_info.o
./gpu_info.o
```

For cuda compile with nvcc and run

```
nvcc -I./common_cuda/inc BlackScholes.cu BlackScholes_gold.cpp -o black_scholes_cuda.o
./black_scholes_cuda.o
```

To create them all run "make" to start makefile.

## Comparisons

Times are reported in **ms**.

| Run         | Python       | C++          |
| ----------- | ------------ | ------------ |
| 1           | 0.109119     | 0.000544     |
| 2           | 0.108878     | 0.000550     |
| 3           | 0.106630     | 0.000544     |
| 4           | 0.109391     | 0.000545     |
| 5           | 0.107120     | 0.000544     |
| 6           | 0.109008     | 0.000545     |
| 7           | 0.106312     | 0.000546     |
| 8           | 0.108675     | 0.000546     |
| 9           | 0.108699     | 0.000548     |
| 10          | 0.108302     | 0.000548     |
| **Average** | **0,108213** | **0,000546** |

The **Python** implementation is is **198 times more slow** than **C++**!

---

Now the following comparison is between **fast_cdf_approximation** and **normal_cdf** functions:
| Run | Fast Approximation| Normal CDF |
|-------------|-----------|------------|
| 1 | 0.000544 | 0.000617 |
| 2 | 0.000550| 0.000619 |
| 3 | 0.000544| 0.000616 |
| 4 | 0.000545| 0.000615 |
| 5 | 0.000544| 0.000615 |
| 6 | 0.000545| 0.000622 |
| 7 | 0.000546| 0.000618 |
| 8 | 0.000546| 0.000622 |
| 9 | 0.000548| 0.000615 |
| 10 | 0.000548 | 0.000615 |
| **Average** | **0,000546** | **0,000617**|

The fast approximation seems to be **x1.13 times faster**
It would be even more helpfull due the the embedding nature of FPGA

---

Finally the results from GPU side with CUDA are:

```
Executing Black-Scholes GPU kernel 1000 iterations...
Black Scholes GPU() average execution time: 0.001679 msec
Effective memory bandwidth: 10.220369 GB/s
Gigaoptions per second: 0.511018

```

GPU IS <u>**~ x80 times faster**</u> than the best implementation so far on CPU from C++
and **x47833** from Python

---

```
FPGA IS <u>**x77.15 times faster**</u> than the best implementation so far on CPU from C++
and **x47833** from Python
```

### In order to run VITIS

Add the required sources

```
source /opt/xilinx/xrt/setup.sh
source /tools/Xilinx/Vitis/2022.1/settings64.sh
export CPATH="/usr/include/x86_64-linux-gnu/"
export XCL_EMULATION_MODE=sw_emu
```

For Server

```
source /opt/xilinx/xrt/setup.sh

<!-- source /tools/Xilinx/Vitis/2023.1/settings64.sh -->

source /mnt/data2/Vivado2022.2.sh

<!-- source /tools/Xilinx/Vitis/2019.2/settings64.sh -->

export XCL_EMULATION_MODE=sw_emu

```

And one last command that maybe it is not needed if you place the alveo u200 to /tools/Xilinx/Vivado/2022.1/data/xhub/boards/XilinxBoardStore/boards/Xilinx

```

export PLATFORM_REPO_PATHS=/tools/Xilinx/Vivado/2022.1/data/boards/board_files

```

Makefile

```

make all TARGET=sw_emu
make all TARGET=hw

```

Copy build

```

cp build_dir.sw_emu.xilinx_u200_gen3x16_xdma_2_202110_1/mmul.xclbin ./
export XCL_EMULATION_MODE=sw_emu

```

Start Vitis:

```

vitis_hls

```

In order to connect to server you have to:

```

ssh -i ~/.ssh/id_rsa_server guest@skylla.physics.auth.gr

```

Copy files to the server:

```

scp -r ./Server/ guest@skylla.physics.auth.gr:/home/guest/BlackScholes

```

In order to run the Simulation in Vitis:

- 10ns for 100MHz sim although u200 can achieve max 300MHz
- Select Alveo u200 from boards

The estimated simulation from Vitis regarding execution time is:
**7.3ns** so **7 times slower** than the GPU.

![App Screenshot](./assets/vitis.png)

---

### Usefull Links

- [Black Scholes Wikipedia](https://en.wikipedia.org/wiki/Black%E2%80%93Scholes_model)
- [Tim Worrall Notes for Black Scholes](http://www.timworrall.com/fin-40008/bscholes.pdf)
- [Black Scholes Cuda by Nvidia](https://github.com/tpn/cuda-samples/tree/master/v9.0/4_Finance/BlackScholes)
- [Cuda Commands](https://docs.nvidia.com/cuda/cuda-runtime-api/group__CUDART__DEVICE.html#group__CUDART__DEVICE)
- [FPGA Platform Documentation](https://docs.amd.com/r/en-US/ug1120-alveo-platforms/U200-Gen3x16-XDMA-base_2-Platform)
  ``

```

```
