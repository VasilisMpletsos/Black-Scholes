// number of runs
#define SIZE 5000
#define RUNS 1000

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
int main (int argc, char *argv[]) {
    // Body
    printf("Hello World Pointer\n");
    float a, b;
    calculate_prob_factors_d1_d2(1.0, 1.0, 1.0, 1.0, 1.0, &a, &b);
    printf("a: %f, b: %f\n", a, b);
}