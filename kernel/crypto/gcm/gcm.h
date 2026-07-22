#ifndef KERNEL_GCM_H
#define KERNEL_GCM_H

#include <stdint.h>
#include <stddef.h>
#include "kernel/crypto/aes/aes.h"

int gcm_encrypt(const AES_CTX* aes,
                const uint8_t iv[12],
                const uint8_t* aad, size_t aad_len,
                const uint8_t* plain, size_t plain_len,
                uint8_t* cipher, uint8_t tag[16]);

int gcm_decrypt(const AES_CTX* aes,
                const uint8_t iv[12],
                const uint8_t* aad, size_t aad_len,
                const uint8_t* cipher, size_t cipher_len,
                const uint8_t tag[16],
                uint8_t* plain);

#endif // KERNEL_GCM_H
