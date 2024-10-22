// number of runs
#define SIZE 5000
#define RUNS 1000
#define DATA_SIZE 252

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

#include "black_scholes.hpp"


// With Arguments
int main(int argc, char ** argv) {
    // Body
    std::string CLOSE_FILE_PATH = argv[1];
    std::string DATES_FILE_PATH = argv[2];
    
    // Print input arguments
    printf("CLOSE_FILE_PATH: %s\n", CLOSE_FILE_PATH.c_str());
    printf("DATES_FILE_PATH: %s\n", DATES_FILE_PATH.c_str());

    std::ifstream closeFile (CLOSE_FILE_PATH);
    std::ifstream datesFile (DATES_FILE_PATH);

    // Read close prices
    float closePrices[DATA_SIZE];
    float dates[DATA_SIZE];

    // Iterate over the close prices
    for (int i = 0; i < DATA_SIZE; i++) {
        std::string line;
        std::getline(closeFile, line);
        closePrices[i] = std::stof(line);
    }

    // Iterate over the dates
    for (int i = 0; i < DATA_SIZE; i++) {
        std::string line;
        std::getline(datesFile, line);
        dates[i] = std::stof(line);
    }
    
    float a, b;
    calculate_prob_factors_d1_d2(60.0, 56.0, 14.0, 0.3, 0.5, &a, &b);
    printf("Extracted d1 & d2 propabilities: \na: %f \nb: %f\n", a, b);
}