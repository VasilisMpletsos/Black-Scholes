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
./black_scholes.o ./datasets/close.txt ./datasets/dates.txt
```

Arguments:
1. Close Prices Filepath
2. Dates Filepath

## Todo:
- [ ] Check that fast CDF approximation is correct
- [ ] Finish CPU implementation
- [ ] Get a dataset

### Usefull Links
* [Black Scholes Wikipedia](https://en.wikipedia.org/wiki/Black%E2%80%93Scholes_model)