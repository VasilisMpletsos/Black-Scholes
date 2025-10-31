#define SIZE 858
#define REPLICATION 1

#include "utility.hpp"

typedef ap_fixed <23,13,AP_RND_CONV > FPGA_FIXED_POINT;
typedef ap_uint <1> OPTION_TYPE_BOOL;

// number of runs
#define RUNS 10000

// number of compute units on FPGA
#define CU 4 // at least 2 (1 for put and 1 for call)
#define QoS 0.5 // quality threshold

// 1.575% risk free rate, logical values from 1% to 3% but depends on the country
#define RISK_FREE_RATE  0.01575
// logical values from 10% to 30% but depends on the stock
#define VOLATILITY 0.25

// Import various libraries
#include <cstdint>
#include "string.h"
#include <cmath>
#include "stdio.h"
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
#include <chrono>

// Include black scholes implementation
#include "black_scholes.hpp"

int main(int argc, char ** argv) {

  // ------------------------------------------------------------------------------------
  // Step 1: Read Data
  // -----------------------------------------------------------------------------------

  cout << "REACHED HERE 1" << endl;

  std::ifstream closeFile("./datasets/option_price.txt");
  std::ifstream strikeFile("./datasets/strike.txt");
  std::ifstream tteFile("./datasets/tte.txt");
  std::ifstream typeFile("./datasets/type.txt");
  cout << "REACHED HERE 2" << endl;
  std::string binaryFile("./build_dir.hw.xilinx_u200_gen3x16_xdma_2_202110_1//kernelBlackScholes.xclbin");
  // std::string binaryFile("./build_dir.hw.xilinx_u200_gen3x16_xdma_2_202110_1/kernelBlackScholes.xclbin");

  cout << "REACHED HERE 3" << endl;

  // Check if files opened successfully
    if (!closeFile || !strikeFile || !tteFile || !typeFile) {
        throw std::runtime_error("Failed to open one or more input files.");
    }

    // Read close prices
    float closePrices[SIZE], strikePrices[SIZE], tte[SIZE];
    int callTypes[SIZE];

    const int BATCH_SIZE = SIZE * REPLICATION;

    float passedClosePrices[BATCH_SIZE], passedStrikePrices[BATCH_SIZE], passedTte[BATCH_SIZE];
    int passedCallTypes[BATCH_SIZE];

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

    for (int r = 0; r < REPLICATION; r++) {
        for (int i = 0; i < SIZE; i++) {
            int index = r * SIZE + i;
            passedClosePrices[index] = closePrices[i];
            passedStrikePrices[index] = strikePrices[i];
            passedTte[index] = tte[i];
            passedCallTypes[index] = callTypes[i];
        }
    }


    // for (int i = 0; i < SIZE; i++) {
    //   cout << "Close Price: " << closePrices[i] << " | Strike Price: " << strikePrices[i] << " | Time to Expire: " << tte[i] << " | Call Type: " << callTypes[i] << endl;
    //   float logInput = closePrices[i]/strikePrices[i];
    //   cout << "Log Input: " << logInput << endl;
    //   float logVal = log(logInput);
    //   cout << "Log Value: " << logVal << endl;
    //   float sqrtVal = sqrt(tte[i]);
    //   cout << "Sqrt Value: " << sqrtVal << endl;
    //   float powerVal = VOLATILITY * VOLATILITY;
    //   float powerVal2 = powerVal * 0.5;
    //   float nD1 = logVal + (RISK_FREE_RATE + powerVal2) * tte[i];
    //   float nD2 = logVal + (RISK_FREE_RATE - powerVal2) * tte[i];
    //   cout << "nD1: " << nD1 << " | nD2: " << nD2 << endl;
    //   float denum = VOLATILITY * sqrtVal + 1e-6;
    //   cout << "Denum: " << denum << endl;
    //   float d1 = (float)(nD1 / denum);
    //   float d2 = (float)(nD2 / denum);
    //   cout << "d1: " << d1 << " | d2: " << d2 << endl;
    // }
 
  // ------------------------------------------------------------------------------------
  // Step 2: Initialize the OpenCL environment
  // -----------------------------------------------------------------------------------

  cout << "REACHED HERE 4" << endl;
  
  cl_int err;
  cl::CommandQueue q;
  cl::Context context;
  std::string CU_id;
  std::vector <cl::Kernel> krnls(CU);

  cout << "REACHED HERE 5" << endl;

  auto devices = xcl::get_xil_devices();
  cout << "REACHED HERE 5.1" << endl;
  auto fileBuf = xcl::read_binary_file(binaryFile);
  cout << "REACHED HERE 5.2" << endl;
  cl::Program::Binaries bins {
    {
      fileBuf.data(), fileBuf.size()
    }
  };

  cout << "REACHED HERE 6" << endl;

  bool valid_device = false;
  std::string krnl_name = "kernelBlackScholes";

  for (unsigned int i = 0; i < devices.size(); i++) {
    auto device = devices[i];
    // Creating Context and Command Queue for selected Device
    OCL_CHECK(err, context = cl::Context(device, nullptr, nullptr, nullptr, & err));
    OCL_CHECK(err, q = cl::CommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE | CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE, & err));

    std::cout << "Trying to program device[" << i << "]: " << device.getInfo <CL_DEVICE_NAME>() << std::endl;
    cl::Program program(context, {
      device
    }, bins, nullptr, & err);
    if (err != CL_SUCCESS) {
      std::cout << "Failed to program device[" << i << "] with xclbin file!\n";
    } else {
      std::cout << "Device[" << i << "]: program successful!\n";
      for (int i = 0; i < CU; i++) {
        CU_id = std::to_string(i + 1);
        std::string KERNEL_NAME_FULL = krnl_name + ":{" + "kernelBlackScholes" + "_" + CU_id + "}";
        OCL_CHECK(err, krnls[i] = cl::Kernel(program, KERNEL_NAME_FULL.c_str(), & err));
      }
      valid_device = true;
      break; // We break because we found a valid device
    }
  }
  if (!valid_device) {
    std::cout << "Failed to program any device found, exit!\n";
    exit(EXIT_FAILURE);
  }

  cout << "REACHED HERE 7" << endl;

  // ------------------------------------------------------------------------------------
  // Step 3: Initialize Buffers and add it to FPGA
  // -----------------------------------------------------------------------------------

  std::vector <cl::Buffer> call_buf(CU);
  std::vector <cl::Buffer> close_buf(CU);
  std::vector <cl::Buffer> strike_buf(CU);
  std::vector <cl::Buffer> tte_buf(CU);
  std::vector <cl::Buffer> out_buf(CU);

  std::cout << "Setting buffers \n";
  // Create the buffers and allocate memory
  for (int i = 0; i < CU; i++) {
    OCL_CHECK(err, call_buf[i] = cl::Buffer(context, CL_MEM_ALLOC_HOST_PTR | CL_MEM_READ_ONLY, BATCH_SIZE * sizeof(OPTION_TYPE_BOOL), NULL, & err));
    OCL_CHECK(err, close_buf[i] = cl::Buffer(context, CL_MEM_ALLOC_HOST_PTR | CL_MEM_READ_ONLY, BATCH_SIZE * sizeof(FPGA_FIXED_POINT), NULL, & err));
    OCL_CHECK(err, strike_buf[i] = cl::Buffer(context, CL_MEM_ALLOC_HOST_PTR | CL_MEM_READ_ONLY, BATCH_SIZE * sizeof(FPGA_FIXED_POINT), NULL, & err));
    OCL_CHECK(err, tte_buf[i] = cl::Buffer(context, CL_MEM_ALLOC_HOST_PTR | CL_MEM_READ_ONLY, BATCH_SIZE * sizeof(FPGA_FIXED_POINT), NULL, & err));
    OCL_CHECK(err, out_buf[i] = cl::Buffer(context, CL_MEM_ALLOC_HOST_PTR | CL_MEM_READ_WRITE, BATCH_SIZE * sizeof(FPGA_FIXED_POINT), NULL, & err));
  }

  std::cout << "Setting arguments for kernel \n";
  for (int i = 0; i < CU; i++) {
    int narg = 0;
    OCL_CHECK(err, err = krnls[i].setArg(narg++, call_buf[i]));
    OCL_CHECK(err, err = krnls[i].setArg(narg++, close_buf[i]));
    OCL_CHECK(err, err = krnls[i].setArg(narg++, strike_buf[i]));
    OCL_CHECK(err, err = krnls[i].setArg(narg++, tte_buf[i]));
    OCL_CHECK(err, err = krnls[i].setArg(narg++, out_buf[i]));
  }

  OCL_CHECK(err, err = q.finish());

  // ------------------------------------------------------------------------------------
  // Step 3: Create buffers and initialize test values
  // ------------------------------------------------------------------------------------

  std::vector < OPTION_TYPE_BOOL * > calloption(CU);
  std::vector < FPGA_FIXED_POINT * > closeprice(CU);
  std::vector < FPGA_FIXED_POINT * > strikeprice(CU);
  std::vector < FPGA_FIXED_POINT * > timetoexpire(CU);
  std::vector < FPGA_FIXED_POINT * > result(CU);

  std::cout << "Enquing data \n";
  for (int i = 0; i < CU; i++) {
    calloption[i] = (OPTION_TYPE_BOOL * ) q.enqueueMapBuffer(call_buf[i], CL_TRUE, CL_MAP_READ, 0, BATCH_SIZE * sizeof(OPTION_TYPE_BOOL));
    closeprice[i] = (FPGA_FIXED_POINT * ) q.enqueueMapBuffer(close_buf[i], CL_TRUE, CL_MAP_READ, 0, BATCH_SIZE * sizeof(FPGA_FIXED_POINT));
    strikeprice[i] = (FPGA_FIXED_POINT * ) q.enqueueMapBuffer(strike_buf[i], CL_TRUE, CL_MAP_READ, 0, BATCH_SIZE * sizeof(FPGA_FIXED_POINT));
    timetoexpire[i] = (FPGA_FIXED_POINT * ) q.enqueueMapBuffer(tte_buf[i], CL_TRUE, CL_MAP_READ, 0, BATCH_SIZE * sizeof(FPGA_FIXED_POINT));
    result[i] = (FPGA_FIXED_POINT * ) q.enqueueMapBuffer(out_buf[i], CL_TRUE, CL_MAP_READ | CL_MAP_WRITE, 0, BATCH_SIZE * sizeof(FPGA_FIXED_POINT));
  }

  // 3. Copy host data into mapped buffers.
  for (int cu = 0; cu < CU; cu++) {
    for (int i = 0; i < BATCH_SIZE; i++) {
        calloption[cu][i] = passedCallTypes[i];
        closeprice[cu][i] = passedClosePrices[i];
        strikeprice[cu][i] = passedStrikePrices[i];
        timetoexpire[cu][i] = passedTte[i];
    }
  }


  //------------- Execution------------

  std::cout << "Pushing data \n";
  for (int i = 0; i < CU; i++)
    OCL_CHECK(err, err = q.enqueueMigrateMemObjects({
      call_buf[i],
      close_buf[i],
      strike_buf[i],
      tte_buf[i],
    }, 0));
  OCL_CHECK(err, err = q.finish());


  // --------------- FPGA Execution ---------------

  std::cout << "Starting execution \n";
  chrono::high_resolution_clock::time_point t1, t2;
  t1 = chrono::high_resolution_clock::now();
  for (int run = 0; run < RUNS; run++) {
    for (int cu = 0; cu < CU; cu++) {
      OCL_CHECK(err, err = q.enqueueTask(krnls[cu]));
    }
  }
  OCL_CHECK(err, err = q.finish());
  t2 = chrono::high_resolution_clock::now();

  std::cout << "Reading results \n";
  for (int cu = 0; cu < CU; cu++) {
    OCL_CHECK(err, err = q.enqueueMigrateMemObjects({
      out_buf[cu]
    }, CL_MIGRATE_MEM_OBJECT_HOST));
  }
  OCL_CHECK(err, err = q.finish());
  chrono::duration <double, std::micro> FPGA_time = (t2 - t1);
  printf("FPGA Time: %f micro seconds\n", FPGA_time.count());

  // --------------- CPU Execution ---------------

  float cpu_option_prices[BATCH_SIZE];
  t1 = chrono::high_resolution_clock::now();
  for (int j = 0; j < RUNS; j++){
    for (int i = 0; i < BATCH_SIZE; i++) {
        Black_Scholes_CPU(passedCallTypes[i] ,passedClosePrices[i], passedStrikePrices[i], RISK_FREE_RATE, VOLATILITY, passedTte[i], &cpu_option_prices[i]);
    }
  }
  // for (int i = 0; i < SIZE; i++) {
  //   cout << cpu_option_prices[i] << " , ";
  // }
  t2 = chrono::high_resolution_clock::now();
  chrono::duration <double, std::milli> CPU_time = t2 - t1;
  printf("CPU Time: %f ms\n", CPU_time.count());

  // --------------- Compare Results ---------------

  printf("--- Compare result CPU Vs FPGA --- \n");
  int counter = 0;
  float sum = 0.0;
  printf("Calculating Diffs \n");
  for (int i = 0; i < BATCH_SIZE; i++) {
    float fpga_value = (float) result[0][i];
    float dif = abs(cpu_option_prices[i] - fpga_value);
    if (dif > 2){
      cout << "----------------------------------------------------------------------------" << endl;
      cout << "Call Type " << passedCallTypes[i] << endl;
      cout << "Strike Price " << passedStrikePrices[i] << endl;
      cout << "Time to expirty " << passedTte[i]  << endl;
      cout << "CPU result " << i+1 << " = " << cpu_option_prices[i] << " | FPGA result= " << fpga_value << " | Diff= " << dif << endl;
    }
    sum += dif;
    // If the difference is less that 0.5% we consider it correct result
    if (dif <= 2) counter++;
  }

  cout << endl;
  cout << "--- FINAL OPTIONS --- " << endl;
  cout << "Correct Predictions = " << counter << " | Size = " << SIZE << endl;
  float score = (float) counter / BATCH_SIZE;
  float mean_diff = sum / BATCH_SIZE;
  cout << "Score = " << score * 100 << " % " << endl;
  cout << "Sum = " << sum << endl;
  cout << "Mean Diff = " << mean_diff << endl;
  cout << "--------------------" << endl;

  printf("Total options calculations done: %d\n", BATCH_SIZE*RUNS);
  printf("Total CPU Time: %f milli seconds\n", CPU_time.count());
  printf("In seconds: %f \n", CPU_time.count()/1000);
  // PRINT AVG TIME
  uint64_t cpu_total_ops = (uint64_t)BATCH_SIZE * RUNS;
  uint64_t fpga_total_ops = cpu_total_ops * CU;
  printf("Average CPU Time per option: %f milli seconds\n", (CPU_time.count()/cpu_total_ops));
  printf("Average FPGA Time per option: %f micro seconds\n", (FPGA_time.count()/fpga_total_ops));

  cout << "--------------------" << endl;

  // --------------- Unmap the used resources from FPGA - Final Cleanup ---------------

  for (int i = 0; i < CU; i++) {
    OCL_CHECK(err, err = q.enqueueUnmapMemObject(call_buf[i], calloption[i]));
    OCL_CHECK(err, err = q.enqueueUnmapMemObject(close_buf[i], closeprice[i]));
    OCL_CHECK(err, err = q.enqueueUnmapMemObject(strike_buf[i], strikeprice[i]));
    OCL_CHECK(err, err = q.enqueueUnmapMemObject(tte_buf[i], timetoexpire[i]));
    OCL_CHECK(err, err = q.enqueueUnmapMemObject(out_buf[i], result[i]));
  }

  OCL_CHECK(err, err = q.finish());

  return (0);
}

