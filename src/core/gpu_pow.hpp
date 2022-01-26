#ifdef GPU_ENABLED
#define SHA256_DIGEST_SIZE 32
#define POW_MAX_ITERATIONS 1000000

#define POW_INPUT_SIZE 32
#define POW_NONCE_SIZE 32
#define POW_BUFF_SIZE (POW_INPUT_SIZE + POW_NONCE_SIZE)
#define MAX_SOURCE_SIZE 10000000

void initKernel();
void runKernel(const char* pow_input, int zero_bits_num, const char* start_nonce,
                int* is_found, char* end_nonce, int* attempts_num,
                uint32_t* digest);
#endif