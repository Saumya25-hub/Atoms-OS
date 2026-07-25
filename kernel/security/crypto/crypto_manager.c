/*
 * BOS OS — Phase 3: Production TLS & Security Engine
 * crypto_manager.c — Cryptographic Subsystem Orchestrator & HMAC / HKDF Implementation
 */

#include "kernel/security/crypto/crypto_manager.h"
#include "kernel/security/include/bos_hash.h"
#include "kernel/core/lib/include/string.h"

void bos_crypto_zeroize(void* v, size_t n) {
    if (!v || n == 0) return;
    volatile uint8_t* p = (volatile uint8_t*)v;
    while (n--) {
        *p++ = 0;
    }
}

bos_sec_status_t bos_hmac_sha256(const uint8_t* key, size_t key_len,
                                 const uint8_t* msg, size_t msg_len,
                                 uint8_t out[32]) {
    if (!out) return BOS_SEC_ERR_NULL_POINTER;
    if (key_len > 0 && !key) return BOS_SEC_ERR_NULL_POINTER;
    if (msg_len > 0 && !msg) return BOS_SEC_ERR_NULL_POINTER;

    uint8_t k0[64] = {0};
    if (key_len > 64) {
        bos_sha256(key, key_len, k0);
    } else {
        memcpy(k0, key, key_len);
    }

    uint8_t ipad[64], opad[64];
    for (int i = 0; i < 64; i++) {
        ipad[i] = k0[i] ^ 0x36;
        opad[i] = k0[i] ^ 0x5c;
    }

    /* Inner hash = SHA256(ipad || msg) */
    bos_sha256_ctx_t inner_ctx;
    uint8_t inner_hash[32];
    bos_sha256_init(&inner_ctx);
    bos_sha256_update(&inner_ctx, ipad, 64);
    bos_sha256_update(&inner_ctx, msg, msg_len);
    bos_sha256_final(&inner_ctx, inner_hash);

    /* Outer hash = SHA256(opad || inner_hash) */
    bos_sha256_ctx_t outer_ctx;
    bos_sha256_init(&outer_ctx);
    bos_sha256_update(&outer_ctx, opad, 64);
    bos_sha256_update(&outer_ctx, inner_hash, 32);
    bos_sha256_final(&outer_ctx, out);

    bos_crypto_zeroize(k0, sizeof(k0));
    bos_crypto_zeroize(ipad, sizeof(ipad));
    bos_crypto_zeroize(opad, sizeof(opad));
    return BOS_SEC_OK;
}

bos_sec_status_t bos_hmac_sha384(const uint8_t* key, size_t key_len,
                                 const uint8_t* msg, size_t msg_len,
                                 uint8_t out[48]) {
    if (!out) return BOS_SEC_ERR_NULL_POINTER;
    uint8_t k0[128] = {0};
    if (key_len > 128) {
        bos_sha384(key, key_len, k0);
    } else {
        memcpy(k0, key, key_len);
    }

    uint8_t ipad[128], opad[128];
    for (int i = 0; i < 128; i++) {
        ipad[i] = k0[i] ^ 0x36;
        opad[i] = k0[i] ^ 0x5c;
    }

    bos_sha384_ctx_t inner_ctx;
    uint8_t inner_hash[48];
    bos_sha384_init(&inner_ctx);
    bos_sha384_update(&inner_ctx, ipad, 128);
    bos_sha384_update(&inner_ctx, msg, msg_len);
    bos_sha384_final(&inner_ctx, inner_hash);

    bos_sha384_ctx_t outer_ctx;
    bos_sha384_init(&outer_ctx);
    bos_sha384_update(&outer_ctx, opad, 128);
    bos_sha384_update(&outer_ctx, inner_hash, 48);
    bos_sha384_final(&outer_ctx, out);

    bos_crypto_zeroize(k0, sizeof(k0));
    return BOS_SEC_OK;
}

bos_sec_status_t bos_hmac_sha512(const uint8_t* key, size_t key_len,
                                 const uint8_t* msg, size_t msg_len,
                                 uint8_t out[64]) {
    if (!out) return BOS_SEC_ERR_NULL_POINTER;
    uint8_t k0[128] = {0};
    if (key_len > 128) {
        bos_sha512(key, key_len, k0);
    } else {
        memcpy(k0, key, key_len);
    }

    uint8_t ipad[128], opad[128];
    for (int i = 0; i < 128; i++) {
        ipad[i] = k0[i] ^ 0x36;
        opad[i] = k0[i] ^ 0x5c;
    }

    bos_sha512_ctx_t inner_ctx;
    uint8_t inner_hash[64];
    bos_sha512_init(&inner_ctx);
    bos_sha512_update(&inner_ctx, ipad, 128);
    bos_sha512_update(&inner_ctx, msg, msg_len);
    bos_sha512_final(&inner_ctx, inner_hash);

    bos_sha512_ctx_t outer_ctx;
    bos_sha512_init(&outer_ctx);
    bos_sha512_update(&outer_ctx, opad, 128);
    bos_sha512_update(&outer_ctx, inner_hash, 64);
    bos_sha512_final(&outer_ctx, out);

    bos_crypto_zeroize(k0, sizeof(k0));
    return BOS_SEC_OK;
}

bos_sec_status_t bos_hkdf_sha256(const uint8_t* salt, size_t salt_len,
                                 const uint8_t* ikm, size_t ikm_len,
                                 const uint8_t* info, size_t info_len,
                                 uint8_t* okm, size_t okm_len) {
    if (!okm || okm_len == 0) return BOS_SEC_ERR_INVALID_PARAM;

    /* HKDF-Extract: PRK = HMAC-Hash(salt, IKM) */
    uint8_t prk[32];
    uint8_t default_salt[32] = {0};
    const uint8_t* s = salt ? salt : default_salt;
    size_t s_len = salt ? salt_len : 32;

    bos_hmac_sha256(s, s_len, ikm, ikm_len, prk);

    /* HKDF-Expand: N = ceil(L/HashLen) */
    uint8_t t[32 + 256 + 1];
    uint8_t T_block[32];
    size_t T_len = 0;
    size_t offset = 0;
    uint8_t counter = 1;

    while (offset < okm_len) {
        size_t t_input_len = 0;
        if (counter > 1) {
            memcpy(t, T_block, 32);
            t_input_len += 32;
        }
        if (info && info_len > 0) {
            memcpy(t + t_input_len, info, info_len);
            t_input_len += info_len;
        }
        t[t_input_len++] = counter;

        bos_hmac_sha256(prk, 32, t, t_input_len, T_block);

        size_t todo = (okm_len - offset < 32) ? (okm_len - offset) : 32;
        memcpy(okm + offset, T_block, todo);
        offset += todo;
        counter++;
    }

    bos_crypto_zeroize(prk, sizeof(prk));
    return BOS_SEC_OK;
}

bos_sec_status_t crypto_manager_init(void) {
    return BOS_SEC_OK;
}
