#ifndef KERNEL_CRYPTO_RAND_H
#define KERNEL_CRYPTO_RAND_H

#include <stdint.h>
#include <stddef.h>

void crypto_random_bytes(uint8_t* buf, size_t len);

#endif // KERNEL_CRYPTO_RAND_H
