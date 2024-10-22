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
- [ ] Identify the bug with PUT options returning 0
- [ ] Check that fast CDF approximation is correct
- [ ] d2 calculation can be done in parallel

### Usefull Links
* [Black Scholes Wikipedia](https://en.wikipedia.org/wiki/Black%E2%80%93Scholes_model)
* [Tim Worrall Notes for Black Scholes](http://www.timworrall.com/fin-40008/bscholes.pdf)