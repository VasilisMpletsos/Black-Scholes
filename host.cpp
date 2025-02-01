#define SIZE 858

// #include "utility.hpp"

// typedef ap_fixed <23,13,AP_RND_CONV > DTYPE;
// typedef ap_uint<1> OPTION_TYPE_BOOL;
// typedef float DTYPE;

// number of runs
#define RUNS 1000

// number of compute units on FPGA
#define CU 6 // at least 2 (1 for put and 1 for call)
#define QoS 0.5 // quality threshold

// 1.575% risk free rate, logical values from 1% to 3% but depends on the country
#define RISK_FREE_RATE  0.01575
// logical values from 10% to 30% but depends on the stock
#define VOLATILITY 0.25

// Import various libraries
// #include "string.h"
#include <cmath>
// #include "stdio.h"
#include <iostream>
#include <fstream>
#include <vector>
// #include <sstream>
// #include <unistd.h>
// #include <stdint.h>
// #include <stdlib.h>
// #include <time.h>
#include <cstdio>
#include <cstring>
// #include <algorithm>
#include <chrono>

// Include black scholes implementation
#include "black_scholes.hpp"

int main(int argc, char ** argv) {

  // ------------------------------------------------------------------------------------
  // Step 1: Read Data
  // -----------------------------------------------------------------------------------
  std::cout << "Reading data...\n";
  std::ifstream closeFile("./datasets/option_price.txt");
  std::ifstream strikeFile("./datasets/strike.txt");
  std::ifstream tteFile("./datasets/tte.txt");
  std::ifstream typeFile("./datasets/type.txt");
  std::string binaryFile("./kernelBlackScholes.xclbin");

  // Check if files opened successfully
    if (!closeFile || !strikeFile || !tteFile || !typeFile) {
        throw std::runtime_error("Failed to open one or more input files.");
    }

    // Read close prices
    float closePrices[SIZE], strikePrices[SIZE], tte[SIZE];
    int callTypes[SIZE];

    printf("Reading data...\n");
    // Load data into host arrays
    for (int i = 0; i < SIZE; i++) {
        std::string line;

        std::getline(closeFile, line);
        closePrices[i] = std::stof(line);

        std::getline(strikeFile, line);
        strikePrices[i] = std::stof(line);

        std::getline(tteFile, line);
        tte[i] = std::stof(line);

        std::getline(typeFile, line);
        callTypes[i] = std::stoi(line);  // Assuming type is an integer
    }
 
  // ------------------------------------------------------------------------------------
  // Step 2: Initialize the OpenCL environment
  // -----------------------------------------------------------------------------------

  // cl_int err;
  // cl::CommandQueue q;
  // cl::Context context;
  // std::string CU_id;
  // std::vector <cl::Kernel> krnls(CU);
  // auto devices = xcl::get_xil_devices();
  // auto fileBuf = xcl::read_binary_file(binaryFile);
  // cl::Program::Binaries bins {
  //   {
  //     fileBuf.data(), fileBuf.size()
  //   }
  // };

  // bool valid_device = false;
  // std::string krnl_name = "kernelBlackScholes";

  // for (unsigned int i = 0; i < devices.size(); i++) {
  //   auto device = devices[i];
  //   // Creating Context and Command Queue for selected Device
  //   OCL_CHECK(err, context = cl::Context(device, nullptr, nullptr, nullptr, & err));
  //   OCL_CHECK(err, q = cl::CommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE | CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE, & err));

  //   std::cout << "Trying to program device[" << i << "]: " << device.getInfo <CL_DEVICE_NAME>() << std::endl;
  //   cl::Program program(context, {
  //     device
  //   }, bins, nullptr, & err);
  //   if (err != CL_SUCCESS) {
  //     std::cout << "Failed to program device[" << i << "] with xclbin file!\n";
  //   } else {
  //     std::cout << "Device[" << i << "]: program successful!\n";
  //     for (int i = 0; i < CU; i++) {
  //       CU_id = std::to_string(i + 1);
  //       std::string KERNEL_NAME_FULL = krnl_name + ":{" + "kernelBlackScholes" + "_" + CU_id + "}";
  //       OCL_CHECK(err, krnls[i] = cl::Kernel(program, KERNEL_NAME_FULL.c_str(), & err));
  //     }
  //     valid_device = true;
  //     break; // We break because we found a valid device
  //   }
  // }
  // if (!valid_device) {
  //   std::cout << "Failed to program any device found, exit!\n";
  //   exit(EXIT_FAILURE);
  // }

  // // ------------------------------------------------------------------------------------
  // // Step 3: Initialize Buffers and add it to FPGA
  // // -----------------------------------------------------------------------------------

  // std::vector <cl::Buffer> tte_buf(SIZE);
  // std::vector <cl::Buffer> close_buf(SIZE);
  // std::vector <cl::Buffer> strike_buf(SIZE);
  // std::vector <cl::Buffer> call_buf(SIZE);
  // std::vector <cl::Buffer> out_buf(SIZE);

  // // Create the buffers and allocate memory

  // for (int i = 0; i < SIZE; i++) {
  //   OCL_CHECK(err, out_buf[i] = cl::Buffer(context, CL_MEM_ALLOC_HOST_PTR | CL_MEM_READ_WRITE, SIZE * sizeof(DTYPE), NULL, & err));
  //   OCL_CHECK(err, strike_buf[i] = cl::Buffer(context, CL_MEM_ALLOC_HOST_PTR | CL_MEM_READ_WRITE, SIZE * sizeof(DTYPE), NULL, & err));
  //   OCL_CHECK(err, tte_buf[i] = cl::Buffer(context, CL_MEM_ALLOC_HOST_PTR | CL_MEM_READ_ONLY, SIZE * sizeof(DTYPE), NULL, & err));
  //   OCL_CHECK(err, close_buf[i] = cl::Buffer(context, CL_MEM_ALLOC_HOST_PTR | CL_MEM_READ_ONLY, SIZE * sizeof(DTYPE), NULL, & err));
  //   OCL_CHECK(err, call_buf[i] = cl::Buffer(context, CL_MEM_ALLOC_HOST_PTR | CL_MEM_READ_ONLY, SIZE * sizeof(OPTION_TYPE_BOOL), NULL, & err));
  // }

  // for (int i = 0; i < SIZE; i++) {
  //   int narg = 0;
  //   OCL_CHECK(err, err = krnls[i].setArg(narg++, 0));
  //   OCL_CHECK(err, err = krnls[i].setArg(narg++, close_buf[i]));
  //   OCL_CHECK(err, err = krnls[i].setArg(narg++,tte_buf[i]));
  //   OCL_CHECK(err, err = krnls[i].setArg(narg++, strike_buf[i]));
  //   OCL_CHECK(err, err = krnls[i].setArg(narg++, call_buf[i]));
  //   OCL_CHECK(err, err = krnls[i].setArg(narg++, out_buf[i]));
  // }

  // OCL_CHECK(err, err = q.finish());

  // // ------------------------------------------------------------------------------------
  // // Step 3: Create buffers and initialize test values
  // // -------------------------------------------------------------------------------
  // std::vector < DTYPE * > result(SIZE);
  // std::vector < DTYPE * > closeprice(SIZE);
  // std::vector < DTYPE * > tte(SIZE);
  // std::vector < DTYPE * > strikeprice(SIZE);
  // std::vector < OPTION_TYPE_BOOL * > calloption(SIZE);

  // for (int i = 0; i < SIZE; i++) {
  //   result[i] = (DTYPE * ) q.enqueueMapBuffer(out_buf[i], CL_TRUE, CL_MAP_READ | CL_MAP_WRITE, 0, SIZE * sizeof(DTYPE));
  //   closeprice[i] = (DTYPE * ) q.enqueueMapBuffer(close_buf[i], CL_TRUE, CL_MAP_READ, 0, SIZE * sizeof(DTYPE));
  //   tte[i] = (DTYPE * ) q.enqueueMapBuffer(tte_buf[i], CL_TRUE, CL_MAP_READ, 0, SIZE * sizeof(DTYPE));
  //   strikeprice[i] = (DTYPE * ) q.enqueueMapBuffer(strike_buf[i], CL_TRUE, CL_MAP_READ, 0, SIZE * sizeof(DTYPE));
  //   calloption[i] = (DTYPE * ) q.enqueueMapBuffer(call_buf[i], CL_TRUE, CL_MAP_READ, 0, SIZE * sizeof(DTYPE));
  // }


  // DTYPE rate = (DTYPE) 0.01575; // r
  // DTYPE volatility = (DTYPE) 0.25; // sigma

  //------------- Execution------------

  // TODO: Write the code to execute the kernel on FPGA
  // for (int i = 0; i < CU; i++)
  //   OCL_CHECK(err, err = q.enqueueMigrateMemObjects({
  //     close_buf[i],
  //     tte_buf[i],
  //     strike_buf[i],
  //     call_buf[i]
  //   }, 0));
  // OCL_CHECK(err, err = q.finish());

  // chrono::high_resolution_clock::time_point t1, t2;
  // t1 = chrono::high_resolution_clock::now();
  // for (int i = 0; i < RUNS; i++) {
  //   for (int j = 0; j < CU; j++) {
  //     OCL_CHECK(err, err = q.enqueueTask(krnls[j]));
  //   }
  // }
  // OCL_CHECK(err, err = q.finish());
  // t2 = chrono::high_resolution_clock::now();

  // for (int i = 0; i < CU; i++) {
  //   OCL_CHECK(err, err = q.enqueueMigrateMemObjects({
  //     strike_buf[i]
  //   }, CL_MIGRATE_MEM_OBJECT_HOST));
  // }
  // OCL_CHECK(err, err = q.finish());

  // chrono::duration < double > FPGA_time = t2 - t1;

  float cpu_option_prices[SIZE];
  chrono::high_resolution_clock::time_point t1, t2;
  t1 = chrono::high_resolution_clock::now();
  for (int j = 0; j < RUNS; j++){
    for (int i = 0; i < SIZE; i++) {
        float a, b;
        Black_Scholes_CPU(callTypes[i] ,closePrices[i], strikePrices[i], RISK_FREE_RATE, VOLATILITY, tte[i], &cpu_option_prices[i]);
    }
  }
  t2 = chrono::high_resolution_clock::now();
  chrono::duration <double, std::milli> CPU_time = t2 - t1;
  printf("CPU Time: %f ms\n", CPU_time.count());


  // // TODO: Compare the results of the FPGA to the CPU
  // printf("--- Compare result CPU Vs FPGA --- \n");
  // int counter = 0;
  // float sum = 0.0;
  // printf("Calculating Diffs \n");
  // for (int i = 0; i < SIZE; i++) {
  //   float dif = abs(cpu_option_prices[i] - (float) result[i]) / (cpu_option_prices[i]) * 100;
  //   sum += dif;
  //   if (dif <= QoS)
  //     counter++;
  //   else {
  //     // printf("error at option %d \n", i);
  //     // cout << "CPU result = " << cpu_option_prices[i] << endl;
  //     // cout << "FPGA result = " << (float)result[i] << endl;
  //   }
  // }

  // cout << "--- FINAL OPTIONS --- " << endl;
  // cout << "correct = " << counter << " size = " << SIZE << endl;
  // float score = (float) counter / SIZE;
  // cout << "Score = " << score * 100 << " % " << endl;
  // printf("Mean dif = %f % \n", sum / SIZE)
  // cout << "----------" << endl;


  double average_cpu_time = CPU_time.count()/RUNS;
  // double average_fpga_time = FPGA_time.count()/RUNS;
  // cout << "Overall FPGA execution time " << FPGA_time.count() * 1000 << " ms " << endl;
  cout << "Overall CPU execution time " << CPU_time.count() * 1000 << " ms" << endl;
  // cout << "Average FPGA execution time " << average_fpga_time * 1000 << " ms " << endl;
  cout << "Average CPU execution time " << average_cpu_time * 1000 << " ms" << endl;
  // cout << "Speedup " << CPU_time.count() / FPGA_time.count() << endl;
  cout << "CPU options per second: " << (int)SIZE/(average_cpu_time) << endl;
  // cout << "FPGA options per second: " << (int)SIZE/(average_fpga_time) << endl;

  printf("--------- \n");

  // for (int i = 0; i < CU; i++) {
  //   OCL_CHECK(err, err = q.enqueueUnmapMemObject(close_buf[i], closeprice[i]));
  //   OCL_CHECK(err, err = q.enqueueUnmapMemObject(tte_buf[i], tte[i]));
  // }

  // OCL_CHECK(err, err = q.finish());

  return (0);
}
