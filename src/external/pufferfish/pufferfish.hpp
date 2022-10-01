#pragma once

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include "openssl/sha.h"
#include "openssl/hmac.h"

#define bin2enc_len(x) (((x) + 2) / 3 * 4)
#define chr64(c)((c) > 127 ? 255 : idx64[(c)])
#define shr(x,n) (x >> n)
#define shl(x,n) (x << n)
#define PF_ID "$PF2$"
#define PF_ID_SZ strlen(PF_ID)
#define PF_SBOX_N 4
#define PF_SALT_SZ 16
#define PF_SALTSPACE (2 + PF_ID_SZ + bin2enc_len(sizeof(pf_salt)))
#define PF_HASHSPACE (PF_SALTSPACE + bin2enc_len(PF_DIGEST_LENGTH))

#define PF_DIGEST EVP_sha512()
#define PF_DIGEST_LENGTH SHA512_DIGEST_LENGTH

typedef struct pf_salt
{
    uint8_t cost_t;
    uint8_t cost_m;
    char salt[PF_SALT_SZ];
} pf_salt;

int pf_newhash(const void *pass, const size_t pass_sz, const size_t cost_t, const size_t cost_m, char *hash);
