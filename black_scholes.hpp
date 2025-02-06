// #define inverse_sqrt_2pi 0.39894228040143270286
#define inverse_sqrt_2pi 0.39894228040143267793994605993438
#define gamma 0.2316419
#define alpha1 0.31938153
#define alpha2 -0.356563782
#define alpha3 1.781477937
#define alpha4 -1.821255978
#define alpha5 1.330274429
#include <cmath>
using namespace std;

inline void calculate_prob_factors_d1_d2(float spotprice, float strike, float rate, float volatility, float time, float *d1, float *d2);

inline void calculate_prob_factors_d1_d2(float spotprice, float strike, float rate, float volatility, float time, float *d1, float *d2){
  float logVal = log(spotprice / strike);
  float sqrtTime = sqrt(time);
  float sigma_squared = volatility * volatility;
  float sigma_squared_half = sigma_squared / 2 ;
  float nD1 = logVal + (rate + sigma_squared_half) * time;
  float denum = volatility * sqrtTime;
  *d1 = nD1 / denum;
  *d2 = *d1 - denum;
}

// Classic CDF calculation
float normal_cdf(float x) {
    return 0.5 * (1 + std::erf(x / std::sqrt(2)));
}

// This is a quicker version of the CDF calculation
inline float polynomial_approximation(float input) {
  // Approximation of the CDF based on Abramowitz and Stegun
  float kappa = 1.0 / (1.0 + gamma * fabs(input));
  float polynomial_approximation = kappa * (alpha1 + kappa * (alpha2 + kappa * (alpha3 + kappa * (alpha4 + alpha5 * kappa))));
  return polynomial_approximation;
}

inline float fast_cdf_approximation(float input) {
  float expVal = exp(-0.5f * input * input);
  float normal_prime = expVal * inverse_sqrt_2pi;
  float cdn = normal_prime * polynomial_approximation(input);

  if (input > 0.0) {
    return (1.0 - cdn);
  } 
  return cdn;
}

// if option = 0 then return european put option else if option = 1 return call option
void Black_Scholes_CPU(int optionType, float spot_price, float strike_price, float rate, float volatility, float time, float *optionPrice) {

  // C = S * N(d1) - X * e^(-rt) * N(d2)
  // S = spot price
  // X = strike price
  // r = rate
  // S * N(d1): Present value of the expected stock price received if the option is exercised.
  // -X * e^(-rt) * N(d2): Present value of paying the exercise price.


  float d1, d2;

  // Step 1: Calculate d1 and d2
  calculate_prob_factors_d1_d2(spot_price, strike_price, rate, volatility, time, &d1, &d2);


  // Step 2: Calculate the price of the option
  float FutureValue = strike_price * exp((-rate) * time);
  float Nd1 = fast_cdf_approximation(d1);
  float Nd2 = fast_cdf_approximation(d2);

  if (optionType) {
    // C = (spot price * N(d1)) - (strike_price * e^(-rt) * N(d2))
    *optionPrice = spot_price * Nd1 - FutureValue * Nd2;
  } else {
    // P = (strike_price * e^(-rt) * N(-d2)) − (spot price * N(-d1))
    *optionPrice = FutureValue * (1-Nd2) - spot_price * (1-Nd1);
  }
  
}