/*
 * BOS OS — Phase 3: Production TLS & Security Engine
 * bos_crypto.h — Cryptographic Utilities & Key Derivation API
 */

#ifndef BOS_CRYPTO_H
#define BOS_CRYPTO_H

#include "kernel/security/include/bos_security_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Zeroizes secret buffer memory in a security-conscious manner (preventing compiler dead-code elimination).
 */
void bos_crypto_zeroize(void* v, size_t n);

/**
 * @brief Computes HMAC-SHA256 over message with key.
 */
bos_sec_status_t bos_hmac_sha256(const uint8_t* key, size_t key_len,
                                 const uint8_t* msg, size_t msg_len,
                                 uint8_t out[32]);

/**
 * @brief Computes HMAC-SHA384 over message with key.
 */
bos_sec_status_t bos_hmac_sha384(const uint8_t* key, size_t key_len,
                                 const uint8_t* msg, size_t msg_len,
                                 uint8_t out[48]);

/**
 * @brief Computes HMAC-SHA512 over message with key.
 */
bos_sec_status_t bos_hmac_sha512(const uint8_t* key, size_t key_len,
                                 const uint8_t* msg, size_t msg_len,
                                 uint8_t out[64]);

/**
 * @brief HKDF-Extract & HKDF-Expand (RFC 5869) key derivation using SHA-256.
 */
bos_sec_status_t bos_hkdf_sha256(const uint8_t* salt, size_t salt_len,
                                 const uint8_t* ikm, size_t ikm_len,
                                 const uint8_t* info, size_t info_len,
                                 uint8_t* okm, size_t okm_len);

#ifdef __cplusplus
}
#endif

#endif /* BOS_CRYPTO_H */
