# Black Scholes FPGA

## Introduction
This is a repository of the work for my final assignment in the UNI for the MSc degree. Because i like trading stocks a lot i decided to implement the simple Black Scholes algorithm on CPU, GPU & FPGA to see gaining advancments!

## Notes

In order to compile you have to download gnu compiler g++ and:
```
g++ main.cpp -o black_scholes.o
```

And to run:
```
./black_scholes.o ./datasets/option_price.txt ./datasets/strike.txt ./datasets/tte.txt ./datasets/type.txt
```

Arguments:
1. Close Prices Filepath
2. Strike Prices Filepath
3. Time To Expiration (TTE) Filepath
4. Option Type Filepath

## Todo:
- [x] Finish CPU implementation
- [x] Get a dataset
- [x] Check that fast CDF approximation is correct
- [x] Python version in order to run on GPU
- [ ] d2 calculation can be done in parallel
- [ ] Profiling for the algorithm (**Prioroty**)
- [ ] Contact Papaefstathiou & Aggelos after profiling

### Usefull Links
* [Black Scholes Wikipedia](https://en.wikipedia.org/wiki/Black%E2%80%93Scholes_model)
* [Tim Worrall Notes for Black Scholes](http://www.timworrall.com/fin-40008/bscholes.pdf)

## Comparisons

Times are reported in **ms**.

| Run         | Python      | C++         |
|-------------|-------------|-------------|
| 1           | 83.78       | 0.215180    |
| 2           | 86.064      | 0.212246    |
| 3           | 82.999      | 0.177605    |
| 4           | 81.916      | 0.217485    |
| 5           | 81.56       | 0.068165    |
| 6           | 81.144      | 0.177815    |
| 7           | 81.904      | 0.210710    |
| 8           | 82.483      | 0.085695    |
| 9           | 94.895      | 0.095403    |
| 10          | 86.902      | 0.131441    |
| **Average** | **84.3647** | **0.135917**|

The **Python** implementation is is **x620 times more slow** than **C++**!

****

Now the following comparison is between **fast_cdf_approximation** and **normal_cdf** functions:
| Run         | Fast Approximation| Normal CDF  |
|-------------|-------------------|-------------|
| 1           | 0.070818          | 0.215529    |
| 2           | 0.086184          | 0.075847    |
| 3           | 0.128787          | 0.184100    |
| 4           | 0.127878          | 0.072495    |
| 5           | 0.231174          | 0.187733    |
| 6           | 0.070958          | 0.094425    |
| 7           | 0.068235          | 0.132488    |
| 8           | 0.136400          | 0.096380    |
| 9           | 0.069562          | 0.101339    |
| 10          | 0.070050          | 0.135142    |
| **Average** | **0.1060046**     | **0.129548**|

The fast approximation seems to be **x1.22 times faster**
It would be even more helpfull due the the embedding nature of FPGA 
****