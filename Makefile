#
# Copyright 2019-2020 Xilinx, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# makefile-generator v1.0.3
#

############################## Help Section ##############################
.PHONY: help

help::
	$(ECHO) "Makefile Usage:"
	$(ECHO) "  make all TARGET=<sw_emu/hw_emu/hw> DEVICE=<FPGA platform> HOST_ARCH=<aarch32/aarch64/x86> EDGE_COMMON_SW=<rootfs and kernel image path>"
	$(ECHO) "      Command to generate the design for specified Target and Shell."
	$(ECHO) "      By default, HOST_ARCH=x86. HOST_ARCH and EDGE_COMMON_SW is required for SoC shells"
	$(ECHO) ""
	$(ECHO) "  make clean "
	$(ECHO) "      Command to remove the generated non-hardware files."
	$(ECHO) ""
	$(ECHO) "  make cleanall"
	$(ECHO) "      Command to remove all the generated files."
	$(ECHO) ""
	$(ECHO)  "  make test DEVICE=<FPGA platform>"
	$(ECHO)  "     Command to run the application. This is same as 'run' target but does not have any makefile dependency."
	$(ECHO)  ""
	$(ECHO) "  make sd_card TARGET=<sw_emu/hw_emu/hw> DEVICE=<FPGA platform> HOST_ARCH=<aarch32/aarch64/x86> EDGE_COMMON_SW=<rootfs and kernel image path>"
	$(ECHO) "      Command to prepare sd_card files."
	$(ECHO) "      By default, HOST_ARCH=x86. HOST_ARCH and EDGE_COMMON_SW is required for SoC shells"
	$(ECHO) ""
	$(ECHO) "  make run TARGET=<sw_emu/hw_emu/hw> DEVICE=<FPGA platform> HOST_ARCH=<aarch32/aarch64/x86> EDGE_COMMON_SW=<rootfs and kernel image path>"
	$(ECHO) "      Command to run application in emulation."
	$(ECHO) "      By default, HOST_ARCH=x86. HOST_ARCH and EDGE_COMMON_SW is required for SoC shells"
	$(ECHO) ""
	$(ECHO) "  make build TARGET=<sw_emu/hw_emu/hw> DEVICE=<FPGA platform> HOST_ARCH=<aarch32/aarch64/x86> EDGE_COMMON_SW=<rootfs and kernel image path>"
	$(ECHO) "      Command to build xclbin application."
	$(ECHO) "      By default, HOST_ARCH=x86. HOST_ARCH and EDGE_COMMON_SW is required for SoC shells"
	$(ECHO) ""
	$(ECHO) "  make host HOST_ARCH=<aarch32/aarch64/x86> EDGE_COMMON_SW=<rootfs and kernel image path>"
	$(ECHO) "      Command to build host application."
	$(ECHO) "      By default, HOST_ARCH=x86. HOST_ARCH and EDGE_COMMON_SW is required for SoC shells"
	$(ECHO) ""

############################## Setting up Project Variables ##############################

#Specify the kernel's file name and the directory's name
KERNEL_NAME=kernelBlackScholes
FOLDER_NAME=BlackScholes

# Points to top directory of Git repository
MK_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
COMMON_REPO ?= $(shell bash -c 'export MK_PATH=$(MK_PATH); echo $${MK_PATH%$(FOLDER_NAME)/*}')
PWD = $(shell readlink -f .)
XF_PROJ_ROOT = $(shell readlink -f $(COMMON_REPO))

TARGET := hw
HOST_ARCH := x86
SYSROOT :=

include ./utils.mk

XSA :=
ifneq ($(DEVICE), )
XSA := $(call device2xsa, $(DEVICE))
endif
TEMP_DIR := ./_x.$(TARGET).$(XSA)
BUILD_DIR := ./build_dir.$(TARGET).$(XSA)

# SoC variables
RUN_APP_SCRIPT = ./run_app.sh
PACKAGE_OUT = ./package.$(TARGET)

LAUNCH_EMULATOR = $(PACKAGE_OUT)/launch_$(TARGET).sh
RESULT_STRING = TEST PASSED

VPP := v++
CMD_ARGS = $(BUILD_DIR)/$(KERNEL_NAME).xclbin
SDCARD := sd_card

include ./common_fpga/includes/opencl/opencl.mk
CXXFLAGS += $(opencl_CXXFLAGS) -Wall -O0 -g -std=c++11
LDFLAGS += $(opencl_LDFLAGS)

ifeq ($(findstring nodma, $(DEVICE)), nodma)
$(error [ERROR]: This example is not supported for $(DEVICE).)
endif

############################## Setting up Host Variables ##############################
#Include Required Host Source Files
CXXFLAGS += -I./common_fpga/includes/xcl2
# Hardcoded line to find app_fixed.h although i run the needed source command
CXXFLAGS += -I/tools/Xilinx/Vitis_HLS/2022.1/include
# TODO: Move the logic all to the main.cpp with gpu and cpu
# HOST_SRCS += ./common_fpga/includes/xcl2/xcl2.cpp ./host.cpp
# Host compiler global settings
CXXFLAGS += -fmessage-length=0
LDFLAGS += -lrt -lstdc++

ifneq ($(HOST_ARCH), x86)
	LDFLAGS += --sysroot=$(SYSROOT)
endif

############################## Setting up Kernel Variables ##############################
# Kernel compiler global settings
VPP_FLAGS += -t $(TARGET) --platform $(DEVICE) --connectivity.nk $(KERNEL_NAME):6  --connectivity.sp $(KERNEL_NAME)_6.spotprice:HBM[6] --connectivity.sp $(KERNEL_NAME)_6.time:HBM[6] --connectivity.sp $(KERNEL_NAME)_6.optionPrice:HBM[6] --connectivity.sp $(KERNEL_NAME)_5.spotprice:HBM[5] --connectivity.sp $(KERNEL_NAME)_5.time:HBM[5] --connectivity.sp $(KERNEL_NAME)_5.optionPrice:HBM[5] --connectivity.sp $(KERNEL_NAME)_4.spotprice:HBM[4] --connectivity.sp $(KERNEL_NAME)_4.time:HBM[4] --connectivity.sp $(KERNEL_NAME)_4.optionPrice:HBM[4] --connectivity.sp $(KERNEL_NAME)_3.spotprice:HBM[3] --connectivity.sp $(KERNEL_NAME)_3.time:HBM[3] --connectivity.sp $(KERNEL_NAME)_3.optionPrice:HBM[3] --connectivity.sp $(KERNEL_NAME)_2.spotprice:HBM[2] --connectivity.sp $(KERNEL_NAME)_2.time:HBM[2] --connectivity.sp $(KERNEL_NAME)_2.optionPrice:HBM[2] --connectivity.sp $(KERNEL_NAME)_1.spotprice:HBM[1] --connectivity.sp $(KERNEL_NAME)_1.time:HBM[1] --connectivity.sp $(KERNEL_NAME)_1.optionPrice:HBM[1] --report_level estimate --profile.data all:all:all --profile.stall all:all --profile.exec all:all --trace_memory HBM --kernel_frequency 300  --hls.clock 300000000:$(KERNEL_NAME) --save-temps
# VPP_FLAGS += -t $(TARGET) --platform $(DEVICE)
ifneq ($(TARGET), hw)
	VPP_FLAGS += -g
endif

EXECUTABLE = ./$(KERNEL_NAME)
EMCONFIG_DIR = $(TEMP_DIR)
EMU_DIR = $(SDCARD)/data/emulation

############################## Declaring Binary Containers ##############################
BINARY_CONTAINERS += $(BUILD_DIR)/$(KERNEL_NAME).xclbin
BINARY_CONTAINER_$(KERNEL_NAME)_OBJS += $(TEMP_DIR)/$(KERNEL_NAME).xo

############################## C++ and Cuda Setup ##############################
# Compiler and flags
GCC = g++
NVCC = nvcc
CFLAGS = -O2
CUDAFLAGS = -I./common/inc

# Targets
CPU_TARGET = black_scholes_cpu
GPU_TARGET = black_scholes_cuda
GPU_INFO_TARGET = gpu_info

# Source files
CPU_SRC = main.cpp
GPU_SRC = BlackScholes.cu BlackScholes_gold.cpp
GPU_INFO_SRC = gpu_info.cu

# Object files
CPU_OBJ = $(CPU_TARGET).o
GPU_OBJ = $(GPU_TARGET).o
GPU_INFO_OBJ = $(GPU_INFO_TARGET).o

############################## Setting Targets ##############################
CP = cp -rf

.PHONY: all clean cleanall docs emconfig
all: check-devices $(EXECUTABLE) $(BINARY_CONTAINERS) emconfig sd_card $(CPU_TARGET) $(GPU_TARGET) $(GPU_INFO_TARGET)

.PHONY: host
host: $(EXECUTABLE)

.PHONY: build
build: check-vitis $(BINARY_CONTAINERS)

.PHONY: xclbin
xclbin: build

############################## Setting Rules for Binary Containers (Building Kernels) ##############################
$(TEMP_DIR)/$(KERNEL_NAME).xo: ./black_scholes_fpga.cpp
	mkdir -p $(TEMP_DIR)
	$(VPP) $(VPP_FLAGS) -c -k $(KERNEL_NAME) --temp_dir $(TEMP_DIR)  -I'$(<D)' -o'$@' '$<'
$(BUILD_DIR)/$(KERNEL_NAME).xclbin: $(BINARY_CONTAINER_$(KERNEL_NAME)_OBJS)
	mkdir -p $(BUILD_DIR)
ifeq ($(HOST_ARCH), x86)
	$(VPP) $(VPP_FLAGS) -l $(VPP_LDFLAGS) --temp_dir $(TEMP_DIR)  -o'$(BUILD_DIR)/$(KERNEL_NAME).link.xclbin' $(+)
	$(VPP) -p $(BUILD_DIR)/$(KERNEL_NAME).link.xclbin -t $(TARGET) --platform $(DEVICE) --package.out_dir $(PACKAGE_OUT) -o $(BUILD_DIR)/$(KERNEL_NAME).xclbin
else
	$(VPP) $(VPP_FLAGS) -l $(VPP_LDFLAGS) --temp_dir $(TEMP_DIR) -o'$(BUILD_DIR)/$(KERNEL_NAME).xclbin' $(+)
endif

############################## Setting Rules for Host (Building Host Executable) ##############################
$(CPU_TARGET): $(CPU_SRC)
	$(GCC) $(CFLAGS) $(CPU_SRC) -o $(CPU_OBJ)

$(GPU_TARGET): $(GPU_SRC)
	$(NVCC) $(CUDAFLAGS) $(GPU_SRC) -o $(GPU_OBJ)

$(GPU_INFO_TARGET): $(GPU_INFO_SRC)
	$(NVCC) $(GPU_INFO_SRC) -o $(GPU_INFO_OBJ)

$(EXECUTABLE): $(HOST_SRCS) | check-xrt
		$(CXX) -o $@ $^ $(CXXFLAGS) $(LDFLAGS) $(APPFLAGS)

emconfig:$(EMCONFIG_DIR)/emconfig.json
$(EMCONFIG_DIR)/emconfig.json:
	emconfigutil --platform $(DEVICE) --od $(EMCONFIG_DIR)

############################## Setting Essential Checks and Running Rules ##############################
run: all
ifeq ($(TARGET),$(filter $(TARGET),sw_emu hw_emu))
ifeq ($(HOST_ARCH), x86)
	$(CP) $(EMCONFIG_DIR)/emconfig.json .
	XCL_EMULATION_MODE=$(TARGET) $(EXECUTABLE) $(CMD_ARGS)
else
	$(LAUNCH_EMULATOR_CMD)
endif
else
ifeq ($(HOST_ARCH), x86)
	$(EXECUTABLE) $(CMD_ARGS)
endif
endif


.PHONY: test
test: $(EXECUTABLE)
ifeq ($(TARGET),$(filter $(TARGET),sw_emu hw_emu))
ifeq ($(HOST_ARCH), x86)
	XCL_EMULATION_MODE=$(TARGET) $(EXECUTABLE) $(CMD_ARGS)
else
	$(LAUNCH_EMULATOR_CMD)
endif
else
ifeq ($(HOST_ARCH), x86)
	$(EXECUTABLE) $(CMD_ARGS)
else
	$(ECHO) "Please copy the content of sd_card folder and data to an SD Card and run on the board"
endif
endif

############################## Preparing sdcard ##############################
sd_card: $(BINARY_CONTAINERS) $(EXECUTABLE) gen_run_app
ifneq ($(HOST_ARCH), x86)
	$(VPP) -p $(BUILD_DIR)/$(KERNEL_NAME).xclbin -t $(TARGET) --platform $(DEVICE) --package.out_dir $(PACKAGE_OUT) --package.rootfs $(EDGE_COMMON_SW)/rootfs.ext4 --package.sd_file $(SD_IMAGE_FILE) --package.sd_file xrt.ini --package.sd_file $(RUN_APP_SCRIPT) --package.sd_file $(EXECUTABLE) -o $(KERNEL_NAME).xclbin
endif

############################## BUild Rules ##############################
# Run rules
run_cpu: $(CPU_TARGET)
	./$(CPU_OBJ)

run_gpu: $(GPU_TARGET)
	./$(GPU_OBJ)

run_gpu_info: $(GPU_INFO_TARGET)
	./$(GPU_INFO_OBJ)	

############################## Cleaning Rules ##############################
# Cleaning stuff
clean:
	-$(RMDIR) $(EXECUTABLE) $(XCLBIN)/{*sw_emu*,*hw_emu*}
	-$(RMDIR) profile_* TempConfig system_estimate.xtxt *.rpt *.csv
	-$(RMDIR) src/*.ll *v++* .Xil emconfig.json dltmp* xmltmp* *.log *.jou *.wcfg *.wdb
	rm -f $(CPU_OBJ) $(GPU_OBJ) $(GPU_INFO_OBJ)

cleanall: clean
	-$(RMDIR) build_dir* sd_card*
	-$(RMDIR) package.*
	-$(RMDIR) _x* *xclbin.run_summary qemu-memory-_* emulation _vimage pl* start_simulation.sh *.xclbin
	rm -f $(CPU_OBJ) $(GPU_OBJ) $(GPU_INFO_OBJ)
