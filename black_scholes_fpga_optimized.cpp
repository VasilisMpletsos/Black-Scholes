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

typedef ap_fixed<23,13> FPGA_FIXED_POINT;
typedef ap_uint<1> OPTION_TYPE_BOOL;

#define SIZE 858
#define N_TAYLOR 12
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

#define MIN_TTE 0.000001f
#define SQRT_CORRECTION 1000.0f

// Small helper: abs for ap_fixed (avoid std::fabs overhead)
inline FPGA_FIXED_POINT fp_abs(FPGA_FIXED_POINT x) {
#pragma HLS INLINE
	if (x>0){
		return -x;
	};
	return x;
}

// Taylor-series exp approximation: reduces latency and resources.
// N_TAYLOR trades accuracy <-> latency. Tune it.
inline FPGA_FIXED_POINT exp_taylor_aprox(FPGA_FIXED_POINT x) {
    #pragma HLS INLINE
    FPGA_FIXED_POINT sum = (FPGA_FIXED_POINT)1.0;
    FPGA_FIXED_POINT term = (FPGA_FIXED_POINT)1.0;

    // Use a pipelined loop for HLS to implement accumulation with II=1
    EXP_LOOP: for (int i = 1; i <= N_TAYLOR; ++i) {
        #pragma HLS PIPELINE II=1
        term = term * x / (FPGA_FIXED_POINT)i;
        sum = sum + term;
        // small-term early-exit could be added based on term magnitude
    }
    return sum;
}

inline FPGA_FIXED_POINT polynomial_approximation_fpga(FPGA_FIXED_POINT input) {
    #pragma HLS INLINE
    FPGA_FIXED_POINT abs_input = fp_abs(input);
    FPGA_FIXED_POINT kappa = (FPGA_FIXED_POINT)1.0 / ((FPGA_FIXED_POINT)1.0 + (FPGA_FIXED_POINT)gamma * abs_input);
    // Evaluate polynomial with Horner's method, fully inlined
    FPGA_FIXED_POINT t = (FPGA_FIXED_POINT)alpha5 * kappa + (FPGA_FIXED_POINT)alpha4;
    t = t * kappa + (FPGA_FIXED_POINT)alpha3;
    t = t * kappa + (FPGA_FIXED_POINT)alpha2;
    t = t * kappa + (FPGA_FIXED_POINT)alpha1;
    FPGA_FIXED_POINT polynomial_approximation = kappa * t;
    return polynomial_approximation;
}

inline FPGA_FIXED_POINT fast_cdf_approximation_fpga(FPGA_FIXED_POINT input) {
    #pragma HLS INLINE
    // Use the Abramowitz & Stegun polynomial approximation for the tail
    FPGA_FIXED_POINT inputSquared = input * input;
    FPGA_FIXED_POINT val = (FPGA_FIXED_POINT)(-0.5) * inputSquared;
    FPGA_FIXED_POINT expVal = exp_taylor_aprox(val); // pipelined
    FPGA_FIXED_POINT normal_prime = expVal * (FPGA_FIXED_POINT)inverse_sqrt_2pi;
    FPGA_FIXED_POINT cdn = normal_prime * polynomial_approximation_fpga(input);

    // Bound cdn [0,1]
    if (cdn < 0) cdn = 0;
    if (cdn > 1) cdn = 1;

    if (input > 0) {
        return (FPGA_FIXED_POINT)1.0 - cdn;
    }
    return cdn;
}

inline void calculate_prob_factors_d1_d2_fpga(
    FPGA_FIXED_POINT spotprice,
    FPGA_FIXED_POINT strike,
    FPGA_FIXED_POINT tte,
    FPGA_FIXED_POINT *d1,
    FPGA_FIXED_POINT *d2)
{
    #pragma HLS INLINE
    // NOTE: Using std::log and std::sqrt on floats will instantiate FP IP.
    //       For production/high-throughput, replace these with fixed-point
    //       approximations or specialized IP (CORDIC / lookup + Newton).
    FPGA_FIXED_POINT ratio = spotprice / strike;
    // Cast to float for the standard math helpers (cheap for prototyping,
    // but synthesizes to float IP on FPGA)
    float ratio_f = (float)ratio;
    float logVal_f = log(ratio_f);
    float sqrtVal_f = sqrt((float)tte);
    FPGA_FIXED_POINT logVal = (FPGA_FIXED_POINT)logVal_f;
    FPGA_FIXED_POINT sqrtVal = (FPGA_FIXED_POINT)sqrtVal_f;

    FPGA_FIXED_POINT sigma_squared = (FPGA_FIXED_POINT)VOLATILITY * (FPGA_FIXED_POINT)VOLATILITY;
    FPGA_FIXED_POINT sigma_squared_half = sigma_squared * (FPGA_FIXED_POINT)0.5;
    FPGA_FIXED_POINT nD1 = logVal + ((FPGA_FIXED_POINT)RISK_FREE_RATE + sigma_squared_half) * tte;
    FPGA_FIXED_POINT denum = (FPGA_FIXED_POINT)VOLATILITY * sqrtVal;
    *d1 = nD1 / denum;
    *d2 = *d1 - denum;
}

void read_streams(
    hls::stream<OPTION_TYPE_BOOL> &optionTypeStream,
    hls::stream<FPGA_FIXED_POINT> &spotStream,
    hls::stream<FPGA_FIXED_POINT> &strikeStream,
    hls::stream<FPGA_FIXED_POINT> &tteStream,
    OPTION_TYPE_BOOL optionType[SIZE],
    FPGA_FIXED_POINT spotPrice[SIZE],
    FPGA_FIXED_POINT strikePrice[SIZE],
    FPGA_FIXED_POINT tte[SIZE])
{
    // Read side: pipeline this loop to feed the dataflow
    READ_LOOP: for (int i = 0; i < SIZE; i++) {
        #pragma HLS PIPELINE II=1
        optionTypeStream << optionType[i];
        spotStream << spotPrice[i];
        strikeStream << strikePrice[i];
        tteStream << tte[i];
    }
}

void compute_streams(
    hls::stream<OPTION_TYPE_BOOL> &optionTypeStream,
    hls::stream<FPGA_FIXED_POINT> &spotStream,
    hls::stream<FPGA_FIXED_POINT> &strikeStream,
    hls::stream<FPGA_FIXED_POINT> &tteStream,
    hls::stream<FPGA_FIXED_POINT> &optionResultStream)
{
    // Top compute loop: try to achieve II=1
    COMPUTE_LOOP: for (int i = 0; i < SIZE; i++) {
        #pragma HLS PIPELINE II=1
        OPTION_TYPE_BOOL optiontype = optionTypeStream.read();
        FPGA_FIXED_POINT spot_price = spotStream.read();
        FPGA_FIXED_POINT strike_price = strikeStream.read();
        FPGA_FIXED_POINT tte = tteStream.read();

        FPGA_FIXED_POINT optionPrice = 0.0;
        FPGA_FIXED_POINT d1, d2;
        FPGA_FIXED_POINT inputExp = (FPGA_FIXED_POINT)(-(FPGA_FIXED_POINT)RISK_FREE_RATE * tte);
        FPGA_FIXED_POINT FutureValue = strike_price * exp_taylor_aprox(inputExp);

        calculate_prob_factors_d1_d2_fpga(spot_price, strike_price, tte, &d1, &d2);

        FPGA_FIXED_POINT Nd1 = fast_cdf_approximation_fpga(d1);
        FPGA_FIXED_POINT Nd2 = fast_cdf_approximation_fpga(d2);

        // Bound probabilities
        if (Nd1 < 0) Nd1 = 0;
        if (Nd1 > 1) Nd1 = 1;
        if (Nd2 < 0) Nd2 = 0;
        if (Nd2 > 1) Nd2 = 1;

        if (optiontype) {
            optionPrice = spot_price * Nd1 - FutureValue * Nd2;
        } else {
            optionPrice = FutureValue * ((FPGA_FIXED_POINT)1.0 - Nd2) - spot_price * ((FPGA_FIXED_POINT)1.0 - Nd1);
        }

        if (optionPrice < 0) optionPrice = 0;
        optionResultStream << optionPrice;
    }
}

void write_streams(hls::stream<FPGA_FIXED_POINT> &optionStream, FPGA_FIXED_POINT optionPrice[SIZE]) {
    WRITE_LOOP: for (int i = 0; i < SIZE; i++) {
        #pragma HLS PIPELINE II=1
        optionPrice[i] = optionStream.read();
    }
}

extern "C" {
    void kernelBlackScholes(
        OPTION_TYPE_BOOL optionType[SIZE],
        FPGA_FIXED_POINT spotprice[SIZE],
        FPGA_FIXED_POINT strikeprice[SIZE],
        FPGA_FIXED_POINT time[SIZE],
        FPGA_FIXED_POINT optionPrice[SIZE])
    {
        #pragma HLS INTERFACE m_axi port=optionType bundle=gmem0 offset=slave
        #pragma HLS INTERFACE m_axi port=spotprice bundle=gmem1 offset=slave
        #pragma HLS INTERFACE m_axi port=strikeprice bundle=gmem2 offset=slave
        #pragma HLS INTERFACE m_axi port=time bundle=gmem3 offset=slave
        #pragma HLS INTERFACE m_axi port=optionPrice bundle=gmem4 offset=slave

                // Control interface for HLS block
        #pragma HLS INTERFACE s_axilite port=optionType bundle=control
        #pragma HLS INTERFACE s_axilite port=spotprice bundle=control
        #pragma HLS INTERFACE s_axilite port=strikeprice bundle=control
        #pragma HLS INTERFACE s_axilite port=time bundle=control
        #pragma HLS INTERFACE s_axilite port=optionPrice bundle=control
        #pragma HLS INTERFACE s_axilite port=return bundle=control

        static hls::stream<OPTION_TYPE_BOOL> optionTypeStream("optionTypeStream");
        static hls::stream<FPGA_FIXED_POINT> spotpriceStream("spotpriceStream");
        static hls::stream<FPGA_FIXED_POINT> strikepriceStream("strikepriceStream");
        static hls::stream<FPGA_FIXED_POINT> timeStream("timeStream");
        static hls::stream<FPGA_FIXED_POINT> optionStream("optionStream");

        // Increase stream depth to decouple pipeline stages (tune as needed)
        #pragma HLS STREAM variable=optionTypeStream depth=32
        #pragma HLS STREAM variable=spotpriceStream depth=32
        #pragma HLS STREAM variable=strikepriceStream depth=32
        #pragma HLS STREAM variable=timeStream depth=32
        #pragma HLS STREAM variable=optionStream depth=32

        #pragma HLS DATAFLOW

        read_streams(optionTypeStream, spotpriceStream, strikepriceStream, timeStream,
                     optionType, spotprice, strikeprice, time);
        compute_streams(optionTypeStream, spotpriceStream, strikepriceStream, timeStream, optionStream);
        write_streams(optionStream, optionPrice);
    }
}
