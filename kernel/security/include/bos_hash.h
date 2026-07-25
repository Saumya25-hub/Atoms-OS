/*
 * BOS OS — Phase 3: Production TLS & Security Engine
 * bos_hash.h — Hash Engine Public API (SHA-256, SHA-384, SHA-512)
 */

#ifndef BOS_HASH_H
#define BOS_HASH_H

#include "kernel/security/include/bos_security_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* SHA-256 Context Structure */
typedef struct {
    uint32_t state[8];
    uint64_t count;
    uint8_t  buffer[64];
} bos_sha256_ctx_t;

/* SHA-384 / SHA-512 Context Structure */
typedef struct {
    uint64_t state[8];
    uint64_t count[2];
    uint8_t  buffer[128];
} bos_sha512_ctx_t;

typedef bos_sha512_ctx_t bos_sha384_ctx_t;

/* Standard One-Shot Hash Functions */
bos_sec_status_t bos_sha256(const void* data, size_t len, uint8_t hash[32]);
bos_sec_status_t bos_sha384(const void* data, size_t len, uint8_t hash[48]);
bos_sec_status_t bos_sha512(const void* data, size_t len, uint8_t hash[64]);

/* Streaming SHA-256 API */
void bos_sha256_init(bos_sha256_ctx_t* ctx);
void bos_sha256_update(bos_sha256_ctx_t* ctx, const void* data, size_t len);
void bos_sha256_final(bos_sha256_ctx_t* ctx, uint8_t hash[32]);

/* Streaming SHA-384 API */
void bos_sha384_init(bos_sha384_ctx_t* ctx);
void bos_sha384_update(bos_sha384_ctx_t* ctx, const void* data, size_t len);
void bos_sha384_final(bos_sha384_ctx_t* ctx, uint8_t hash[48]);

/* Streaming SHA-512 API */
void bos_sha512_init(bos_sha512_ctx_t* ctx);
void bos_sha512_update(bos_sha512_ctx_t* ctx, const void* data, size_t len);
void bos_sha512_final(bos_sha512_ctx_t* ctx, uint8_t hash[64]);

#ifdef __cplusplus
}
#endif

#endif /* BOS_HASH_H */
