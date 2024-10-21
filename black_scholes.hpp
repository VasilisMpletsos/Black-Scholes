#define inverse_sqrt_2pi 0.39894228040143270286
#define gamma 0.2316519
#define alpha1 0.319381530
#define alpha2 - 0.356563782
#define alpha3 1.781477937
#define alpha4 - 1.821255978
#define alpha5 1.330274429

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

inline float fast_cdf_approximation(float input) {
  // Approximation of the CDF based on Abramowitz and Stegun
  float kappa = 1.0 / (1.0 + gamma * input);
  float polynomial_approximation = kappa * (alpha1 + kappa * (alpha2 + kappa * (alpha3 + kappa * (alpha4 + alpha5 * kappa))));
  return polynomial_approximation;
}

inline float calculate_cumulative_probabilities_Nd1_Nd2(float input) {
  float expVal = exp(-0.5f * input * input);
  float normal_prime = expVal * inverse_sqrt_2pi;

  if (input >= 0.0) {
    return (1.0 - normal_prime * fast_cdf_approximation(input));
  } else {
    return 1.0 - calculate_cumulative_probabilities_Nd1_Nd2(-input);
  }
}

// if option = 0 then return european put option else if option = 1 return call option
void Black_Scholes_CPU(int option, float spot_price[SIZE], float strike_price, float rate, float volatility, float time[SIZE], float optionPrice[SIZE]) {

  // C = S * N(d1) - X * e^(-rt) * N(d2)
  // S = spot price
  // X = strike price
  // r = rate
  // S * N(d1): Present value of the expected stock price received if the option is exercised.
  // -X * e^(-rt) * N(d2): Present value of paying the exercise price.


  float d1, d2;

  // for all stocks
  for (int i = 0; i < SIZE; i++) {

    // Step 1: Calculate d1 and d2
    calculate_prob_factors_d1_d2(spot_price[i], strike_price, rate, volatility, time[i], & d1, & d2);

    // Step 2: Calculate the price of the option
    optionPrice[i] = 0.0;
    float FutureValue = strike_price * exp((-rate) * time[i]);
    if (option) {
      float Nd1 = calculate_cumulative_probabilities_Nd1_Nd2(d1);
      float Nd2 = calculate_cumulative_probabilities_Nd1_Nd2(d2);
      // C = (spot price * N(d1)) - (strike_price * e^(-rt) * N(d2))
      optionPrice[i] = spot_price[i] * Nd1 - FutureValue * Nd2;
    } else {
      float Nd1 = calculate_cumulative_probabilities_Nd1_Nd2(-d1);
      float Nd2 = calculate_cumulative_probabilities_Nd1_Nd2(-d2);
      optionPrice[i] = FutureValue * Nd2 - spot_price[i] * Nd1;
    }
  }
}