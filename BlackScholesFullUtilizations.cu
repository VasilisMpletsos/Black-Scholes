#define DATA_SIZE 858

#include <helper_functions.h>
#include <helper_cuda.h>  
#include "BlackScholes_kernel.cuh"

const int REPEAT_ITERATIONS_EXPERIMENT = 1000;


const int MEMORY_SIZE_ALLOCATION_FLOAT = DATA_SIZE * sizeof(float);
const int MEMORY_SIZE_ALLOCATION_INT = DATA_SIZE * sizeof(float);
const float RISKFREE = 0.01575f;
const float VOLATILITY = 0.25f;

#define DIV_UP(a, b) ( ((a) + (b) - 1) / (b) )

////////////////////////////////////////////////////////////////////////////////
// ------------------------------ Main program ------------------------------ //
////////////////////////////////////////////////////////////////////////////////
int main(int argc, char **argv)
{
    printf("Starting BlackScholes on GPU...\n");

    float *h_OptionResultGPULarge, *h_StockPrice, *h_OptionStrike, *h_OptionYears;
    float *d_OptionResult, *d_StockPrice, *d_OptionStrike, *d_OptionYears;
    int *h_OptionTypes, *d_OptionTypes;

    double gpuTime;

    StopWatchInterface *hTimer = NULL;
    int i;

    // Detect NVIDIA GPU
    int device = findCudaDevice(argc, (const char **)argv);

    sdkCreateTimer(&hTimer);

    // Get gpu properties
    cudaDeviceProp deviceProp;
    checkCudaErrors(cudaGetDeviceProperties(&deviceProp, device));
    printf("GPU max threads per block %d \n", deviceProp.maxThreadsPerBlock);

    int MAX_THREADS_PER_BLOCK = deviceProp.maxThreadsPerBlock;
    int numSMs = deviceProp.multiProcessorCount;
    int maxThreadsPerSM = deviceProp.maxThreadsPerMultiProcessor;
    int blockSize = deviceProp.maxThreadsPerBlock;
    int blocksPerSM = DIV_UP(maxThreadsPerSM, blockSize);
    printf("GPU SM count: %d\n", numSMs);
    printf("Max threads per SM: %d\n", maxThreadsPerSM);
    printf("Using block size: %d\n", blockSize);
    printf("Blocks per SM for occupancy: %d\n", blocksPerSM);

    // Set oversubscription factor to make kernel run longer for accurate timing
    const int overSub = 32;
    int minNumBlocks = numSMs * blocksPerSM * overSub;
    size_t minTotalOptions = (size_t)minNumBlocks * blockSize;
    size_t numReplicas = DIV_UP(minTotalOptions, DATA_SIZE);
    size_t totalOptions = numReplicas * DATA_SIZE;
    int numBlocks = DIV_UP(totalOptions, blockSize);
    printf("Using %zu replicas, total options: %zu\n", numReplicas, totalOptions);
    printf("Launching %d blocks of %d threads\n", numBlocks, blockSize);

    printf("Allocating CPU memory for options.\n");
    h_StockPrice = (float *)malloc(MEMORY_SIZE_ALLOCATION_FLOAT);
    h_OptionStrike = (float *)malloc(MEMORY_SIZE_ALLOCATION_FLOAT);
    h_OptionYears = (float *)malloc(MEMORY_SIZE_ALLOCATION_FLOAT);
    h_OptionTypes = (int *)malloc(MEMORY_SIZE_ALLOCATION_INT);

    printf("Reading data...\n");
    // Reading data from files
    std::cout << "Reading data...\n";
    std::ifstream closeFile("./datasets/option_price.txt");
    std::ifstream strikeFile("./datasets/strike.txt");
    std::ifstream tteFile("./datasets/tte.txt");
    std::ifstream typeFile("./datasets/type.txt");

    // Check if files opened successfully
    if (!closeFile || !strikeFile || !tteFile || !typeFile) {
        throw std::runtime_error("Failed to open one or more input files.");
    }

    // Load data into host arrays
    for (int i = 0; i < DATA_SIZE; i++) {
        std::string line;

        std::getline(closeFile, line);
        h_StockPrice[i] = std::stof(line);

        std::getline(strikeFile, line);
        h_OptionStrike[i] = std::stof(line);

        std::getline(tteFile, line);
        h_OptionYears[i] = std::stof(line);

        std::getline(typeFile, line);
        h_OptionTypes[i] = std::stoi(line);  // Assuming type is an integer
    }

    // Close files
    closeFile.close();
    strikeFile.close();
    tteFile.close();
    typeFile.close();

    // Allocate large host arrays for replication
    size_t memSizeFloat = totalOptions * sizeof(float);
    size_t memSizeInt = totalOptions * sizeof(int);
    float *h_StockPriceLarge = (float *)malloc(memSizeFloat);
    float *h_OptionStrikeLarge = (float *)malloc(memSizeFloat);
    float *h_OptionYearsLarge = (float *)malloc(memSizeFloat);
    int *h_OptionTypesLarge = (int *)malloc(memSizeInt);
    h_OptionResultGPULarge = (float *)malloc(memSizeFloat);

    // Replicate the data
    for (size_t r = 0; r < numReplicas; r++) {
        memcpy(h_StockPriceLarge + r * DATA_SIZE, h_StockPrice, MEMORY_SIZE_ALLOCATION_FLOAT);
        memcpy(h_OptionStrikeLarge + r * DATA_SIZE, h_OptionStrike, MEMORY_SIZE_ALLOCATION_FLOAT);
        memcpy(h_OptionYearsLarge + r * DATA_SIZE, h_OptionYears, MEMORY_SIZE_ALLOCATION_FLOAT);
        memcpy(h_OptionTypesLarge + r * DATA_SIZE, h_OptionTypes, MEMORY_SIZE_ALLOCATION_INT);
    }

    // Free original small host input arrays to save memory
    free(h_StockPrice);
    free(h_OptionStrike);
    free(h_OptionYears);
    free(h_OptionTypes);

    printf("Allocating GPU memory for options.\n");
    checkCudaErrors(cudaMalloc((void **)&d_OptionResult, memSizeFloat));
    checkCudaErrors(cudaMalloc((void **)&d_StockPrice, memSizeFloat));
    checkCudaErrors(cudaMalloc((void **)&d_OptionStrike, memSizeFloat));
    checkCudaErrors(cudaMalloc((void **)&d_OptionYears, memSizeFloat));
    checkCudaErrors(cudaMalloc((void **)&d_OptionTypes, memSizeInt));

    printf("Copying input data from host CPU to GPU registers.\n");
    checkCudaErrors(cudaMemcpy(d_StockPrice,  h_StockPriceLarge,  memSizeFloat, cudaMemcpyHostToDevice));
    checkCudaErrors(cudaMemcpy(d_OptionStrike, h_OptionStrikeLarge, memSizeFloat, cudaMemcpyHostToDevice));
    checkCudaErrors(cudaMemcpy(d_OptionYears,  h_OptionYearsLarge,  memSizeFloat, cudaMemcpyHostToDevice));
    checkCudaErrors(cudaMemcpy(d_OptionTypes,  h_OptionTypesLarge,  memSizeInt, cudaMemcpyHostToDevice));
    printf("Data copies successfully.\n\n");

    // Initialize result
    for (i = 0; i < totalOptions; i++)
    {
        h_OptionResultGPULarge[i] = 0.0f;
    }

    printf("Executing Black-Scholes GPU kernel %i iterations...\n", REPEAT_ITERATIONS_EXPERIMENT);
    checkCudaErrors(cudaDeviceSynchronize());
    sdkResetTimer(&hTimer);
    sdkStartTimer(&hTimer);

    for (i = 0; i < REPEAT_ITERATIONS_EXPERIMENT; i++)
    {
        BlackScholesGPU<<<numBlocks, blockSize>>>(
            (int1 *)d_OptionTypes,
            (float1 *)d_StockPrice,
            (float1 *)d_OptionStrike,
            RISKFREE,
            VOLATILITY,
            (float1 *)d_OptionYears,
            (float1 *)d_OptionResult
        );
        getLastCudaError("BlackScholesGPU() execution failed\n");
    }

    checkCudaErrors(cudaDeviceSynchronize());
    sdkStopTimer(&hTimer);
    gpuTime = sdkGetTimerValue(&hTimer) / REPEAT_ITERATIONS_EXPERIMENT;

    //Both call and put is calculated
    printf("Black Scholes GPU() average execution time: %f msec\n", gpuTime * 1000);
    printf("Effective memory bandwidth: %f GB/s\n", ((double)(5 * totalOptions * sizeof(float)) * 1E-9) / (gpuTime * 1E-3));
    printf("Gigaoptions per second: %f \n\n", ((double)(totalOptions) * 1E-9) / (gpuTime * 1E-3));

    printf("\nReading back GPU results...\n");
    //Read back GPU results to compare them to CPU results
    checkCudaErrors(cudaMemcpy(h_OptionResultGPULarge, d_OptionResult, memSizeFloat, cudaMemcpyDeviceToHost));

    // Iterate through first set of results and print
    // for (int i = 0; i < DATA_SIZE; i++) {
    //     printf("Option %d: %.5f\n", i+1, h_OptionResultGPULarge[i]);
    // }

    printf("Cleaning GPU allocated memory.\n");
    checkCudaErrors(cudaFree(d_OptionYears));
    checkCudaErrors(cudaFree(d_OptionStrike));
    checkCudaErrors(cudaFree(d_StockPrice));
    checkCudaErrors(cudaFree(d_OptionResult));
    checkCudaErrors(cudaFree(d_OptionTypes));

    printf("Cleaning CPU allocated memory\n");
    free(h_OptionYearsLarge);
    free(h_OptionStrikeLarge);
    free(h_StockPriceLarge);
    free(h_OptionTypesLarge);
    free(h_OptionResultGPULarge);
    sdkDeleteTimer(&hTimer);
    printf("Test Done\n");
    exit(EXIT_SUCCESS);
}