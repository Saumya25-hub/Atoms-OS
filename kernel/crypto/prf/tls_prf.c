#include "tls_prf.h"
#include "kernel/crypto/hmac/hmac_sha256.h"
#include "kernel/core/lib/include/string.h"

void tls12_prf(const uint8_t* secret, size_t secret_len,
               const char* label,
               const uint8_t* seed, size_t seed_len,
               uint8_t* out, size_t out_len) {
    if (!out || out_len == 0) return;

    size_t label_len = strlen(label);
    size_t total_seed_len = label_len + seed_len;
    uint8_t total_seed[256];

    if (total_seed_len <= sizeof(total_seed)) {
        memcpy(total_seed, label, label_len);
        if (seed && seed_len > 0) {
            memcpy(total_seed + label_len, seed, seed_len);
        }
    } else {
        return;
    }

    uint8_t A[32];
    hmac_sha256(secret, secret_len, total_seed, total_seed_len, A);

    size_t offset = 0;
    while (offset < out_len) {
        uint8_t block_data[300];
        memcpy(block_data, A, 32);
        memcpy(block_data + 32, total_seed, total_seed_len);

        uint8_t hmac_out[32];
        hmac_sha256(secret, secret_len, block_data, 32 + total_seed_len, hmac_out);

        size_t copy_bytes = (out_len - offset < 32) ? (out_len - offset) : 32;
        memcpy(out + offset, hmac_out, copy_bytes);
        offset += copy_bytes;

        if (offset < out_len) {
            uint8_t next_A[32];
            hmac_sha256(secret, secret_len, A, 32, next_A);
            memcpy(A, next_A, 32);
        }
    }
}
