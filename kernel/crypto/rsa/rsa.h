#ifndef KERNEL_RSA_H
#define KERNEL_RSA_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define RSA_MAX_KEY_BITS  4096
#define RSA_MAX_KEY_BYTES (RSA_MAX_KEY_BITS / 8)

typedef struct {
    uint8_t  modulus[RSA_MAX_KEY_BYTES];
    size_t   modulus_len;
    uint8_t  exponent[8];
    size_t   exponent_len;
    uint32_t e_val;
} RsaPublicKey;

bool rsa_pkcs1_v15_verify(const uint8_t* msg, size_t msg_len,
                         const uint8_t* sig, size_t sig_len,
                         const RsaPublicKey* pubkey);

#endif // KERNEL_RSA_H
