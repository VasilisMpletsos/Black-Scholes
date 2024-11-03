#include "string.h"
#include "ap_fixed.h"
#include <cmath>
#include "stdio.h"
#include <hls_stream.h>
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
typedef ap_fixed <23,13,AP_RND_CONV> FPGA_FIXED_POINT;
typedef ap_uint<1> OPTION_TYPE_BOOL;

#define N 12
#define SQRT_MAGIC_F 0x5f3759df

inline void calculate_prob_factors_d1_d2(FPGA_FIXED_POINT spotprice, FPGA_FIXED_POINT strike, FPGA_FIXED_POINT tte, FPGA_FIXED_POINT *d1, FPGA_FIXED_POINT *d2);

inline void calculate_prob_factors_d1_d2(
    FPGA_FIXED_POINT spotprice,
    FPGA_FIXED_POINT strike,
    FPGA_FIXED_POINT tte,
    FPGA_FIXED_POINT *d1,
    FPGA_FIXED_POINT *d2)
{
  DTYPE logInput = (DTYPE) (spotprice/((DTYPE)strike));
  DTYPE logVal = (DTYPE) log((float)logInput);
  DTYPE sqrtVal = (DTYPE)sqrt((float)tte);
  DTYPE powerVal = (DTYPE)VOLATILITY * (DTYPE)VOLATILITY;
  DTYPE powerVal2 = powerVal * (DTYPE)0.5;
  DTYPE nD1 = logVal + (DTYPE)((DTYPE)RISK_FREE_RATE + powerVal2) * tte;
  DTYPE nD2 = logVal + (DTYPE)((DTYPE)RISK_FREE_RATE - powerVal2) * tte;
  DTYPE denum = (DTYPE)((DTYPE)VOLATILITY * sqrtVal);
  *d1 = (DTYPE)(nD1 / denum);
  *d2 = (DTYPE)(nD2 / denum);
}

// Returns approximate value of e^x using sum of first n terms of Taylor Series
inline FPGA_FIXED_POINT exp_taylor_aprox(FPGA_FIXED_POINT x) {
    FPGA_FIXED_POINT sum = 1; // Initialize sum of series to the first term (e^0 = 1)
    for (int i = N - 1; i > 0; --i) {
        sum = (FPGA_FIXED_POINT)1 + (x * sum) / (FPGA_FIXED_POINT)i;
    }
    return sum;
}

void read(
    hls::stream<OPTION_TYPE_BOOL> &optionTypeStream,
    hls::stream<FPGA_FIXED_POINT> &spotStream,
    hls::stream<FPGA_FIXED_POINT> &strikeStream,
    hls::stream<FPGA_FIXED_POINT> &tteStream,
    OPTION_TYPE_BOOL optionType[SIZE], 
    FPGA_FIXED_POINT spotPrice[SIZE], 
    FPGA_FIXED_POINT strikePrice[SIZE], 
    FPGA_FIXED_POINT tte[SIZE])
{
	LOOP_READ:for(int i=0; i<SIZE;i++)
	{
		#pragma HLS PIPELINE II=1
		optionTypeStream << optionType[i];
		spotStream << spotPrice[i];
		strikeStream << strikePrice[i];
		tteStream << tte[i];
	}
}


void compute(
    hls::stream<OPTION_TYPE_BOOL> &optionTypeStream,
    hls::stream<FPGA_FIXED_POINT> &spotStream,
    hls::stream<FPGA_FIXED_POINT> &strikeStream,
    hls::stream<FPGA_FIXED_POINT> &tteStream)
    hls::stream<FPGA_FIXED_POINT> &optionResultStream)
{
	LOOP_COMPUTE:for(int i=0; i< SIZE; i++)
	{
        // Read input data
        #pragma HLS PIPELINE II=1
        OPTION_TYPE_BOOL optiontype = optionTypeStream.read();
        FPGA_FIXED_POINT spotprice = spotStream.read();
        FPGA_FIXED_POINT strikeprice = strikeStream.read();
        FPGA_FIXED_POINT tte = tteStream.read();

        // Initialize variables
        FPGA_FIXED_POINT optionPrice = 0.0;
        FPGA_FIXED_POINT d1,d2;
        FPGA_FIXED_POINT inputExp = (FPGA_FIXED_POINT)(-(FPGA_FIXED_POINT)rate * tte);
        FPGA_FIXED_POINT FutureValue = (FPGA_FIXED_POINT)strike * exp_taylor_aprox(inputExp);

        calculate_d1_d2(spot_price, strike_price, rate, volatility, time, &d1, &d2);

        if (option) {
            FPGA_FIXED_POINT Nd1 = P_calculate_Nd1_Nd2(d1);
            FPGA_FIXED_POINT Nd2 = P_calculate_Nd1_Nd2(d2);
            optionPrice = spotprice * Nd1 - FutureValue * Nd2;
        } else {
            FPGA_FIXED_POINT Nd1 = P_calculate_Nd1_Nd2(-d1);
            FPGA_FIXED_POINT Nd2 = P_calculate_Nd1_Nd2(-d2);
            optionPrice = FutureValue * Nd2 - spotprice * Nd1;
        }
        optionResultStream << optionPrice;
	}
}
