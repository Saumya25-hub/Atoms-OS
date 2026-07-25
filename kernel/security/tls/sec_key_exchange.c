/*
 * BOS OS — Phase 3: Production TLS & Security Engine
 * sec_key_exchange.c — Key Exchange & TLS PRF Master Secret Derivation
 */

#include "kernel/security/tls/sec_tls.h"
#include "kernel/security/include/bos_crypto.h"
#include "kernel/security/include/bos_hash.h"
#include "kernel/core/lib/include/string.h"

/* TLS 1.2 PRF using HMAC-SHA256 */
static void tls_prf_sha256(const uint8_t* secret, size_t secret_len,
                           const char* label,
                           const uint8_t* seed1, size_t seed1_len,
                           const uint8_t* seed2, size_t seed2_len,
                           uint8_t* out, size_t out_len) {
    size_t label_len = strlen(label);
    uint8_t seed[128];
    memcpy(seed, label, label_len);
    if (seed1 && seed1_len > 0) memcpy(seed + label_len, seed1, seed1_len);
    if (seed2 && seed2_len > 0) memcpy(seed + label_len + seed1_len, seed2, seed2_len);
    size_t full_seed_len = label_len + seed1_len + seed2_len;

    /* P_hash(secret, seed) */
    uint8_t a[32];
    bos_hmac_sha256(secret, secret_len, seed, full_seed_len, a);

    size_t offset = 0;
    while (offset < out_len) {
        uint8_t hmac_input[32 + 128];
        memcpy(hmac_input, a, 32);
        memcpy(hmac_input + 32, seed, full_seed_len);

        uint8_t output_block[32];
        bos_hmac_sha256(secret, secret_len, hmac_input, 32 + full_seed_len, output_block);

        size_t todo = (out_len - offset < 32) ? (out_len - offset) : 32;
        memcpy(out + offset, output_block, todo);
        offset += todo;

        /* A(i) = HMAC(secret, A(i-1)) */
        uint8_t next_a[32];
        bos_hmac_sha256(secret, secret_len, a, 32, next_a);
        memcpy(a, next_a, 32);
    }
}

bos_sec_status_t sec_tls_derive_master_secret(const uint8_t pre_master_secret[48],
                                               const uint8_t client_random[32],
                                               const uint8_t server_random[32],
                                               uint8_t master_secret[48]) {
    tls_prf_sha256(pre_master_secret, 48, "master secret", client_random, 32, server_random, 32, master_secret, 48);
    return BOS_SEC_OK;
}

bos_sec_status_t sec_tls_generate_keys(const uint8_t master_secret[48],
                                        const uint8_t client_random[32],
                                        const uint8_t server_random[32],
                                        uint8_t* client_write_key,
                                        uint8_t* server_write_key,
                                        uint8_t* client_write_iv,
                                        uint8_t* server_write_iv) {
    uint8_t key_block[128];
    tls_prf_sha256(master_secret, 48, "key expansion", server_random, 32, client_random, 32, key_block, 128);

    /* Allocate MAC keys, Encryption keys, IVs */
    size_t mac_len = 32;
    size_t enc_len = 16;
    size_t iv_len = 4;

    size_t ptr = 0;
    ptr += mac_len * 2; /* Skip client & server MAC keys for AEAD/GCM */
    memcpy(client_write_key, key_block + ptr, enc_len); ptr += enc_len;
    memcpy(server_write_key, key_block + ptr, enc_len); ptr += enc_len;
    memcpy(client_write_iv, key_block + ptr, iv_len); ptr += iv_len;
    memcpy(server_write_iv, key_block + ptr, iv_len);

    return BOS_SEC_OK;
}
