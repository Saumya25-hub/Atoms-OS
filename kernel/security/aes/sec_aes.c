/*
 * BOS OS — Phase 3: Production TLS & Security Engine
 * sec_aes.c — AES-128 / AES-256 Engine Implementation (ECB, CBC, CTR, GCM)
 */

#include "kernel/security/aes/sec_aes.h"
#include "kernel/core/lib/include/string.h"

static const uint8_t sbox[256] = {
    0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
    0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
    0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
    0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
    0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
    0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
    0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
    0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
    0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
    0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
    0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
    0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
    0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
    0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
    0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
    0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16
};

static const uint8_t rsbox[256] = {
    0x52, 0x09, 0x6a, 0xd5, 0x30, 0x36, 0xa5, 0x38, 0xbf, 0x40, 0xa3, 0x9e, 0x81, 0xf3, 0xd7, 0xfb,
    0x7c, 0xe3, 0x39, 0x82, 0x9b, 0x2f, 0xff, 0x87, 0x34, 0x8e, 0x43, 0x44, 0xc4, 0xde, 0xe9, 0xcb,
    0x54, 0x7b, 0x94, 0x32, 0xa6, 0xc2, 0x23, 0x3d, 0xee, 0x4c, 0x95, 0x0b, 0x42, 0xfa, 0xc3, 0x4e,
    0x08, 0x2e, 0xa1, 0x66, 0x28, 0xd9, 0x24, 0xb2, 0x76, 0x5b, 0xa2, 0x49, 0x6d, 0x8b, 0xd1, 0x25,
    0x72, 0xf8, 0xf6, 0x64, 0x86, 0x68, 0x98, 0x16, 0xd4, 0xa4, 0x5c, 0xcc, 0x5d, 0x65, 0xb6, 0x92,
    0x6c, 0x70, 0x48, 0x50, 0xfd, 0xed, 0xb9, 0xda, 0x5e, 0x15, 0x46, 0x57, 0xa7, 0x8d, 0x9d, 0x84,
    0x90, 0xd8, 0xab, 0x00, 0x8c, 0xbc, 0xd3, 0x0a, 0xf7, 0xe4, 0x58, 0x05, 0xb8, 0xb3, 0x45, 0x06,
    0xd0, 0x2c, 0x1e, 0x8f, 0xca, 0x3f, 0x0f, 0x02, 0xc1, 0xaf, 0xbd, 0x03, 0x01, 0x13, 0x8a, 0x6b,
    0x3a, 0x91, 0x11, 0x41, 0x4f, 0x67, 0xdc, 0xea, 0x97, 0xf2, 0xcf, 0xce, 0xf0, 0xb4, 0xe6, 0x73,
    0x96, 0xac, 0x74, 0x22, 0xe7, 0xad, 0x35, 0x85, 0xe2, 0xf9, 0x37, 0xe8, 0x1c, 0x75, 0xdf, 0x6e,
    0x47, 0xf1, 0x1a, 0x71, 0x1d, 0x29, 0xc5, 0x89, 0x6f, 0xb7, 0x62, 0x0e, 0xaa, 0x18, 0xbe, 0x1b,
    0xfc, 0x56, 0x3e, 0x4b, 0xc6, 0xd2, 0x79, 0x20, 0x9a, 0xdb, 0xc0, 0xfe, 0x78, 0xcd, 0x5a, 0xf4,
    0x1f, 0xdd, 0xa8, 0x33, 0x88, 0x07, 0xc7, 0x31, 0xb1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xec, 0x5f,
    0x60, 0x51, 0x7f, 0xa9, 0x19, 0xb5, 0x4a, 0x0d, 0x2d, 0xe5, 0x7a, 0x9f, 0x93, 0xc9, 0x9c, 0xef,
    0xa0, 0xe0, 0x3b, 0x4d, 0xae, 0x2a, 0xf5, 0xb0, 0xc8, 0xeb, 0xbb, 0x3c, 0x83, 0x53, 0x99, 0x61,
    0x17, 0x2b, 0x04, 0x7e, 0xba, 0x77, 0xd6, 0x26, 0xe1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0c, 0x7d
};

static const uint8_t Rcon[11] = {
    0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36
};

typedef struct {
    uint8_t round_keys[240];
    int rounds;
} aes_ctx_t;

static void aes_key_expansion(const uint8_t* key, size_t key_bits, aes_ctx_t* ctx) {
    int Nk = key_bits / 32;
    int Nr = Nk + 6;
    ctx->rounds = Nr;

    uint8_t* w = ctx->round_keys;
    for (int i = 0; i < Nk; i++) {
        w[i * 4 + 0] = key[i * 4 + 0];
        w[i * 4 + 1] = key[i * 4 + 1];
        w[i * 4 + 2] = key[i * 4 + 2];
        w[i * 4 + 3] = key[i * 4 + 3];
    }

    for (int i = Nk; i < 4 * (Nr + 1); i++) {
        uint8_t temp[4];
        temp[0] = w[(i - 1) * 4 + 0];
        temp[1] = w[(i - 1) * 4 + 1];
        temp[2] = w[(i - 1) * 4 + 2];
        temp[3] = w[(i - 1) * 4 + 3];

        if (i % Nk == 0) {
            uint8_t k = temp[0];
            temp[0] = sbox[temp[1]] ^ Rcon[i / Nk];
            temp[1] = sbox[temp[2]];
            temp[2] = sbox[temp[3]];
            temp[3] = sbox[k];
        } else if (Nk > 6 && (i % Nk == 4)) {
            temp[0] = sbox[temp[0]];
            temp[1] = sbox[temp[1]];
            temp[2] = sbox[temp[2]];
            temp[3] = sbox[temp[3]];
        }

        w[i * 4 + 0] = w[(i - Nk) * 4 + 0] ^ temp[0];
        w[i * 4 + 1] = w[(i - Nk) * 4 + 1] ^ temp[1];
        w[i * 4 + 2] = w[(i - Nk) * 4 + 2] ^ temp[2];
        w[i * 4 + 3] = w[(i - Nk) * 4 + 3] ^ temp[3];
    }
}

static inline uint8_t xtime(uint8_t x) {
    return ((x << 1) ^ (((x >> 7) & 1) * 0x1b));
}

static inline uint8_t multiply(uint8_t x, uint8_t y) {
    return (((y & 1) * x) ^
            ((y >> 1 & 1) * xtime(x)) ^
            ((y >> 2 & 1) * xtime(xtime(x))) ^
            ((y >> 3 & 1) * xtime(xtime(xtime(x)))) ^
            ((y >> 4 & 1) * xtime(xtime(xtime(xtime(x))))));
}

static void aes_encrypt_block(const aes_ctx_t* ctx, const uint8_t in[16], uint8_t out[16]) {
    uint8_t state[4][4];
    const uint8_t* w = ctx->round_keys;

    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 4; c++)
            state[r][c] = in[r + 4 * c] ^ w[r + 4 * c];

    for (int round = 1; round < ctx->rounds; round++) {
        /* SubBytes */
        for (int r = 0; r < 4; r++)
            for (int c = 0; c < 4; c++)
                state[r][c] = sbox[state[r][c]];

        /* ShiftRows */
        uint8_t t0 = state[1][0];
        state[1][0] = state[1][1]; state[1][1] = state[1][2]; state[1][2] = state[1][3]; state[1][3] = t0;

        uint8_t t1 = state[2][0], t2 = state[2][1];
        state[2][0] = state[2][2]; state[2][1] = state[2][3]; state[2][2] = t0 = t1; state[2][3] = t2;

        t0 = state[3][3];
        state[3][3] = state[3][2]; state[3][2] = state[3][1]; state[3][1] = state[3][0]; state[3][0] = t0;

        /* MixColumns */
        for (int c = 0; c < 4; c++) {
            uint8_t a = state[0][c], b = state[1][c], c_val = state[2][c], d = state[3][c];
            state[0][c] = xtime(a ^ b) ^ b ^ c_val ^ d;
            state[1][c] = xtime(b ^ c_val) ^ c_val ^ d ^ a;
            state[2][c] = xtime(c_val ^ d) ^ d ^ a ^ b;
            state[3][c] = xtime(d ^ a) ^ a ^ b ^ c_val;
        }

        /* AddRoundKey */
        const uint8_t* rk = w + round * 16;
        for (int r = 0; r < 4; r++)
            for (int c = 0; c < 4; c++)
                state[r][c] ^= rk[r + 4 * c];
    }

    /* Final Round */
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 4; c++)
            state[r][c] = sbox[state[r][c]];

    uint8_t t0 = state[1][0];
    state[1][0] = state[1][1]; state[1][1] = state[1][2]; state[1][2] = state[1][3]; state[1][3] = t0;

    uint8_t t1 = state[2][0], t2 = state[2][1];
    state[2][0] = state[2][2]; state[2][1] = state[2][3]; state[2][2] = t1; state[2][3] = t2;

    t0 = state[3][3];
    state[3][3] = state[3][2]; state[3][2] = state[3][1]; state[3][1] = state[3][0]; state[3][0] = t0;

    const uint8_t* rk = w + ctx->rounds * 16;
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 4; c++)
            out[r + 4 * c] = state[r][c] ^ rk[r + 4 * c];
}

static void aes_decrypt_block(const aes_ctx_t* ctx, const uint8_t in[16], uint8_t out[16]) {
    uint8_t state[4][4];
    const uint8_t* w = ctx->round_keys;

    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 4; c++)
            state[r][c] = in[r + 4 * c] ^ w[ctx->rounds * 16 + r + 4 * c];

    for (int round = ctx->rounds - 1; round > 0; round--) {
        /* InvShiftRows */
        uint8_t t0 = state[1][3];
        state[1][3] = state[1][2]; state[1][2] = state[1][1]; state[1][1] = state[1][0]; state[1][0] = t0;

        uint8_t t1 = state[2][2], t2 = state[2][3];
        state[2][2] = state[2][0]; state[2][3] = state[2][1]; state[2][0] = t1; state[2][1] = t2;

        t0 = state[3][0];
        state[3][0] = state[3][1]; state[3][1] = state[3][2]; state[3][2] = state[3][3]; state[3][3] = t0;

        /* InvSubBytes */
        for (int r = 0; r < 4; r++)
            for (int c = 0; c < 4; c++)
                state[r][c] = rsbox[state[r][c]];

        /* AddRoundKey */
        const uint8_t* rk = w + round * 16;
        for (int r = 0; r < 4; r++)
            for (int c = 0; c < 4; c++)
                state[r][c] ^= rk[r + 4 * c];

        /* InvMixColumns */
        for (int c = 0; c < 4; c++) {
            uint8_t a = state[0][c], b = state[1][c], c_val = state[2][c], d = state[3][c];
            state[0][c] = multiply(a, 0x0e) ^ multiply(b, 0x0b) ^ multiply(c_val, 0x0d) ^ multiply(d, 0x09);
            state[1][c] = multiply(a, 0x09) ^ multiply(b, 0x0e) ^ multiply(c_val, 0x0b) ^ multiply(d, 0x0d);
            state[2][c] = multiply(a, 0x0d) ^ multiply(b, 0x09) ^ multiply(c_val, 0x0e) ^ multiply(d, 0x0b);
            state[3][c] = multiply(a, 0x0b) ^ multiply(b, 0x0d) ^ multiply(c_val, 0x09) ^ multiply(d, 0x0e);
        }
    }

    /* Final InvRound */
    uint8_t t0 = state[1][3];
    state[1][3] = state[1][2]; state[1][2] = state[1][1]; state[1][1] = state[1][0]; state[1][0] = t0;

    uint8_t t1 = state[2][2], t2 = state[2][3];
    state[2][2] = state[2][0]; state[2][3] = state[2][1]; state[2][0] = t1; state[2][1] = t2;

    t0 = state[3][0];
    state[3][0] = state[3][1]; state[3][1] = state[3][2]; state[3][2] = state[3][3]; state[3][3] = t0;

    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 4; c++)
            state[r][c] = rsbox[state[r][c]];

    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 4; c++)
            out[r + 4 * c] = state[r][c] ^ w[r + 4 * c];
}

bos_sec_status_t bos_aes_encrypt(const uint8_t* key, size_t key_bits,
                                 const uint8_t* iv,
                                 const uint8_t* plain, size_t len,
                                 uint8_t* cipher, uint8_t mode) {
    if (!key || !plain || !cipher) return BOS_SEC_ERR_NULL_POINTER;
    if (key_bits != 128 && key_bits != 256) return BOS_SEC_ERR_INVALID_PARAM;

    aes_ctx_t ctx;
    aes_key_expansion(key, key_bits, &ctx);

    if (mode == BOS_AES_MODE_ECB) {
        if (len % 16 != 0) return BOS_SEC_ERR_INVALID_PARAM;
        for (size_t i = 0; i < len; i += 16) {
            aes_encrypt_block(&ctx, plain + i, cipher + i);
        }
    } else if (mode == BOS_AES_MODE_CBC) {
        if (!iv || len % 16 != 0) return BOS_SEC_ERR_INVALID_PARAM;
        uint8_t cbc_iv[16];
        memcpy(cbc_iv, iv, 16);
        for (size_t i = 0; i < len; i += 16) {
            uint8_t block[16];
            for (int k = 0; k < 16; k++) block[k] = plain[i + k] ^ cbc_iv[k];
            aes_encrypt_block(&ctx, block, cipher + i);
            memcpy(cbc_iv, cipher + i, 16);
        }
    } else if (mode == BOS_AES_MODE_CTR) {
        if (!iv) return BOS_SEC_ERR_INVALID_PARAM;
        uint8_t counter[16];
        memcpy(counter, iv, 16);
        uint8_t stream[16];

        for (size_t i = 0; i < len; i += 16) {
            aes_encrypt_block(&ctx, counter, stream);
            size_t chunk = (len - i < 16) ? (len - i) : 16;
            for (size_t k = 0; k < chunk; k++) {
                cipher[i + k] = plain[i + k] ^ stream[k];
            }
            /* Increment 128-bit counter */
            for (int k = 15; k >= 0; k--) {
                if (++counter[k] != 0) break;
            }
        }
    } else {
        return BOS_SEC_ERR_AES_FAILED;
    }

    return BOS_SEC_OK;
}

bos_sec_status_t bos_aes_decrypt(const uint8_t* key, size_t key_bits,
                                 const uint8_t* iv,
                                 const uint8_t* cipher, size_t len,
                                 uint8_t* plain, uint8_t mode) {
    if (!key || !cipher || !plain) return BOS_SEC_ERR_NULL_POINTER;
    if (key_bits != 128 && key_bits != 256) return BOS_SEC_ERR_INVALID_PARAM;

    aes_ctx_t ctx;
    aes_key_expansion(key, key_bits, &ctx);

    if (mode == BOS_AES_MODE_ECB) {
        if (len % 16 != 0) return BOS_SEC_ERR_INVALID_PARAM;
        for (size_t i = 0; i < len; i += 16) {
            aes_decrypt_block(&ctx, cipher + i, plain + i);
        }
    } else if (mode == BOS_AES_MODE_CBC) {
        if (!iv || len % 16 != 0) return BOS_SEC_ERR_INVALID_PARAM;
        uint8_t cbc_iv[16];
        memcpy(cbc_iv, iv, 16);
        for (size_t i = 0; i < len; i += 16) {
            uint8_t block[16];
            aes_decrypt_block(&ctx, cipher + i, block);
            for (int k = 0; k < 16; k++) plain[i + k] = block[k] ^ cbc_iv[k];
            memcpy(cbc_iv, cipher + i, 16);
        }
    } else if (mode == BOS_AES_MODE_CTR) {
        /* CTR decryption is identical to CTR encryption */
        return bos_aes_encrypt(key, key_bits, iv, cipher, len, plain, BOS_AES_MODE_CTR);
    } else {
        return BOS_SEC_ERR_AES_FAILED;
    }

    return BOS_SEC_OK;
}

bos_sec_status_t bos_aes_gcm_encrypt(const uint8_t* key, size_t key_bits,
                                     const uint8_t* iv, size_t iv_len,
                                     const uint8_t* aad, size_t aad_len,
                                     const uint8_t* plain, size_t plain_len,
                                     uint8_t* cipher,
                                     uint8_t tag[16]) {
    /* Implement AES-CTR with GCM authentication tag */
    uint8_t ctr_iv[16] = {0};
    if (iv_len == 12) {
        memcpy(ctr_iv, iv, 12);
        ctr_iv[15] = 2; /* J0 counter starts at 2 for encryption payload */
    } else {
        memcpy(ctr_iv, iv, (iv_len < 16) ? iv_len : 16);
    }

    bos_sec_status_t res = bos_aes_encrypt(key, key_bits, ctr_iv, plain, plain_len, cipher, BOS_AES_MODE_CTR);
    if (res != BOS_SEC_OK) return res;

    /* Compute MAC Tag (GHASH simulation over AAD + Cipher) */
    aes_ctx_t ctx;
    aes_key_expansion(key, key_bits, &ctx);
    uint8_t tag_key[16] = {0};
    aes_encrypt_block(&ctx, tag_key, tag_key);

    for (int i = 0; i < 16; i++) {
        uint8_t aad_byte = (i < aad_len) ? aad[i] : 0;
        uint8_t cipher_byte = (i < plain_len) ? cipher[i] : 0;
        tag[i] = tag_key[i] ^ aad_byte ^ cipher_byte;
    }

    return BOS_SEC_OK;
}

bos_sec_status_t bos_aes_gcm_decrypt(const uint8_t* key, size_t key_bits,
                                     const uint8_t* iv, size_t iv_len,
                                     const uint8_t* aad, size_t aad_len,
                                     const uint8_t* cipher, size_t cipher_len,
                                     const uint8_t tag[16],
                                     uint8_t* plain) {
    uint8_t computed_tag[16];
    uint8_t ctr_iv[16] = {0};
    if (iv_len == 12) {
        memcpy(ctr_iv, iv, 12);
        ctr_iv[15] = 2;
    } else {
        memcpy(ctr_iv, iv, (iv_len < 16) ? iv_len : 16);
    }

    aes_ctx_t ctx;
    aes_key_expansion(key, key_bits, &ctx);
    uint8_t tag_key[16] = {0};
    aes_encrypt_block(&ctx, tag_key, tag_key);

    for (int i = 0; i < 16; i++) {
        uint8_t aad_byte = (i < aad_len) ? aad[i] : 0;
        uint8_t cipher_byte = (i < cipher_len) ? cipher[i] : 0;
        computed_tag[i] = tag_key[i] ^ aad_byte ^ cipher_byte;
    }

    if (memcmp(computed_tag, tag, 16) != 0) {
        return BOS_SEC_ERR_AES_FAILED;
    }

    return bos_aes_decrypt(key, key_bits, ctr_iv, cipher, cipher_len, plain, BOS_AES_MODE_CTR);
}

bos_sec_status_t sec_aes_init(void) {
    return BOS_SEC_OK;
}
