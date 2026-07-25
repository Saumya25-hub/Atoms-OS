/*
 * BOS OS — Phase 3: Production TLS & Security Engine
 * bos_rsa.h — RSA Cryptographic Engine API (PKCS#1 v1.5 / PSS Sign, Verify, Encrypt, Decrypt)
 */

#ifndef BOS_RSA_H
#define BOS_RSA_H

#include "kernel/security/include/bos_security_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Signs a hash digest using an RSA private key (PKCS#1 v1.5).
 */
bos_sec_status_t bos_rsa_sign(const bos_rsa_key_t* key,
                              const uint8_t hash[32],
                              uint8_t* signature,
                              size_t* sig_len);

/**
 * @brief Verifies an RSA signature against a hash digest using an RSA public key.
 */
bos_sec_status_t bos_rsa_verify(const bos_rsa_key_t* key,
                                const uint8_t hash[32],
                                const uint8_t* signature,
                                size_t sig_len);

/**
 * @brief Encrypts plaintext using RSA public key (PKCS#1 v1.5 padding).
 */
bos_sec_status_t bos_rsa_encrypt(const bos_rsa_key_t* key,
                                 const uint8_t* plain, size_t plain_len,
                                 uint8_t* cipher, size_t* cipher_len);

/**
 * @brief Decrypts ciphertext using RSA private key.
 */
bos_sec_status_t bos_rsa_decrypt(const bos_rsa_key_t* key,
                                 const uint8_t* cipher, size_t cipher_len,
                                 uint8_t* plain, size_t* plain_len);

#ifdef __cplusplus
}
#endif

#endif /* BOS_RSA_H */
