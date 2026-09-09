/*
 * ATOMS Platform Adaptation Layer (APAL)
 * Randomness & Entropy Adapter (Chromium base::RandBytes)
 */

#ifndef ATOMS_APAL_RAND_H
#define ATOMS_APAL_RAND_H

#include "../include/apal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Fills buffer with cryptographically secure random bytes */
apal_status_t apal_rand_bytes(void *output, size_t count);

/* Returns a 64-bit unsigned random integer */
uint64_t apal_rand_uint64(void);

#ifdef __cplusplus
}
#endif

#endif /* ATOMS_APAL_RAND_H */
