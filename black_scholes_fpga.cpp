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

#define SIZE 878
#define N 12
#define SQRT_MAGIC_F 0x5f3759df
#define RISK_FREE_RATE  0.01575
#define VOLATILITY 0.25
#define inverse_sqrt_2pi 0.39894228040143267793994605993438
#define gamma 0.2316419
#define alpha1 0.31938153
#define alpha2 -0.356563782
#define alpha3 1.781477937
#define alpha4 -1.821255978
#define alpha5 1.330274429


inline void calculate_prob_factors_d1_d2_fpga(FPGA_FIXED_POINT spotprice, FPGA_FIXED_POINT strike, FPGA_FIXED_POINT tte, FPGA_FIXED_POINT *d1, FPGA_FIXED_POINT *d2);
inline FPGA_FIXED_POINT polynomial_approximation_fpga(FPGA_FIXED_POINT input);


// Returns approximate value of e^x using sum of first n terms of Taylor Series
inline FPGA_FIXED_POINT exp_taylor_aprox(FPGA_FIXED_POINT x) {
    FPGA_FIXED_POINT sum = 1; // Initialize sum of series to the first term (e^0 = 1)
    for (int i = N - 1; i > 0; --i) {
        sum = (FPGA_FIXED_POINT)1 + (x * sum) / (FPGA_FIXED_POINT)i;
    }
    return sum;
}

inline void calculate_prob_factors_d1_d2_fpga(
    FPGA_FIXED_POINT spotprice,
    FPGA_FIXED_POINT strike,
    FPGA_FIXED_POINT tte,
    FPGA_FIXED_POINT *d1,
    FPGA_FIXED_POINT *d2)
{
    FPGA_FIXED_POINT logInput = (FPGA_FIXED_POINT) (spotprice/strike);
    FPGA_FIXED_POINT logVal = (FPGA_FIXED_POINT) log((float)logInput);
    FPGA_FIXED_POINT sqrtVal = (FPGA_FIXED_POINT)sqrt((float)tte);
    FPGA_FIXED_POINT powerVal = (FPGA_FIXED_POINT)VOLATILITY * (FPGA_FIXED_POINT)VOLATILITY;
    FPGA_FIXED_POINT powerVal2 = powerVal * (FPGA_FIXED_POINT)0.5;
    FPGA_FIXED_POINT nD1 = logVal + (FPGA_FIXED_POINT)((FPGA_FIXED_POINT)RISK_FREE_RATE + powerVal2) * tte;
    FPGA_FIXED_POINT nD2 = logVal + (FPGA_FIXED_POINT)((FPGA_FIXED_POINT)RISK_FREE_RATE - powerVal2) * tte;
    FPGA_FIXED_POINT denum = (FPGA_FIXED_POINT)((FPGA_FIXED_POINT)VOLATILITY * sqrtVal);
    *d1 = (FPGA_FIXED_POINT)(nD1 / denum);
    *d2 = (FPGA_FIXED_POINT)(nD2 / denum);
}

inline FPGA_FIXED_POINT polynomial_approximation_fpga(FPGA_FIXED_POINT input) {
    // Approximation of the CDF based on Abramowitz and Stegun  
    FPGA_FIXED_POINT kappa = (FPGA_FIXED_POINT)1.0 / ((FPGA_FIXED_POINT)1.0 + (FPGA_FIXED_POINT)gamma * input);
    FPGA_FIXED_POINT polynomial_approximation = kappa * ((FPGA_FIXED_POINT)alpha1 + kappa * ((FPGA_FIXED_POINT)alpha2 + kappa * ((FPGA_FIXED_POINT)alpha3 + kappa * ((FPGA_FIXED_POINT)alpha4 + (FPGA_FIXED_POINT)alpha5 * kappa))));
    return polynomial_approximation;
}

inline FPGA_FIXED_POINT fast_cdf_approximation_fpga(FPGA_FIXED_POINT input) {
    FPGA_FIXED_POINT inputSquared = (FPGA_FIXED_POINT)(input * input);
    FPGA_FIXED_POINT val = (FPGA_FIXED_POINT)((FPGA_FIXED_POINT)0.5 * -inputSquared);
    FPGA_FIXED_POINT expVal = exp_taylor_aprox(val);
    FPGA_FIXED_POINT normal_prime = expVal * (FPGA_FIXED_POINT)inverse_sqrt_2pi;
    FPGA_FIXED_POINT polynomial_approximation = polynomial_approximation_fpga(input);
    FPGA_FIXED_POINT cdn = normal_prime * polynomial_approximation;
    if (input >= 0){
        return ((FPGA_FIXED_POINT)1.0 - cdn);
    }
    return cdn;
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
    hls::stream<FPGA_FIXED_POINT> &tteStream,
    hls::stream<FPGA_FIXED_POINT> &optionResultStream)
{
	LOOP_COMPUTE:for(int i=0; i< SIZE; i++)
	{
        // Read input data
        #pragma HLS PIPELINE II=1
        OPTION_TYPE_BOOL optiontype = optionTypeStream.read();
        FPGA_FIXED_POINT spot_price = spotStream.read();
        FPGA_FIXED_POINT strike_price = strikeStream.read();
        FPGA_FIXED_POINT tte = tteStream.read();

        // Initialize variables
        FPGA_FIXED_POINT optionPrice = 0.0;
        FPGA_FIXED_POINT d1,d2;
        FPGA_FIXED_POINT inputExp = (FPGA_FIXED_POINT)(-(FPGA_FIXED_POINT)RISK_FREE_RATE * tte);
        FPGA_FIXED_POINT FutureValue = strike_price * exp_taylor_aprox(inputExp);

        calculate_prob_factors_d1_d2_fpga(spot_price, strike_price, tte, &d1, &d2);

        if (optiontype) {
            FPGA_FIXED_POINT Nd1 = fast_cdf_approximation_fpga(d1);
            FPGA_FIXED_POINT Nd2 = fast_cdf_approximation_fpga(d2);
            optionPrice = spot_price * Nd1 - FutureValue * Nd2;
        } else {
            FPGA_FIXED_POINT Nd1 = fast_cdf_approximation_fpga(-d1);
            FPGA_FIXED_POINT Nd2 = fast_cdf_approximation_fpga(-d2);
            optionPrice = FutureValue * Nd2 - spot_price * Nd1;
        }
        optionResultStream << optionPrice;
	}
}

void write(hls::stream<FPGA_FIXED_POINT> &optionStream, FPGA_FIXED_POINT optionPrice[SIZE])
{
	LOOP_WRITE:for(int i=0; i< SIZE; i++){
        #pragma HLS PIPELINE II=1
		optionPrice[i] = optionStream.read();
	}
}

extern "C" {
    void kernelBlackScholes(OPTION_TYPE_BOOL optionType[SIZE], FPGA_FIXED_POINT spotprice[SIZE], FPGA_FIXED_POINT strikeprice[SIZE],  FPGA_FIXED_POINT time[SIZE], FPGA_FIXED_POINT optionPrice[SIZE]) {
        #pragma HLS INTERFACE m_axi port=optionType bundle=gmem0
        #pragma HLS INTERFACE m_axi port=spotprice bundle=gmem1
        #pragma HLS INTERFACE m_axi port=strikeprice bundle=gmem2
        #pragma HLS INTERFACE m_axi port=time bundle=gmem3
        #pragma HLS INTERFACE m_axi port=optionPrice bundle=gmem4

        static hls::stream<OPTION_TYPE_BOOL> optionTypeStream("read");
        static hls::stream<FPGA_FIXED_POINT> spotpriceStream("read");
        static hls::stream<FPGA_FIXED_POINT> strikepriceStream("read");
        static hls::stream<FPGA_FIXED_POINT> timeStream("read");
        static hls::stream<FPGA_FIXED_POINT> optionStream("write");

        // Add pragmas to the streams
        #pragma HLS STREAM variable=optionTypeStream depth=2
        #pragma HLS STREAM variable=spotpriceStream depth=2
        #pragma HLS STREAM variable=strikepriceStream depth=2
        #pragma HLS STREAM variable=timeStream depth=2
        #pragma HLS STREAM variable=optionStream depth=2

        #pragma HLS DATAFLOW
        read(optionTypeStream,spotpriceStream,strikepriceStream,timeStream,optionType,spotprice,strikeprice,time);
        compute(optionTypeStream,spotpriceStream,strikepriceStream,timeStream,optionStream);
        write(optionStream, optionPrice);
  }
}