#ifdef GPU_ENABLED
#include <sys/time.h>
#include <err.h>
#include <stdio.h>
#include <string.h>

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

#include "gpu_pow.hpp"

static cl_platform_id platform_id = NULL;
static cl_device_id device_id = NULL;
static cl_uint ret_num_devices;
static cl_uint ret_num_platforms;
static cl_context context;

static char* source_str;
static size_t source_size;

static cl_program program;
static cl_kernel kernel;
static cl_command_queue command_queue;

// OpenCL data exchange buffers.
static cl_mem cl_pow_input, cl_digest, pow_buff, cl_start_nonce, cl_end_nonce,
              cl_zero_bits_num, cl_is_found, cl_attempts_num;

static size_t global_work_size = 1;
static size_t local_work_size = 1;

// Check OpenCL call success.
#define CHECK_CL(rc) do { \
  if (rc != CL_SUCCESS) \
    errx(-1, "OpenCL call failed at line %i, error code: %i\n", __LINE__, rc); \
  } while (0)


// Load OpenCL kernel source code.
void sha256_pow_load_source() {

  FILE *fp = fopen("sha256_pow.cl", "r");
  if (!fp) {
    fprintf(stderr, "Failed to load kernel.\n");
    exit(1);
  }
  source_str = (char*)malloc(MAX_SOURCE_SIZE);
  source_size = fread( source_str, 1, MAX_SOURCE_SIZE, fp);
  fclose(fp);
}

// Initialize OpenCL device.
void sha256_pow_create_device() {
  CHECK_CL(clGetPlatformIDs(1, &platform_id, &ret_num_platforms));
  CHECK_CL(clGetDeviceIDs( platform_id, CL_DEVICE_TYPE_ALL, 1, &device_id, &ret_num_devices));
  cl_int rc;
  context = clCreateContext( NULL, 1, &device_id, NULL, NULL, &rc);
  CHECK_CL(rc);
}

// Initialize OpenCL kernel.
void sha256_pow_create_kernel() {

  cl_int rc;
  program = clCreateProgramWithSource(context, 1, (const char **)&source_str, (const size_t *)&source_size, &rc);
  CHECK_CL(rc);

  rc = clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);
  if (rc == CL_BUILD_PROGRAM_FAILURE) {
    size_t log_size;
    clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, 0,
                          NULL, &log_size);
    char *log = (char *)malloc(log_size);
    clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, log_size,
                          log, NULL);
    printf("Error compiling kernel:\n%s\n", log);
  }
  else
    CHECK_CL(rc);

  kernel = clCreateKernel(program, "sha256_pow_kernel", &rc);
  CHECK_CL(rc);
  command_queue = clCreateCommandQueue(context, device_id, 0, &rc);
  CHECK_CL(rc);
}

// Initialize OpenCL buffers and kernel arguments.
void sha256_pow_create_cl_obj() {

  cl_int rc;
  cl_pow_input = clCreateBuffer(context, CL_MEM_READ_ONLY, POW_INPUT_SIZE, NULL, &rc);
  CHECK_CL(rc);
  cl_start_nonce = clCreateBuffer(context, CL_MEM_READ_ONLY, POW_NONCE_SIZE, NULL, &rc);
  CHECK_CL(rc);
  cl_zero_bits_num = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(unsigned int) * 3, NULL, &rc);
  CHECK_CL(rc);
  cl_end_nonce = clCreateBuffer(context, CL_MEM_WRITE_ONLY, POW_NONCE_SIZE, NULL, &rc);
  CHECK_CL(rc);
  cl_is_found = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(int), NULL, &rc);
  CHECK_CL(rc);
  cl_digest = clCreateBuffer(context, CL_MEM_WRITE_ONLY, SHA256_DIGEST_SIZE, NULL, &rc);
  CHECK_CL(rc);
  cl_attempts_num = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(int), NULL, &rc);
  CHECK_CL(rc);

  clSetKernelArg(kernel, 0, sizeof(cl_pow_input), (void *) &cl_pow_input);
  clSetKernelArg(kernel, 1, sizeof(cl_zero_bits_num), (void *) &cl_zero_bits_num);
  clSetKernelArg(kernel, 2, sizeof(cl_start_nonce), (void *) &cl_start_nonce);
  clSetKernelArg(kernel, 3, sizeof(cl_digest), (void *) &cl_digest);
  clSetKernelArg(kernel, 4, sizeof(cl_end_nonce), (void *) &cl_end_nonce);
  clSetKernelArg(kernel, 5, sizeof(cl_is_found), (void *) &cl_is_found);
  clSetKernelArg(kernel, 6, sizeof(cl_attempts_num), (void *) &cl_attempts_num);
}

// Print a hex string prefixed by a message.
void print_hex(const char* msg, const void* mem, int size) {
  printf("%s ", msg);
  for (int i = 0; i < size; i++)
    printf("%02x", ((unsigned char*)mem)[i]);
  printf("\n");
}

// Return current timestamp in milliseconds.
uint64_t get_timestamp_ms() {

  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000;
}

// Execute PoW, according to the arguments.
void runKernel(const char* pow_input, int zero_bits_num, const char* start_nonce,
                int* is_found, char* end_nonce, int* attempts_num,
                uint32_t* digest) {

  global_work_size = 1;

  CHECK_CL(clEnqueueWriteBuffer(command_queue, cl_zero_bits_num, CL_TRUE, 0,
                                sizeof(int), &zero_bits_num, 0, NULL, NULL));
  CHECK_CL(clEnqueueWriteBuffer(command_queue, cl_pow_input, CL_TRUE, 0,
                                POW_INPUT_SIZE, pow_input, 0, NULL, NULL));
  CHECK_CL(clEnqueueWriteBuffer(command_queue, cl_start_nonce, CL_TRUE, 0,
                                POW_NONCE_SIZE, start_nonce, 0, NULL, NULL));

  CHECK_CL(clEnqueueNDRangeKernel(
    command_queue, kernel, 1, NULL, &global_work_size, &local_work_size, 0,
    NULL, NULL
  ));
  CHECK_CL(clFinish(command_queue));

  CHECK_CL(clEnqueueReadBuffer(command_queue, cl_digest, CL_TRUE, 0,
                               SHA256_DIGEST_SIZE, digest, 0, NULL, NULL));
  CHECK_CL(clEnqueueReadBuffer(command_queue, cl_end_nonce, CL_TRUE, 0,
                               POW_NONCE_SIZE, end_nonce, 0, NULL, NULL));
  CHECK_CL(clEnqueueReadBuffer(command_queue, cl_is_found, CL_TRUE, 0,
                               sizeof(*is_found), is_found, 0, NULL, NULL));
  CHECK_CL(clEnqueueReadBuffer(command_queue, cl_attempts_num, CL_TRUE, 0,
                               sizeof(*attempts_num), attempts_num, 0, NULL, NULL));
}

void initKernel() {
  sha256_pow_load_source();
  sha256_pow_create_device();
  sha256_pow_create_kernel();
  sha256_pow_create_cl_obj();
}
#endif