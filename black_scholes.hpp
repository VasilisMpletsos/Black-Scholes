using namespace std;

void calculate_prob_factors_d1_d2(float spotprice, float strike, float rate, float volatility, float time, float *d1, float *d2);

void calculate_prob_factors_d1_d2(float spotprice, float strike, float rate, float volatility, float time, float *d1, float *d2){
  float logVal = log(spotprice / strike);
  float sqrtTime = sqrt(time);
  float sigma_squared = volatility * volatility;
  float sigma_squared_half = sigma_squared / 2 ;
  float nD1 = logVal + (rate + sigma_squared_half) * time;
  float denum = volatility * sqrtTime;
  *d1 = nD1 / denum;
  *d2 = *d1 - denum;
}