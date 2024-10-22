// number of runs
#define DATA_SIZE 252
#define PULL_OPTION 0
#define CALL_OPTION 1


// Define Option Constants for test
// 1.575% risk free rate, logical values from 1% to 3% but depends on the country
#define RISK_FREE_RATE  0.01575
// ~25% volatility, logical values from 10% to 30% but depends on the stock
#define VOLATILITY 0.15
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
    std::string DATES_FILE_PATH = argv[2];
    
    // Print input arguments
    printf("CLOSE_FILE_PATH: %s\n", CLOSE_FILE_PATH.c_str());
    printf("DATES_FILE_PATH: %s\n\n", DATES_FILE_PATH.c_str());

    // Open files streams
    std::ifstream closeFile (CLOSE_FILE_PATH);
    std::ifstream datesFile (DATES_FILE_PATH);

    // Read close prices
    float closePrices[DATA_SIZE];
    float time[DATA_SIZE];

    // Iterate over the close prices and store
    for (int i = 0; i < DATA_SIZE; i++) {
        std::string line;
        std::getline(closeFile, line);
        closePrices[i] = std::stof(line);
    }

    // Iterate over the dates and store
    for (int i = 0; i < DATA_SIZE; i++) {
        std::string line;
        std::getline(datesFile, line);
        time[i] = std::stof(line);
    }

    // Calculate the predicted option prices
    float optionPrices[DATA_SIZE];
    chrono::high_resolution_clock::time_point t1, t2;
    t1 = chrono::high_resolution_clock::now();
    for (int i = 0; i < DATA_SIZE; i++) {
        float a, b;
        Black_Scholes_CPU(CALL_OPTION ,closePrices[i], STRIKE_PRICE, RISK_FREE_RATE, VOLATILITY, time[i], &optionPrices[i]);
    }
    t2 = chrono::high_resolution_clock::now();
    chrono::duration <double, std::milli> CPU_time = t2 - t1;
    printf("CPU Time: %f ms\n", CPU_time.count());

    // Print the 10 first option prices
    for (int i = 0; i < 10; i++) {
        printf("Option Price: %f\n", optionPrices[i]);
    }
}