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

    float *h_OptionResultGPU, *h_StockPrice, *h_OptionStrike, *h_OptionYears;
    float *d_OptionResult, *d_StockPrice, *d_OptionStrike, *d_OptionYears;
    int *h_OptionTypes, *d_OptionTypes;

    double gpuTime;

    StopWatchInterface *hTimer = NULL;
    int i;

    // Detect NVIDIA GPU
    findCudaDevice(argc, (const char **)argv);

    sdkCreateTimer(&hTimer);

    printf("Allocating CPU memory for options.\n");
    h_OptionResultGPU  = (float *)malloc(MEMORY_SIZE_ALLOCATION_FLOAT);
    h_StockPrice = (float *)malloc(MEMORY_SIZE_ALLOCATION_FLOAT);
    h_OptionStrike = (float *)malloc(MEMORY_SIZE_ALLOCATION_FLOAT);
    h_OptionYears = (float *)malloc(MEMORY_SIZE_ALLOCATION_FLOAT);
    h_OptionTypes = (int *)malloc(MEMORY_SIZE_ALLOCATION_INT);

    printf("Allocating GPU memory for options.\n");
    checkCudaErrors(cudaMalloc((void **)&d_OptionResult, MEMORY_SIZE_ALLOCATION_FLOAT));
    checkCudaErrors(cudaMalloc((void **)&d_StockPrice, MEMORY_SIZE_ALLOCATION_FLOAT));
    checkCudaErrors(cudaMalloc((void **)&d_OptionStrike, MEMORY_SIZE_ALLOCATION_FLOAT));
    checkCudaErrors(cudaMalloc((void **)&d_OptionYears, MEMORY_SIZE_ALLOCATION_FLOAT));
    checkCudaErrors(cudaMalloc((void **)&d_OptionTypes, MEMORY_SIZE_ALLOCATION_INT));

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

    //Generate random options set
    for (i = 0; i < DATA_SIZE; i++)
    {
        h_OptionResultGPU[i] = 0.0f;
    }

    printf("Copying input data from host CPU to GPU registers.\n");
    checkCudaErrors(cudaMemcpy(d_StockPrice,  h_StockPrice,   MEMORY_SIZE_ALLOCATION_FLOAT, cudaMemcpyHostToDevice));
    checkCudaErrors(cudaMemcpy(d_OptionStrike, h_OptionStrike,  MEMORY_SIZE_ALLOCATION_FLOAT, cudaMemcpyHostToDevice));
    checkCudaErrors(cudaMemcpy(d_OptionYears,  h_OptionYears,   MEMORY_SIZE_ALLOCATION_FLOAT, cudaMemcpyHostToDevice));
    checkCudaErrors(cudaMemcpy(d_OptionTypes,  h_OptionTypes,   MEMORY_SIZE_ALLOCATION_INT, cudaMemcpyHostToDevice));
    printf("Data copies successfully.\n\n");


    printf("Executing Black-Scholes GPU kernel %i iterations...\n", REPEAT_ITERATIONS_EXPERIMENT);
    checkCudaErrors(cudaDeviceSynchronize());
    sdkResetTimer(&hTimer);
    sdkStartTimer(&hTimer);

    for (i = 0; i < REPEAT_ITERATIONS_EXPERIMENT; i++)
    {
        // because we have 858 options, we need to launch 13 blocks of 66 threads
        BlackScholesGPU<<<13, 66>>>(
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

    sdkStopTimer(&hTimer);
    checkCudaErrors(cudaDeviceSynchronize());
    gpuTime = sdkGetTimerValue(&hTimer) / REPEAT_ITERATIONS_EXPERIMENT;

    //Both call and put is calculated
    printf("Black Scholes GPU() average execution time: %f msec\n", gpuTime);
    printf("Effective memory bandwidth: %f GB/s\n", ((double)(5 * DATA_SIZE * sizeof(float)) * 1E-9) / (gpuTime * 1E-3));
    printf("Gigaoptions per second: %f \n\n", ((double)(DATA_SIZE) * 1E-9) / (gpuTime * 1E-3));

    printf("\nReading back GPU results...\n");
    //Read back GPU results to compare them to CPU results
    checkCudaErrors(cudaMemcpy(h_OptionResultGPU, d_OptionResult, MEMORY_SIZE_ALLOCATION_FLOAT, cudaMemcpyDeviceToHost));

    // // Iterate through results and print
    // for (int i = 0; i < DATA_SIZE; i++) {
    //     printf("Option %d: %.5f\n", i+1, h_OptionResultGPU[i]);
    // }

    printf("Cleaning GPU allocated memory.\n");
    checkCudaErrors(cudaFree(d_OptionYears));
    checkCudaErrors(cudaFree(d_OptionStrike));
    checkCudaErrors(cudaFree(d_StockPrice));
    checkCudaErrors(cudaFree(d_OptionResult));
    checkCudaErrors(cudaFree(d_OptionTypes));

    printf("Cleaning CPU allocated memory\n");
    free(h_OptionYears);
    free(h_OptionStrike);
    free(h_StockPrice);
    free(h_OptionTypes);
    free(h_OptionResultGPU);
    sdkDeleteTimer(&hTimer);
    printf("Test Done\n");
    exit(EXIT_SUCCESS);
}
