/*
 * BOS OS — Phase 3: Production TLS & Security Engine
 * bos_random.h — Cryptographically Secure Random Number Generator (CSPRNG) API
 */

#ifndef BOS_RANDOM_H
#define BOS_RANDOM_H

#include "kernel/security/include/bos_security_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Fills buffer with CSPRNG cryptographically secure random bytes.
 * @param buf Pointer to buffer to fill.
 * @param len Number of bytes requested.
 * @return BOS_SEC_OK on success.
 */
bos_sec_status_t bos_random_bytes(void* buf, size_t len);

/**
 * @brief Returns a 32-bit cryptographically secure random integer.
 */
uint32_t bos_random_u32(void);

/**
 * @brief Returns a 64-bit cryptographically secure random integer.
 */
uint64_t bos_random_u64(void);

/**
 * @brief Feeds additional entropy into the CSPRNG entropy pool.
 */
void bos_random_seed_entropy(const void* data, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* BOS_RANDOM_H */
