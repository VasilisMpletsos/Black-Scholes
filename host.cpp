#define SIZE 5000

#include "utility.hpp"
#include "BS_cpu.hpp"

typedef ap_fixed < 23, 13, AP_RND_CONV > DTYPE;
// typedef float DTYPE;

// number of runs
#define RUNS 1000

// number of compute units on FPGA
#define CU 6 // at least 2 (1 for put and 1 for call)
#define QoS 0.5 // quality threshold

int main(int argc, char ** argv) {

  // ------------------------------------------------------------------------------------
  // Step 1: Initialize the OpenCL environment
  // -----------------------------------------------------------------------------------

  if (argc != 4) {
    std::cout << "Usage: " << argv[0] << " <XCLBIN File> <CLOSE_FILE_PATH> <DATES_FILE_PATH> " << std::endl;
    std::cout << argv[0] << ", " << argv[1] << ", " << argv[2] << ", " << argv[3] << std::endl;
    return EXIT_FAILURE;
  }

  std::string binaryFile = argv[1];
  std::string CLOSE_FILE_PATH = argv[2];
  std::string DATES_FILE_PATH = argv[3];
  cl_int err;
  cl::CommandQueue q;
  cl::Context context;
  std::string CU_id;
  std::vector < cl::Kernel > krnls(CU);
  auto devices = xcl::get_xil_devices();
  auto fileBuf = xcl::read_binary_file(binaryFile);
  cl::Program::Binaries bins {
    {
      fileBuf.data(), fileBuf.size()
    }
  };

  bool valid_device = false;
  std::string krnl_name = "krnl_BS";

  for (unsigned int i = 0; i < devices.size(); i++) {
    auto device = devices[i];
    // Creating Context and Command Queue for selected Device
    OCL_CHECK(err, context = cl::Context(device, nullptr, nullptr, nullptr, & err));
    OCL_CHECK(err, q = cl::CommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE | CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE, & err));

    std::cout << "Trying to program device[" << i << "]: " << device.getInfo < CL_DEVICE_NAME > () << std::endl;
    cl::Program program(context, {
      device
    }, bins, nullptr, & err);
    if (err != CL_SUCCESS) {
      std::cout << "Failed to program device[" << i << "] with xclbin file!\n";
    } else {
      std::cout << "Device[" << i << "]: program successful!\n";
      for (int i = 0; i < CU; i++) {
        CU_id = std::to_string(i + 1);
        std::string KERNEL_NAME_FULL = krnl_name + ":{" + "krnl_BS" + "_" + CU_id + "}";
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

  std::vector < cl::Buffer > time_buf(CU);
  std::vector < cl::Buffer > spotprice_buf(CU);
  std::vector < cl::Buffer > out_buf(CU);

  // Create the buffers and allocate memory

  for (int i = 0; i < CU; i++) {
    OCL_CHECK(err, out_buf[i] = cl::Buffer(context, CL_MEM_ALLOC_HOST_PTR | CL_MEM_READ_WRITE, SIZE * sizeof(DTYPE), NULL, & err));
    OCL_CHECK(err, time_buf[i] = cl::Buffer(context, CL_MEM_ALLOC_HOST_PTR | CL_MEM_READ_ONLY, SIZE * sizeof(DTYPE), NULL, & err));
    OCL_CHECK(err, spotprice_buf[i] = cl::Buffer(context, CL_MEM_ALLOC_HOST_PTR | CL_MEM_READ_ONLY, SIZE * sizeof(DTYPE), NULL, & err));
  }

  // CUs for put option pricing
  for (int i = 0; i < CU / 2; i++) {
    int narg = 0;
    OCL_CHECK(err, err = krnls[i].setArg(narg++, 0));
    OCL_CHECK(err, err = krnls[i].setArg(narg++, spotprice_buf[i]));
    OCL_CHECK(err, err = krnls[i].setArg(narg++, time_buf[i]));
    OCL_CHECK(err, err = krnls[i].setArg(narg++, out_buf[i]));
  }

  // CUs for put option pricing
  for (int i = CU / 2; i < CU; i++) {
    int narg = 0;
    OCL_CHECK(err, err = krnls[i].setArg(narg++, 1));
    OCL_CHECK(err, err = krnls[i].setArg(narg++, spotprice_buf[i]));
    OCL_CHECK(err, err = krnls[i].setArg(narg++, time_buf[i]));
    OCL_CHECK(err, err = krnls[i].setArg(narg++, out_buf[i]));
  }
  OCL_CHECK(err, err = q.finish());
  // ------------------------------------------------------------------------------------
  // Step 2: Create buffers and initialize test values
  // -------------------------------------------------------------------------------

  std::vector < DTYPE * > result(CU);
  std::vector < DTYPE * > spotprice(CU);
  std::vector < DTYPE * > time(CU);

  for (int i = 0; i < CU; i++) {
    result[i] = (DTYPE * ) q.enqueueMapBuffer(out_buf[i], CL_TRUE, CL_MAP_READ | CL_MAP_WRITE, 0, SIZE * sizeof(DTYPE));
    spotprice[i] = (DTYPE * ) q.enqueueMapBuffer(spotprice_buf[i], CL_TRUE, CL_MAP_READ, 0, SIZE * sizeof(DTYPE));
    time[i] = (DTYPE * ) q.enqueueMapBuffer(time_buf[i], CL_TRUE, CL_MAP_READ, 0, SIZE * sizeof(DTYPE));
  }

  DTYPE strike = (DTYPE) 2400; // K
  DTYPE rate = (DTYPE) 0.01575; // r
  DTYPE volatility = (DTYPE) 0.3147330660128807; // sigma

  // read returns and close prices from INB files
  float * close, * years_until_expiry;
  close = (float * ) malloc(SIZE * sizeof(float));
  years_until_expiry = (float * ) malloc(SIZE * sizeof(float));

  float ** result_cpu_put, ** result_cpu_call;
  result_cpu_put = (float ** ) malloc(CU / 2 * sizeof(float * ));
  result_cpu_call = (float ** ) malloc(CU / 2 * sizeof(float * ));
  for (int i = 0; i < CU / 2; i++) {
    result_cpu_put[i] = (float * ) malloc(SIZE * sizeof(float));
    result_cpu_call[i] = (float * ) malloc(SIZE * sizeof(float));
  }

  //------- Read dataset -----
  std::ifstream file;

  file.open(CLOSE_FILE_PATH);

  int index = 0;
  std::string line;
  while (getline(file, line)) {
    close[index] = (float) atof(line.c_str());
    for (int k = 0; k < CU; k++)
      spotprice[k][index] = (DTYPE) atof(line.c_str());
    index++;
  }
  file.close();

  file.open(DATES_FILE_PATH);
  index = 0;
  while (getline(file, line)) {
    years_until_expiry[index] = (float) atof(line.c_str());
    for (int k = 0; k < CU; k++)
      time[k][index] = (DTYPE) atof(line.c_str());
    index++;
  }

  file.close();

  //--- copy the dataset multiple times to reach SIZE data points ---

  for (int k = 0; k < CU; k++) {
    int t = 1;
    for (int i = index; i < SIZE; i++) {
      if (i % (index - 1) == 0) t++;
      close[i] = close[i - (index - 1) * t];
      spotprice[k][i] = (DTYPE) close[i];

      years_until_expiry[i] = years_until_expiry[i - (index - 1) * t];
      time[k][i] = (DTYPE) years_until_expiry[i];
    }
  }

  //------------- Execution------------

  // CUs for put option pricing
  for (int i = 0; i < CU / 2; i++) {
    int narg = 0;
    OCL_CHECK(err, err = krnls[i].setArg(narg++, 0));
    OCL_CHECK(err, err = krnls[i].setArg(narg++, spotprice_buf[i]));
    OCL_CHECK(err, err = krnls[i].setArg(narg++, time_buf[i]));
    OCL_CHECK(err, err = krnls[i].setArg(narg++, out_buf[i]));
  }

  // CUs for put option pricing
  for (int i = CU / 2; i < CU; i++) {
    int narg = 0;
    OCL_CHECK(err, err = krnls[i].setArg(narg++, 1));
    OCL_CHECK(err, err = krnls[i].setArg(narg++, spotprice_buf[i]));
    OCL_CHECK(err, err = krnls[i].setArg(narg++, time_buf[i]));
    OCL_CHECK(err, err = krnls[i].setArg(narg++, out_buf[i]));
  }
  OCL_CHECK(err, err = q.finish());

  for (int i = 0; i < CU; i++)
    OCL_CHECK(err, err = q.enqueueMigrateMemObjects({
      spotprice_buf[i],
      time_buf[i]
    }, 0));
  OCL_CHECK(err, err = q.finish());

  chrono::high_resolution_clock::time_point t1, t2;
  t1 = chrono::high_resolution_clock::now();
  for (int i = 0; i < RUNS; i++) {

    for (int j = 0; j < CU; j++) {
      OCL_CHECK(err, err = q.enqueueTask(krnls[j]));
    }
  }
  OCL_CHECK(err, err = q.finish());
  t2 = chrono::high_resolution_clock::now();

  for (int i = 0; i < CU; i++) {
    OCL_CHECK(err, err = q.enqueueMigrateMemObjects({
      out_buf[i]
    }, CL_MIGRATE_MEM_OBJECT_HOST));
  }
  OCL_CHECK(err, err = q.finish());

  chrono::duration < double > FPGA_time = t2 - t1;

  t1 = chrono::high_resolution_clock::now();
  for (int j = 0; j < RUNS; j++) {

    for (int i = 0; i < CU / 2; i++)
      BS_cpu(0, close, (float) strike, (float) rate, (float) volatility, years_until_expiry, result_cpu_put[i]);
    for (int i = 0; i < CU / 2; i++)
      BS_cpu(1, close, (float) strike, (float) rate, (float) volatility, years_until_expiry, result_cpu_call[i]);
  }
  t2 = chrono::high_resolution_clock::now();
  chrono::duration < double > CPU_time = t2 - t1;

  printf("--- Compare result CPU Vs FPGA --- \n");
  int counter = 0;
  float sum = 0.0;
  printf("PUT OPTIONS \n");
  for (int k = 0; k < CU / 2; k++) {
    for (int i = SIZE - 1; i >= 0; i--) {
      float dif = abs(result_cpu_put[k][i] - (float) result[k][i]) / (result_cpu_put[k][i]) * 100;
      sum += dif;
      // printf("stock %d = %f \n",i,result_cpu[i]);
      // cout << " dif = " << dif << endl;
      if (dif <= QoS)
        counter++;
      else {
        // printf("error at option %d \n", i);
        // cout << "CPU result = " << result_cpu_put[k][i] << endl;
        //cout << "FPGA result = " << (float)result[k][i] << endl;
      }
    }
  }
  cout << "--- PUT OPTIONS --- " << endl;
  cout << "correct = " << counter << " size = " << SIZE << endl;
  float score = (float) counter / ((CU / 2) * SIZE);
  cout << "Score = " << score * 100 << " % " << endl;
  printf("Mean dif = %f % \n", sum / ((CU / 2) * SIZE));
  cout << "----------" << endl;

  printf("CALL OPTIONS \n");
  sum = 0.0;
  counter = 0;
  for (int k = 0; k < CU / 2; k++) {
    for (int i = SIZE - 1; i >= 0; i--) {
      float dif = abs(result_cpu_put[k][i] - (float) result[k][i]) / (result_cpu_put[k][i]) * 100;
      sum += dif;
      // printf("stock %d = %f \n",i,result_cpu[i]);
      // cout << " dif = " << dif << endl;
      if (dif <= QoS)
        counter++;
      else {
        //  printf("error at option %d \n", i);
        // cout << "CPU result = " << result_cpu[i] << endl;
        //cout << "FPGA result = " << (float)result[i] << endl;
      }
    }
  }
  double average_cpu_time = CPU_time.count()/RUNS;
  double average_fpga_time = FPGA_time.count()/RUNS;
  cout << "--- CALL OPTIONS ---" << endl;
  cout << "correct = " << counter << " size = " << SIZE << endl;
  score = (float) counter / ((CU / 2) * SIZE);
  cout << "Score = " << score * 100 << " % " << endl;
  printf("Mean dif = %f % \n", sum / ((CU / 2) * SIZE));
  cout << "----------" << endl;
  cout << "Overall FPGA execution time " << FPGA_time.count() * 1000 << " ms " << endl;
  cout << "Overall CPU execution time " << CPU_time.count() * 1000 << " ms" << endl;
  cout << "Average FPGA execution time " << average_fpga_time * 1000 << " ms " << endl;
  cout << "Average CPU execution time " << average_cpu_time * 1000 << " ms" << endl;
  cout << "speedup " << CPU_time.count() / FPGA_time.count() << endl;
  cout << "CPU options per second: " << (int)SIZE/(average_cpu_time) << endl;
  cout << "FPGA options per second: " << (int)SIZE/(average_fpga_time) << endl;

  printf("--------- \n");

  free(close);
  free(years_until_expiry);
  for (int i = 0; i < CU / 2; i++) {
    free(result_cpu_put[i]);
    free(result_cpu_call[i]);
  }
  free(result_cpu_put);
  free(result_cpu_call);

  for (int i = 0; i < CU; i++) {
    OCL_CHECK(err, err = q.enqueueUnmapMemObject(spotprice_buf[i], spotprice[i]));
    OCL_CHECK(err, err = q.enqueueUnmapMemObject(time_buf[i], time[i]));
    OCL_CHECK(err, err = q.enqueueUnmapMemObject(out_buf[i], result[i]));
  }

  OCL_CHECK(err, err = q.finish());

  return (0);
}
