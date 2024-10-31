#include <stdio.h>

int main() {
    
    // Get the limit of the current cuda support
    int device;
    cudaDeviceProp props;
    cudaGetDevice(&device);
    cudaGetDeviceProperties(&props, device);
    int threads_per_block = props.maxThreadsPerBlock;
    printf("Maximum threads per block: %d\n", threads_per_block);

    return 0;
}