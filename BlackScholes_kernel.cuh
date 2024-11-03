#define MAX_THREADS_PER_BLOCK DATA_SIZE
#define MIN_BLOCKS_PER_MP     2
#define GAMMA 0.2316419f

///////////////////////////////////////////////////////////////////////////////
// Polynomial approximation of cumulative normal distribution function
///////////////////////////////////////////////////////////////////////////////
__device__ inline float fast_cdf_approximation_GPU(float d)
{
    const float A1 = 0.31938153f;
    const float A2 = -0.356563782f;
    const float A3 = 1.781477937f;
    const float A4 = -1.821255978f;
    const float A5 = 1.330274429f;
    const float RSQRT2PI = 0.39894228040143267793994605993438f;

    // Calculate the fast approximate division of the input arguments. 
    float K = __fdividef(1.0f, (1.0f + GAMMA * fabsf(d)));

    float cnd = RSQRT2PI * __expf(-0.5f * d * d) * (K * (A1 + K * (A2 + K * (A3 + K * (A4 + K * A5)))));

    // if d is negative we avoid recursive calculation
    if (d > 0) cnd = 1.0f - cnd;

    return cnd;
}


///////////////////////////////////////////////////////////////////////////////
// Black-Scholes formula for both call and put
///////////////////////////////////////////////////////////////////////////////
__device__ inline void BlackScholesBodyGPU(
    int TP, //Option Type
    float S, //Spot price
    float X, //Stirke price
    float R, //Riskl Free rate
    float V, //Volatility rate
    float T, //Time to expiration
    float &OptionResult
)
{
    float sqrtT, expRT;
    float d1, d2, CNDD1, CNDD2;

    sqrtT = __fdividef(1.0F, rsqrtf(T));
    d1 = __fdividef(__logf(S / X) + (R + 0.5f * V * V) * T, V * sqrtT);
    d2 = d1 - V * sqrtT;

    CNDD1 = fast_cdf_approximation_GPU(d1);
    CNDD2 = fast_cdf_approximation_GPU(d2);

    //Calculate Call and Put simultaneously
    expRT = __expf(- R * T);

    if (TP)
    {
        OptionResult = S * CNDD1 - X * expRT * CNDD2;
    }
    else
    {
        OptionResult = X * expRT * (1.0f - CNDD2) - S * (1.0f - CNDD1);
    }
}


////////////////////////////////////////////////////////////////////////////////
//Process an array of optN options on GPU
////////////////////////////////////////////////////////////////////////////////
// TODO: This kernel is not optimized for performance
__launch_bounds__(MAX_THREADS_PER_BLOCK, MIN_BLOCKS_PER_MP)
__global__ void BlackScholesGPU(
    int1 * __restrict d_OptionType,
    float1 * __restrict d_StockPrice,
    float1 * __restrict d_OptionStrike,
    float Riskfree,
    float Volatility,
    float1 * __restrict d_OptionYears,
    float1 * __restrict d_OptionResult
)
{
    // blockDim.x is always 256
    // so we get 256 * blockIdx.x + threadIdx.x
    // examples 
    // 256 * 0 + 0 = 0
    // 256 * 0 + 1 = 1
    // 256 * 1 + 0 = 256
    // 256 * 1 + 1 = 257
    const int opt = blockDim.x * blockIdx.x + threadIdx.x;

    // I do not try to use ILP because currently i am not familiar in that stage with CUDA
    float optionResult;
    BlackScholesBodyGPU(
        d_OptionType[opt].x,
        d_StockPrice[opt].x,
        d_OptionStrike[opt].x,
        Riskfree,
        Volatility,
        d_OptionYears[opt].x,
        optionResult
    );
    d_OptionResult[opt] = make_float1(optionResult);
}