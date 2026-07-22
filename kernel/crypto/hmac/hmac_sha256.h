#ifndef KERNEL_HMAC_SHA256_H
#define KERNEL_HMAC_SHA256_H

#include <stdint.h>
#include <stddef.h>

void hmac_sha256(const uint8_t* key, size_t key_len, const uint8_t* data, size_t data_len, uint8_t mac[32]);

#endif // KERNEL_HMAC_SHA256_H
