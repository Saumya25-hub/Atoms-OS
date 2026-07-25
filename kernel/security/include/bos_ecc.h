/*
 * BOS OS — Phase 3: Production TLS & Security Engine
 * bos_ecc.h — Elliptic Curve Cryptography (ECC secp256r1 / P-256) API
 */

#ifndef BOS_ECC_H
#define BOS_ECC_H

#include "kernel/security/include/bos_security_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Generates an ECDSA signature over SHA-256 hash using ECC P-256 private key.
 */
bos_sec_status_t bos_ecc_sign(const bos_ecc_key_t* key,
                              const uint8_t hash[32],
                              uint8_t r[32], uint8_t s[32]);

/**
 * @brief Verifies an ECDSA signature against a SHA-256 hash using ECC P-256 public key.
 */
bos_sec_status_t bos_ecc_verify(const bos_ecc_key_t* key,
                                const uint8_t hash[32],
                                const uint8_t r[32], const uint8_t s[32]);

/**
 * @brief Computes ECDHE shared secret between local private key and remote public key.
 */
bos_sec_status_t bos_ecc_compute_shared_secret(const bos_ecc_key_t* priv_key,
                                                const bos_ecc_key_t* pub_key,
                                                uint8_t secret[32]);

/**
 * @brief Generates a ephemeral ECC P-256 key pair.
 */
bos_sec_status_t bos_ecc_generate_keypair(bos_ecc_key_t* keypair);

#ifdef __cplusplus
}
#endif

#endif /* BOS_ECC_H */
