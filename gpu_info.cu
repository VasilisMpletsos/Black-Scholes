#include <stdio.h>
#define KB 1024
#define MB 1024 * KB
#define GB 1024 * MB

int main() {
    // Read cuda device properties
    int device;
    cudaDeviceProp props;
    cudaGetDevice(&device);
    cudaGetDeviceProperties(&props, device);

    // Extract device properties
    size_t totalGlobalMem = props.totalGlobalMem;
    size_t sharedMemPerBlock = props.sharedMemPerBlock;
    int threads_per_block = props.maxThreadsPerBlock;
    int threads_per_multiprocessor = props.maxThreadsPerMultiProcessor;
    int registers_per_block = props.regsPerBlock;
    int maxThreadsDim[3] = {props.maxThreadsDim[0], props.maxThreadsDim[1], props.maxThreadsDim[2]};
    int maxGridSize[3] = {props.maxGridSize[0], props.maxGridSize[1], props.maxGridSize[2]};
    int clockRate = props.clockRate;

    // Print device properties
    printf("Maximum threads per block: %d\n", threads_per_block);
    printf("Maximum threads per multiprocessor: %d\n", threads_per_multiprocessor);
    printf("Registers per block: %d\n", registers_per_block);
    printf("Shared memory per block: %zu MB\n", sharedMemPerBlock/MB);
    printf("Total global memory: %zu GB\n", totalGlobalMem/(1024*1024*1024));
    printf("Maximum dimension of each block:");
    printf(" x: %d, y: %d, z: %d\n", maxThreadsDim[0], maxThreadsDim[1], maxThreadsDim[2]);
    printf("Maximum dimension of each grid:");
    printf(" x: %d, y: %d, z: %d\n", maxGridSize[0], maxGridSize[1], maxGridSize[2]);
    printf("Clock rate: %d MHz\n", clockRate/1000);

    return 0;
}