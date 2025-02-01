#define CL_HPP_CL_1_2_DEFAULT_BUILD
#define CL_HPP_TARGET_OPENCL_VERSION 120
#define CL_HPP_MINIMUM_OPENCL_VERSION 120
#define CL_HPP_ENABLE_PROGRAM_CONSTRUCTION_FROM_ARRAY_COMPATIBILITY 1
#define CL_USE_DEPRECATED_OPENCL_1_2_APIS

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
#include <random>
#include <cstring>
#include <CL/cl2.hpp>
// #include "xcl2.hpp"
#include <ctime>
// #include "ap_fixed.h"
#include "/tools/Xilinx/Vitis_HLS/2022.1/include/ap_fixed.h"

std::vector < cl::Device > get_xilinx_devices();
char * read_binary_file(const std::string & xclbin_file_name, unsigned & nb);

using namespace std;

// Utility functions
// ------------------------------------------------------------------------------------
std::vector < cl::Device > get_xilinx_devices() {
  size_t i;
  cl_int err;
  std::vector < cl::Platform > platforms;
  err = cl::Platform::get( & platforms);
  cl::Platform platform;
  for (i = 0; i < platforms.size(); i++) {
    platform = platforms[i];
    std::string platformName = platform.getInfo < CL_PLATFORM_NAME > ( & err);
    if (platformName == "Xilinx") {
      std::cout << "INFO: Found Xilinx Platform" << std::endl;
      break;
    }
  }
  if (i == platforms.size()) {
    std::cout << "ERROR: Failed to find Xilinx platform" << std::endl;
    exit(EXIT_FAILURE);
  }

  //Getting ACCELERATOR Devices and selecting 1st such device
  std::vector < cl::Device > devices;
  err = platform.getDevices(CL_DEVICE_TYPE_ACCELERATOR, & devices);
  return devices;
}

char *read_binary_file(const std::string & xclbin_file_name, unsigned & nb) {
  if (access(xclbin_file_name.c_str(), R_OK) != 0) {
    printf("ERROR: %s xclbin not available please build\n", xclbin_file_name.c_str());
    exit(EXIT_FAILURE);
  }
  //Loading XCL Bin into char buffer
  std::cout << "INFO: Loading '" << xclbin_file_name << "'\n";
  std::ifstream bin_file(xclbin_file_name.c_str(), std::ifstream::binary);
  bin_file.seekg(0, bin_file.end);
  nb = bin_file.tellg();
  bin_file.seekg(0, bin_file.beg);
  char * buf = new char[nb];
  bin_file.read(buf, nb);
  return buf;
}
