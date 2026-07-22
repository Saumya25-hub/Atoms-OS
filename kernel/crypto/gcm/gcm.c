#include "gcm.h"
#include "kernel/core/lib/include/string.h"

static void gf_mul(const uint8_t x[16], const uint8_t y[16], uint8_t z[16]) {
    uint8_t V[16];
    uint8_t Z[16];
    memset(Z, 0, 16);
    memcpy(V, y, 16);

    for (int i = 0; i < 128; i++) {
        int bit = (x[i / 8] >> (7 - (i % 8))) & 1;
        if (bit) {
            for (int j = 0; j < 16; j++) Z[j] ^= V[j];
        }
        int lsb = V[15] & 1;
        for (int j = 15; j > 0; j--) {
            V[j] = (V[j] >> 1) | ((V[j - 1] & 1) << 7);
        }
        V[0] >>= 1;
        if (lsb) {
            V[0] ^= 0xE1;
        }
    }
    memcpy(z, Z, 16);
}

static void ghash(const uint8_t H[16], const uint8_t* aad, size_t aad_len,
                  const uint8_t* cipher, size_t cipher_len, uint8_t out[16]) {
    uint8_t Y[16];
    memset(Y, 0, 16);

    // AAD blocks
    size_t aad_blocks = (aad_len + 15) / 16;
    for (size_t i = 0; i < aad_blocks; i++) {
        uint8_t block[16];
        memset(block, 0, 16);
        size_t rem = aad_len - i * 16;
        if (rem > 16) rem = 16;
        memcpy(block, aad + i * 16, rem);
        for (int j = 0; j < 16; j++) Y[j] ^= block[j];
        gf_mul(Y, H, Y);
    }

    // Ciphertext blocks
    size_t cipher_blocks = (cipher_len + 15) / 16;
    for (size_t i = 0; i < cipher_blocks; i++) {
        uint8_t block[16];
        memset(block, 0, 16);
        size_t rem = cipher_len - i * 16;
        if (rem > 16) rem = 16;
        memcpy(block, cipher + i * 16, rem);
        for (int j = 0; j < 16; j++) Y[j] ^= block[j];
        gf_mul(Y, H, Y);
    }

    // Length block: 64-bit len(AAD) in bits, 64-bit len(C) in bits
    uint8_t len_block[16];
    uint64_t aad_bits = (uint64_t)aad_len * 8;
    uint64_t cipher_bits = (uint64_t)cipher_len * 8;

    for (int i = 0; i < 8; i++) {
        len_block[i]     = (uint8_t)((aad_bits >> ((7 - i) * 8)) & 0xFF);
        len_block[8 + i] = (uint8_t)((cipher_bits >> ((7 - i) * 8)) & 0xFF);
    }

    for (int j = 0; j < 16; j++) Y[j] ^= len_block[j];
    gf_mul(Y, H, out);
}

int gcm_encrypt(const AES_CTX* aes,
                const uint8_t iv[12],
                const uint8_t* aad, size_t aad_len,
                const uint8_t* plain, size_t plain_len,
                uint8_t* cipher, uint8_t tag[16]) {
    if (!aes || !iv || !tag) return -1;

    // H = E(K, 0^128)
    uint8_t zero[16] = {0};
    uint8_t H[16];
    aes_encrypt_block(aes, zero, H);

    // J0 = IV || 0x00000001
    uint8_t J0[16];
    memcpy(J0, iv, 12);
    J0[12] = 0; J0[13] = 0; J0[14] = 0; J0[15] = 1;

    // CTR mode encryption starting from counter J0 + 1
    uint8_t counter[16];
    memcpy(counter, J0, 16);

    size_t blocks = (plain_len + 15) / 16;
    for (size_t i = 0; i < blocks; i++) {
        // Increment 32-bit counter
        uint32_t c = ((uint32_t)counter[12] << 24) | ((uint32_t)counter[13] << 16) |
                     ((uint32_t)counter[14] << 8) | counter[15];
        c++;
        counter[12] = (c >> 24) & 0xFF; counter[13] = (c >> 16) & 0xFF;
        counter[14] = (c >> 8) & 0xFF;  counter[15] = c & 0xFF;

        uint8_t ectr[16];
        aes_encrypt_block(aes, counter, ectr);

        size_t rem = plain_len - i * 16;
        if (rem > 16) rem = 16;
        for (size_t j = 0; j < rem; j++) {
            cipher[i * 16 + j] = plain[i * 16 + j] ^ ectr[j];
        }
    }

    // GHASH(H, AAD, C)
    uint8_t S[16];
    ghash(H, aad, aad_len, cipher, plain_len, S);

    // Tag = S ^ E(K, J0)
    uint8_t EJ0[16];
    aes_encrypt_block(aes, J0, EJ0);
    for (int i = 0; i < 16; i++) {
        tag[i] = S[i] ^ EJ0[i];
    }

    return 0;
}

int gcm_decrypt(const AES_CTX* aes,
                const uint8_t iv[12],
                const uint8_t* aad, size_t aad_len,
                const uint8_t* cipher, size_t cipher_len,
                const uint8_t tag[16],
                uint8_t* plain) {
    if (!aes || !iv || !cipher || !tag || !plain) return -1;

    // H = E(K, 0^128)
    uint8_t zero[16] = {0};
    uint8_t H[16];
    aes_encrypt_block(aes, zero, H);

    // J0 = IV || 0x00000001
    uint8_t J0[16];
    memcpy(J0, iv, 12);
    J0[12] = 0; J0[13] = 0; J0[14] = 0; J0[15] = 1;

    // GHASH(H, AAD, C)
    uint8_t S[16];
    ghash(H, aad, aad_len, cipher, cipher_len, S);

    // Tag check: S ^ E(K, J0)
    uint8_t EJ0[16];
    aes_encrypt_block(aes, J0, EJ0);

    uint8_t expected_tag[16];
    for (int i = 0; i < 16; i++) {
        expected_tag[i] = S[i] ^ EJ0[i];
    }

    // Constant-time tag check
    int diff = 0;
    for (int i = 0; i < 16; i++) {
        diff |= (expected_tag[i] ^ tag[i]);
    }
    if (diff != 0) {
        return -1; // Authentication failure
    }

    // CTR mode decryption starting from counter J0 + 1
    uint8_t counter[16];
    memcpy(counter, J0, 16);

    size_t blocks = (cipher_len + 15) / 16;
    for (size_t i = 0; i < blocks; i++) {
        uint32_t c = ((uint32_t)counter[12] << 24) | ((uint32_t)counter[13] << 16) |
                     ((uint32_t)counter[14] << 8) | counter[15];
        c++;
        counter[12] = (c >> 24) & 0xFF; counter[13] = (c >> 16) & 0xFF;
        counter[14] = (c >> 8) & 0xFF;  counter[15] = c & 0xFF;

        uint8_t ectr[16];
        aes_encrypt_block(aes, counter, ectr);

        size_t rem = cipher_len - i * 16;
        if (rem > 16) rem = 16;
        for (size_t j = 0; j < rem; j++) {
            plain[i * 16 + j] = cipher[i * 16 + j] ^ ectr[j];
        }
    }

    return 0;
}
