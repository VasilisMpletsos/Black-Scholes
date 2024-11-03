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

# Rules
all: $(CPU_TARGET) $(GPU_TARGET) $(GPU_INFO_TARGET)

$(CPU_TARGET): $(CPU_SRC)
	$(GCC) $(CFLAGS) $(CPU_SRC) -o $(CPU_OBJ)

$(GPU_TARGET): $(GPU_SRC)
	$(NVCC) $(CUDAFLAGS) $(GPU_SRC) -o $(GPU_OBJ)

$(GPU_INFO_TARGET): $(GPU_INFO_SRC)
	$(NVCC) $(GPU_INFO_SRC) -o $(GPU_INFO_OBJ)

# Clean rule
clean:
	rm -f $(CPU_OBJ) $(GPU_OBJ) $(GPU_INFO_OBJ)

# Run rules
run_cpu: $(CPU_TARGET)
	./$(CPU_OBJ)

run_gpu: $(GPU_TARGET)
	./$(GPU_OBJ)

run_gpu_info: $(GPU_INFO_TARGET)
	./$(GPU_INFO_OBJ)