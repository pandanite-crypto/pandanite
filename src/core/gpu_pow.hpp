#define SHA256_DIGEST_SIZE 32
#define POW_MAX_ITERATIONS 10
#define POW_INPUT_SIZE 32
#define POW_NONCE_SIZE 32
#define POW_BUFF_SIZE (POW_INPUT_SIZE + POW_NONCE_SIZE)
#define MAX_SOURCE_SIZE 10000000

typedef struct {
  int zero_bits_num;
  char start_nonce[POW_NONCE_SIZE];
  uint digest[SHA256_DIGEST_SIZE];
  char end_nonce[POW_NONCE_SIZE];
  int is_found;
  int attempts_num;
} sha256_pow_result;

void initKernel();
bool runKernel(const char* pow_input, int zero_bits_num, const char* start_nonce, char* end_nonce, uint64_t& numAttempts);
