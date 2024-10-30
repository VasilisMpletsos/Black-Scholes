// number of runs
#define DATA_SIZE 858
#define PULL_OPTION 0
#define CALL_OPTION 1


// Define Option Constants for test
// 1.575% risk free rate, logical values from 1% to 3% but depends on the country
#define RISK_FREE_RATE  0.01575
// logical values from 10% to 30% but depends on the stock
#define VOLATILITY 0.25
#define STRIKE_PRICE 2400

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


// With Arguments
int main(int argc, char ** argv) {
    // Body
    std::string CLOSE_FILE_PATH = argv[1];
    std::string STRIKE_PRICE_FILE_PATH = argv[2];
    std::string TTE_FILE_PATH = argv[3];
    std::string OPTION_TYPE_FILE_PATH = argv[4];

    // Open files streams
    std::ifstream closeFile (CLOSE_FILE_PATH);
    std::ifstream tteFile (TTE_FILE_PATH);
    std::ifstream strikeFile (STRIKE_PRICE_FILE_PATH);
    std::ifstream typeFile (OPTION_TYPE_FILE_PATH);

    // Read close prices
    float closePrices[DATA_SIZE], strikePrices[DATA_SIZE], tte[DATA_SIZE];
    int callTypes[DATA_SIZE];

    // Iterate over the close prices and store
    for (int i = 0; i < DATA_SIZE; i++) {
        std::string line;
        std::getline(closeFile, line);
        closePrices[i] = std::stof(line);
    }

    // Iterate over the dates and store
    for (int i = 0; i < DATA_SIZE; i++) {
        std::string line;
        std::getline(tteFile, line);
        tte[i] = std::stof(line);
    }

    // Iterate over the strike prices and store
    for (int i = 0; i < DATA_SIZE; i++) {
        std::string line;
        std::getline(strikeFile, line);
        strikePrices[i] = std::stof(line);
    }

    // Iterate over the option types and store
    for (int i = 0; i < DATA_SIZE; i++) {
        std::string line;
        std::getline(typeFile, line);
        callTypes[i] = std::stof(line);
    }

    // =================== CPU Execution ===================

    // Calculate the predicted option prices
    float optionPrices[DATA_SIZE];
    chrono::high_resolution_clock::time_point t1, t2;
    t1 = chrono::high_resolution_clock::now();
    for (int i = 0; i < DATA_SIZE; i++) {
        float a, b;
        Black_Scholes_CPU(callTypes[i] ,closePrices[i], strikePrices[i], RISK_FREE_RATE, VOLATILITY, tte[i], &optionPrices[i]);
    }
    t2 = chrono::high_resolution_clock::now();
    chrono::duration <double, std::milli> CPU_time = t2 - t1;
    printf("CPU Time: %f ms\n", CPU_time.count());

    // // Print the 10 first option prices
    // for (int i = 0; i < DATA_SIZE; i++) {
    //     printf("Option Price: %f\n Call Option Type %d\n\n", optionPrices[i], callTypes[i]);
    // }
}