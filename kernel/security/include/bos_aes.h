/*
 * BOS OS — Phase 3: Production TLS & Security Engine
 * bos_aes.h — AES Encryption Engine Public API (AES-128 & AES-256 in CBC, CTR, GCM)
 */

#ifndef BOS_AES_H
#define BOS_AES_H

#include "kernel/security/include/bos_security_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief AES Encryption (CBC / CTR / ECB).
 * @param key Key bytes (16 bytes for 128-bit, 32 bytes for 256-bit).
 * @param key_bits 128 or 256.
 * @param iv Initialization vector (16 bytes for CBC/CTR; NULL for ECB).
 * @param plain Input plaintext buffer.
 * @param len Plaintext byte length (must be multiple of 16 for CBC/ECB, arbitrary for CTR).
 * @param cipher Output ciphertext buffer.
 * @param mode Mode enum (BOS_AES_MODE_CBC, BOS_AES_MODE_CTR, BOS_AES_MODE_ECB).
 */
bos_sec_status_t bos_aes_encrypt(const uint8_t* key, size_t key_bits,
                                 const uint8_t* iv,
                                 const uint8_t* plain, size_t len,
                                 uint8_t* cipher, uint8_t mode);

/**
 * @brief AES Decryption (CBC / CTR / ECB).
 */
bos_sec_status_t bos_aes_decrypt(const uint8_t* key, size_t key_bits,
                                 const uint8_t* iv,
                                 const uint8_t* cipher, size_t len,
                                 uint8_t* plain, uint8_t mode);

/**
 * @brief AES-GCM Encrypt & Tag generation (Authenticated Encryption).
 */
bos_sec_status_t bos_aes_gcm_encrypt(const uint8_t* key, size_t key_bits,
                                     const uint8_t* iv, size_t iv_len,
                                     const uint8_t* aad, size_t aad_len,
                                     const uint8_t* plain, size_t plain_len,
                                     uint8_t* cipher,
                                     uint8_t tag[16]);

/**
 * @brief AES-GCM Decrypt & Tag verification.
 */
bos_sec_status_t bos_aes_gcm_decrypt(const uint8_t* key, size_t key_bits,
                                     const uint8_t* iv, size_t iv_len,
                                     const uint8_t* aad, size_t aad_len,
                                     const uint8_t* cipher, size_t cipher_len,
                                     const uint8_t tag[16],
                                     uint8_t* plain);

#ifdef __cplusplus
}
#endif

#endif /* BOS_AES_H */
