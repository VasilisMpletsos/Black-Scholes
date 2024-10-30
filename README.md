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

Times are reported in ms

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

Even the python is not the best implementation nor is the C++ and the speedup is **x620** times more fast!