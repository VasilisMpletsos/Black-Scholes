// number of runs
#define DATA_SIZE 858
#define RUNS 10000

// Define Option Constants for test
// 1.575% risk free rate, logical values from 1% to 3% but depends on the country
#define RISK_FREE_RATE  0.01575
// logical values from 10% to 30% but depends on the stock
#define VOLATILITY 0.25

// Import various libraries
#include "string.h"
#include <cmath>
#include "stdio.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <chrono>

// Use Windows threads for MinGW
#ifdef _WIN32
    #include <windows.h>
    #include <process.h>
#else
    #include <thread>
    #include <unistd.h>
#endif

// Include black scholes implementation
#include "black_scholes.hpp"

///////////////////////////////////////////////////////////////////////////////
// Notes:
// This is the simplest file that excecutes the black scholes algorithm on the CPU
///////////////////////////////////////////////////////////////////////////////

// With Arguments
int main(int argc, char **argv) {
    
    // Read close prices
    float closePrices[DATA_SIZE], strikePrices[DATA_SIZE], tte[DATA_SIZE];
    int callTypes[DATA_SIZE];

    printf("Reading data...\n");
    // Reading data from files
    std::cout << "Reading data...\n";
    std::ifstream closeFile("./datasets/option_price.txt");
    std::ifstream strikeFile("./datasets/strike.txt");
    std::ifstream tteFile("./datasets/tte.txt");
    std::ifstream typeFile("./datasets/type.txt");
    std::ifstream binaryFile("./datasets/type.txt");

    // Check if files opened successfully
    if (!closeFile || !strikeFile || !tteFile || !typeFile) {
        throw std::runtime_error("Failed to open one or more input files.");
    }

    // Load data into host arrays
    for (int i = 0; i < DATA_SIZE; i++) {
        std::string line;

        std::getline(closeFile, line);
        closePrices[i] = std::stof(line);

        std::getline(strikeFile, line);
        strikePrices[i] = std::stof(line);

        std::getline(tteFile, line);
        tte[i] = std::stof(line);

        std::getline(typeFile, line);
        callTypes[i] = std::stoi(line);  // Assuming type is an integer
    }

    // =================== CPU Execution ===================

    // volatile float dummy_sum = 0.0f;

    // Calculate the predicted option prices
    float optionPrices[DATA_SIZE];
    std::chrono::high_resolution_clock::time_point t1, t2;
    t1 = std::chrono::high_resolution_clock::now();
    for (int r = 0; r < RUNS; r++) {
        for (int i = 0; i < DATA_SIZE; i++) {
            float a, b;
            Black_Scholes_CPU(callTypes[i] ,closePrices[i], strikePrices[i], RISK_FREE_RATE, VOLATILITY, tte[i], &optionPrices[i]);
            // dummy_sum += optionPrices[i];
        }
    }
    t2 = std::chrono::high_resolution_clock::now();
    std::chrono::duration <double, std::milli> CPU_time = t2 - t1;
    printf("Total options calculations done: %d\n", DATA_SIZE*RUNS);
    printf("Total CPU Time: %f milli seconds\n", CPU_time.count());
    printf("In seconds: %f \n", CPU_time.count()/1000);
    // PRINT AVG TIME
    printf("Average CPU Time per option: %f milli seconds\n", (CPU_time.count()/(DATA_SIZE*RUNS)));

    // Additionally run a 16-thread CPU benchmark (separate buffer)
    #ifdef _WIN32
    // Windows threading implementation
    {
        struct ThreadData {
            int start;
            int end;
            float* optionPrices_mt;
            int* callTypes;
            float* closePrices;
            float* strikePrices;
            float* tte;
        };

        float optionPrices_mt[DATA_SIZE];
        const int NUM_THREADS = 16;
        HANDLE threads[NUM_THREADS];
        ThreadData threadData[NUM_THREADS];
        int base_chunk = DATA_SIZE / NUM_THREADS;

        // Thread function with proper calling convention
        struct ThreadFunctor {
            static unsigned __stdcall threadFunc(void* param) {
                ThreadData* data = (ThreadData*)param;
                for (int run = 0; run < RUNS; ++run) {
                    for (int i = data->start; i < data->end; ++i) {
                        Black_Scholes_CPU(data->callTypes[i], data->closePrices[i], 
                                        data->strikePrices[i], RISK_FREE_RATE, 
                                        VOLATILITY, data->tte[i], &data->optionPrices_mt[i]);
                    }
                }
                return 0;
            }
        };

        t1 = std::chrono::high_resolution_clock::now();

        for (int t = 0; t < NUM_THREADS; ++t) {
            threadData[t].start = t * base_chunk;
            threadData[t].end = (t == NUM_THREADS - 1) ? DATA_SIZE : threadData[t].start + base_chunk;
            threadData[t].optionPrices_mt = optionPrices_mt;
            threadData[t].callTypes = callTypes;
            threadData[t].closePrices = closePrices;
            threadData[t].strikePrices = strikePrices;
            threadData[t].tte = tte;
            threads[t] = (HANDLE)_beginthreadex(NULL, 0, ThreadFunctor::threadFunc, &threadData[t], 0, NULL);
        }

        WaitForMultipleObjects(NUM_THREADS, threads, TRUE, INFINITE);
        for (int t = 0; t < NUM_THREADS; ++t) {
            CloseHandle(threads[t]);
        }

        t2 = std::chrono::high_resolution_clock::now();
        std::chrono::duration <double, std::milli> CPU_time_mt = t2 - t1;
        printf("Total CPU Time (16-thread): %f milli seconds\n", CPU_time_mt.count());
        printf("Average CPU Time per option (16-thread): %f milli seconds\n", (CPU_time_mt.count()/(DATA_SIZE*RUNS)));
    }
    #else
    // POSIX threading implementation
    {
        float optionPrices_mt[DATA_SIZE];
        const int NUM_THREADS = 16;
        int base_chunk = DATA_SIZE / NUM_THREADS;

        t1 = std::chrono::high_resolution_clock::now();
        std::vector<std::thread> threads;
        threads.reserve(NUM_THREADS);

        for (int t = 0; t < NUM_THREADS; ++t) {
            int start = t * base_chunk;
            int end = (t == NUM_THREADS - 1) ? DATA_SIZE : start + base_chunk;
            threads.emplace_back([start, end, &optionPrices_mt, &callTypes, &closePrices, &strikePrices, &tte]() {
                for (int run = 0; run < RUNS; ++run) {
                    for (int i = start; i < end; ++i) {
                        Black_Scholes_CPU(callTypes[i], closePrices[i], strikePrices[i], RISK_FREE_RATE, VOLATILITY, tte[i], &optionPrices_mt[i]);
                    }
                }
            });
        }

        for (auto &th : threads) th.join();

        t2 = std::chrono::high_resolution_clock::now();
        std::chrono::duration <double, std::milli> CPU_time_mt = t2 - t1;
        printf("Total CPU Time (16-thread): %f milli seconds\n", CPU_time_mt.count());
        printf("Average CPU Time per option (16-thread): %f milli seconds\n", (CPU_time_mt.count()/(DATA_SIZE*RUNS)));
    }
    #endif

    // Print the dummy sum to ensure it's not optimized away
    // printf("Checksum (ignore): %f\n", dummy_sum);

    // // Iterate through results and print
    // for (int i = 0; i < DATA_SIZE; i++) {
    //     printf("Option %d: %.5f\n", i+1, optionPrices[i]);
    // }

}