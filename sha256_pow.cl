#include "sha256_pow.h"

inline uint rotr(uint x, int n) {
  if (n < 32) return (x >> n) | (x << (32 - n));
  return x;
}

inline uint ch(uint x, uint y, uint z) {
  return (x & y) ^ (~x & z);
}

inline uint maj(uint x, uint y, uint z) {
  return (x & y) ^ (x & z) ^ (y & z);
}

inline uint sigma0(uint x) {
  return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22);
}

inline uint sigma1(uint x) {
  return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25);
}

inline uint gamma0(uint x) {
  return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3);
}

inline uint gamma1(uint x) {
  return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10);
}

void sha256_crypt_subkernel(char* input, uint *digest) {

  const uint K[64]={
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1,
    0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786,
    0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147,
    0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b,
    0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a,
    0x5b9cca4f, 0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
  };

  digest[0] = 0x6a09e667;
  digest[1] = 0xbb67ae85;
  digest[2] = 0x3c6ef372;
  digest[3] = 0xa54ff53a;
  digest[4] = 0x510e527f;
  digest[5] = 0x9b05688c;
  digest[6] = 0x1f83d9ab;
  digest[7] = 0x5be0cd19;

  uint W[80], A,B,C,D,E,F,G,H,T1,T2;

  for (int item = 0; item < 2; item++) {

      A = digest[0];
      B = digest[1];
      C = digest[2];
      D = digest[3];
      E = digest[4];
      F = digest[5];
      G = digest[6];
      H = digest[7];

      int t;
      for (t = 0; t < 80; t++)
        W[t] = 0x00000000;

      int msg_pad = item * 64;
      int current_pad;

      if (POW_BUFF_SIZE > msg_pad)
        current_pad = (POW_BUFF_SIZE-msg_pad)>64?64:(POW_BUFF_SIZE-msg_pad);
      else
        current_pad =-1;

      if (current_pad > 0 ) {
          int i = current_pad;
          int stop =  i/4;

          for (t = 0 ; t < stop ; t++){
            W[t] = ((uchar)  input[msg_pad + t * 4]) << 24;
            W[t] |= ((uchar) input[msg_pad + t * 4 + 1]) << 16;
            W[t] |= ((uchar) input[msg_pad + t * 4 + 2]) << 8;
            W[t] |= (uchar)  input[msg_pad + t * 4 + 3];
          }

          int mmod = i % 4;
          if (mmod == 3) {
            W[t] = ((uchar)  input[msg_pad + t * 4]) << 24;
            W[t] |= ((uchar) input[msg_pad + t * 4 + 1]) << 16;
            W[t] |= ((uchar) input[msg_pad + t * 4 + 2]) << 8;
            W[t] |=  ((uchar) 0x80) ;
          }
          else if (mmod == 2) {
            W[t] = ((uchar)  input[msg_pad + t * 4]) << 24;
            W[t] |= ((uchar) input[msg_pad + t * 4 + 1]) << 16;
            W[t] |=  0x8000 ;
          }
          else if (mmod == 1) {
            W[t] = ((uchar)  input[msg_pad + t * 4]) << 24;
            W[t] |=  0x800000 ;
          }
          else /*if (mmod == 0)*/ {
            W[t] =  0x80000000 ;
          }
      }

      else if (current_pad < 0) {
        W[0]=0x80000000;
        W[15] = POW_BUFF_SIZE * 8;
      }

      for (t = 0; t < 64; t++) {
        if (t >= 16)
          W[t] = gamma1(W[t - 2]) + W[t - 7] + gamma0(W[t - 15]) + W[t - 16];

        T1 = H + sigma1(E) + ch(E, F, G) + K[t] + W[t];
        T2 = sigma0(A) + maj(A, B, C);
        H = G; G = F; F = E; E = D + T1; D = C; C = B; B = A; A = T1 + T2;
      }

      digest[0] += A;
      digest[1] += B;
      digest[2] += C;
      digest[3] += D;
      digest[4] += E;
      digest[5] += F;
      digest[6] += G;
      digest[7] += H;
    }

  // Convert result to big-endian.
  for (int i = 0; i < 8; i++) {
    digest[i] = (digest[i] << 24)
                | ((digest[i] <<  8) & 0x00ff0000)
                | ((digest[i] >>  8) & 0x0000ff00)
                | ((digest[i] >> 24) & 0x000000ff);
  }
}

// Print hex data prefixed with a number (OpenCL won't allow const char* arg).
void print_hex(int num, void* mem, int size) {
  printf("%i: ", num);
  for (int i = 0; i < size; i++)
    printf("%02x", ((unsigned char*)mem)[i]);
  printf("\n");
}

// PoW nonce-finding kernel.
__kernel void sha256_pow_kernel(__global char* input,
                                __global int* zero_bits_num,
                                __global char* start_nonce,
                                __global uint* digest,
                                __global char* end_nonce,
                                __global int* is_found,
                                __global int* attempts_num) {
  *is_found = 0;

  __uint128_t zero_bits_check_mask = ~0;
  zero_bits_check_mask <<= *zero_bits_num;

  __uint128_t local_input[4];
  char* local_input_bytes = (char*)&local_input;

  for (int i = 0; i < POW_INPUT_SIZE; i++)
    local_input_bytes[i] = input[i];

  for (int i = 0; i < POW_NONCE_SIZE; i++)
    local_input_bytes[POW_INPUT_SIZE + i] = start_nonce[i];

  __uint128_t local_digest[2];

  for (*attempts_num = 1; *attempts_num < POW_MAX_ITERATIONS; ++*attempts_num) {
#ifdef DEBUG_POW
    print_hex(0, local_input, sizeof(local_input));
#endif

    sha256_crypt_subkernel((char*)local_input, (uint*)local_digest);

#ifdef DEBUG_POW
    print_hex(1, local_digest, sizeof(local_digest));
#endif

    if ((local_digest[0] | zero_bits_check_mask) == zero_bits_check_mask) {
      *is_found = 1;
      break;
    }

    // Increment the nonce. If first half-word is 0, it wrapped around.
    ++local_input[2] || ++local_input[3];
  }

  uint* local_digest_words = (uint*)local_digest;
  for (int i = 0; i < 8; i++)
    digest[i] = local_digest_words[i];

  for (int i = 0; i < POW_NONCE_SIZE; i++)
    end_nonce[i] = local_input_bytes[POW_INPUT_SIZE + i];
}