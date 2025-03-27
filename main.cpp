// number of runs
#define DATA_SIZE 858
#define PULL_OPTION 0
#define CALL_OPTION 1
#define RUNS 1000

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
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <chrono>

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

    // Calculate the predicted option prices
    float optionPrices[DATA_SIZE];
    chrono::high_resolution_clock::time_point t1, t2;
    t1 = chrono::high_resolution_clock::now();
    for (int r = 0; r < RUNS; r++) {
        for (int i = 0; i < DATA_SIZE; i++) {
            float a, b;
            Black_Scholes_CPU(callTypes[i] ,closePrices[i], strikePrices[i], RISK_FREE_RATE, VOLATILITY, tte[i], &optionPrices[i]);
        }
    }
 
    t2 = chrono::high_resolution_clock::now();
    chrono::duration <double, std::milli> CPU_time = t2 - t1;
    printf("CPU Time: %f ms\n", CPU_time.count()*1000);
    // PRINT AVG TIME
    printf("Average CPU Time: %f ms\n", (CPU_time.count()/DATA_SIZE)*1000);

    // // Iterate through results and print
    // for (int i = 0; i < DATA_SIZE; i++) {
    //     printf("Option %d: %.5f\n", i+1, optionPrices[i]);
    // }

}