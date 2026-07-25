/*
 * BOS OS — Phase 3: Production TLS & Security Engine
 * sec_sha384.c — FIPS 180-4 Standard SHA-384 Implementation
 */

#include "kernel/security/hash/sec_hash.h"
#include "kernel/core/lib/include/string.h"

void bos_sha384_init(bos_sha384_ctx_t* ctx) {
    ctx->count[0] = 0;
    ctx->count[1] = 0;
    ctx->state[0] = 0xcbbb9d5dc1059ed8ULL;
    ctx->state[1] = 0x629a292a367cd507ULL;
    ctx->state[2] = 0x9159015a3070dd17ULL;
    ctx->state[3] = 0x152fecd8f70e5939ULL;
    ctx->state[4] = 0x67332667ffc00b31ULL;
    ctx->state[5] = 0x8eb44a8768581511ULL;
    ctx->state[6] = 0xdb0c2e0d64f98fa7ULL;
    ctx->state[7] = 0x47b5481dbefa4fa4ULL;
}

void bos_sha384_update(bos_sha384_ctx_t* ctx, const void* data, size_t len) {
    bos_sha512_update((bos_sha512_ctx_t*)ctx, data, len);
}

void bos_sha384_final(bos_sha384_ctx_t* ctx, uint8_t hash[48]) {
    uint8_t full_hash[64];
    bos_sha512_final((bos_sha512_ctx_t*)ctx, full_hash);
    memcpy(hash, full_hash, 48);
}

bos_sec_status_t bos_sha384(const void* data, size_t len, uint8_t hash[48]) {
    if (!hash) return BOS_SEC_ERR_NULL_POINTER;
    if (len > 0 && !data) return BOS_SEC_ERR_NULL_POINTER;
    bos_sha384_ctx_t ctx;
    bos_sha384_init(&ctx);
    bos_sha384_update(&ctx, data, len);
    bos_sha384_final(&ctx, hash);
    return BOS_SEC_OK;
}

bos_sec_status_t sec_hash_init(void) {
    return BOS_SEC_OK;
}
