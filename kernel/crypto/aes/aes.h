#ifndef KERNEL_AES_H
#define KERNEL_AES_H

#include <stdint.h>
#include <stddef.h>

typedef struct {
    uint32_t round_keys[60];
    int rounds;
} AES_CTX;

int aes_set_key(AES_CTX* ctx, const uint8_t* key, size_t key_len);
void aes_encrypt_block(const AES_CTX* ctx, const uint8_t in[16], uint8_t out[16]);

#endif // KERNEL_AES_H
