#ifndef KERNEL_TLS_PRF_H
#define KERNEL_TLS_PRF_H

#include <stdint.h>
#include <stddef.h>

void tls12_prf(const uint8_t* secret, size_t secret_len,
               const char* label,
               const uint8_t* seed, size_t seed_len,
               uint8_t* out, size_t out_len);

#endif // KERNEL_TLS_PRF_H
