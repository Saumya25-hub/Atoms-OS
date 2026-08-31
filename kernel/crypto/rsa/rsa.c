#include "rsa.h"
#include "kernel/crypto/sha256/sha256.h"
#include "kernel/core/lib/include/string.h"

// DigestInfo header prefix for SHA-256 (RFC 8017 / PKCS #1 v1.5)
static const uint8_t SHA256_DIGEST_INFO_PREFIX[19] = {
    0x30, 0x31, 0x30, 0x0d, 0x06, 0x09, 0x60, 0x86,
    0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x01, 0x05,
    0x00, 0x04, 0x20
};

#define BN_WORDS 72 // 72 * 64 bits = 4608 bits (supports up to 4096-bit RSA)

typedef struct {
    uint64_t d[BN_WORDS];
    size_t   n; // Number of active 64-bit limbs
} bn_t;

static void bn_zero(bn_t* a) {
    memset(a->d, 0, sizeof(a->d));
    a->n = 0;
}

static void bn_clamp(bn_t* a) {
    while (a->n > 0 && a->d[a->n - 1] == 0) {
        a->n--;
    }
}

static void bn_set_u64(bn_t* a, uint64_t v) {
    bn_zero(a);
    if (v > 0) {
        a->d[0] = v;
        a->n = 1;
    }
}

static void bn_from_bytes_be(bn_t* a, const uint8_t* b, size_t len) {
    bn_zero(a);
    if (!b || len == 0) return;

    for (size_t i = 0; i < len; i++) {
        size_t byte_idx_from_end = len - 1 - i;
        size_t word_idx = byte_idx_from_end / 8;
        size_t byte_in_word = byte_idx_from_end % 8;
        if (word_idx < BN_WORDS) {
            a->d[word_idx] |= ((uint64_t)b[i]) << (byte_in_word * 8);
            if (word_idx + 1 > a->n && a->d[word_idx] != 0) {
                a->n = word_idx + 1;
            }
        }
    }
    bn_clamp(a);
}

static void bn_to_bytes_be(const bn_t* a, uint8_t* out, size_t out_len) {
    memset(out, 0, out_len);
    for (size_t i = 0; i < out_len; i++) {
        size_t byte_idx_from_end = out_len - 1 - i;
        size_t word_idx = byte_idx_from_end / 8;
        size_t byte_in_word = byte_idx_from_end % 8;
        if (word_idx < BN_WORDS) {
            out[i] = (uint8_t)((a->d[word_idx] >> (byte_in_word * 8)) & 0xFF);
        }
    }
}

static int bn_cmp(const bn_t* a, const bn_t* b) {
    if (a->n > b->n) return 1;
    if (a->n < b->n) return -1;
    for (size_t i = a->n; i > 0; i--) {
        if (a->d[i - 1] > b->d[i - 1]) return 1;
        if (a->d[i - 1] < b->d[i - 1]) return -1;
    }
    return 0;
}

static void bn_sub(bn_t* r, const bn_t* a, const bn_t* b) {
    bn_t tmp;
    bn_zero(&tmp);
    unsigned __int128 borrow = 0;

    for (size_t i = 0; i < a->n; i++) {
        unsigned __int128 sub = (unsigned __int128)(i < b->n ? b->d[i] : 0) + borrow;
        if (a->d[i] >= sub) {
            tmp.d[i] = (uint64_t)(a->d[i] - sub);
            borrow = 0;
        } else {
            tmp.d[i] = (uint64_t)(((unsigned __int128)1 << 64) + a->d[i] - sub);
            borrow = 1;
        }
        tmp.n = i + 1;
    }
    bn_clamp(&tmp);
    *r = tmp;
}

static void bn_lshift1(bn_t* a) {
    uint64_t carry = 0;
    for (size_t i = 0; i < a->n || carry; i++) {
        if (i >= BN_WORDS) break;
        uint64_t next_carry = a->d[i] >> 63;
        a->d[i] = (a->d[i] << 1) | carry;
        carry = next_carry;
        if (i >= a->n && a->d[i] != 0) a->n = i + 1;
    }
    bn_clamp(a);
}

static void bn_rshift1(bn_t* a) {
    uint64_t carry = 0;
    for (size_t i = a->n; i > 0; i--) {
        uint64_t cur = a->d[i - 1];
        a->d[i - 1] = (cur >> 1) | (carry << 63);
        carry = cur & 1;
    }
    bn_clamp(a);
}

static void bn_mul(bn_t* r, const bn_t* a, const bn_t* b) {
    bn_t tmp;
    bn_zero(&tmp);
    if (a->n == 0 || b->n == 0) {
        *r = tmp;
        return;
    }

    for (size_t i = 0; i < a->n; i++) {
        unsigned __int128 carry = 0;
        for (size_t j = 0; j < b->n || carry; j++) {
            size_t idx = i + j;
            if (idx >= BN_WORDS) break;
            unsigned __int128 cur = (unsigned __int128)tmp.d[idx] + carry;
            if (j < b->n) {
                cur += (unsigned __int128)a->d[i] * b->d[j];
            }
            tmp.d[idx] = (uint64_t)cur;
            carry = cur >> 64;
            if (idx + 1 > tmp.n) tmp.n = idx + 1;
        }
    }
    bn_clamp(&tmp);
    *r = tmp;
}

static void bn_div_rem(const bn_t* num, const bn_t* den, bn_t* quo, bn_t* rem) {
    bn_t q, r;
    bn_zero(&q);
    bn_zero(&r);

    if (den->n == 0) {
        if (quo) *quo = q;
        if (rem) *rem = r;
        return;
    }

    size_t num_bits = 0;
    if (num->n > 0) {
        uint64_t msb_word = num->d[num->n - 1];
        num_bits = (num->n - 1) * 64;
        while (msb_word > 0) {
            num_bits++;
            msb_word >>= 1;
        }
    }

    for (size_t i = num_bits; i > 0; i--) {
        size_t bit_idx = i - 1;
        bn_lshift1(&r);
        size_t word_idx = bit_idx / 64;
        size_t bit_in_word = bit_idx % 64;
        if ((num->d[word_idx] >> bit_in_word) & 1) {
            r.d[0] |= 1;
            if (r.n == 0) r.n = 1;
        }
        bn_clamp(&r);

        if (bn_cmp(&r, den) >= 0) {
            bn_sub(&r, &r, den);
            q.d[word_idx] |= ((uint64_t)1 << bit_in_word);
            if (word_idx + 1 > q.n) q.n = word_idx + 1;
        }
    }
    bn_clamp(&q);
    bn_clamp(&r);

    if (quo) *quo = q;
    if (rem) *rem = r;
}

static void bn_mod_exp(bn_t* res, const bn_t* base, const bn_t* exp, const bn_t* mod) {
    bn_t r, b, e;
    bn_set_u64(&r, 1);
    b = *base;
    e = *exp;

    bn_div_rem(&b, mod, NULL, &b);

    while (e.n > 0) {
        if (e.d[0] & 1) {
            bn_t tmp;
            bn_mul(&tmp, &r, &b);
            bn_div_rem(&tmp, mod, NULL, &r);
        }
        bn_t sq;
        bn_mul(&sq, &b, &b);
        bn_div_rem(&sq, mod, NULL, &b);
        bn_rshift1(&e);
    }
    *res = r;
}

bool rsa_pkcs1_v15_verify(const uint8_t* msg, size_t msg_len,
                         const uint8_t* sig, size_t sig_len,
                         const RsaPublicKey* pubkey) {
    if (!msg || msg_len == 0 || !sig || sig_len == 0 || !pubkey) return false;
    if (sig_len < 64 || sig_len > RSA_MAX_KEY_BYTES) return false;
    if (pubkey->modulus_len > 0 && sig_len != pubkey->modulus_len) return false;

    // 1. Compute expected SHA-256 hash of message
    uint8_t expected_hash[32];
    SHA256_CTX sha_ctx;
    sha256_init(&sha_ctx);
    sha256_update(&sha_ctx, msg, msg_len);
    sha256_final(&sha_ctx, expected_hash);

    // 2. Load BigNum integers
    bn_t bn_sig, bn_mod, bn_exp, bn_em;
    bn_from_bytes_be(&bn_sig, sig, sig_len);
    bn_from_bytes_be(&bn_mod, pubkey->modulus, pubkey->modulus_len ? pubkey->modulus_len : sig_len);

    if (pubkey->exponent_len > 0) {
        bn_from_bytes_be(&bn_exp, pubkey->exponent, pubkey->exponent_len);
    } else {
        bn_set_u64(&bn_exp, pubkey->e_val ? pubkey->e_val : 65537);
    }

    // 3. Cryptographic Modular Exponentiation: EM = (sig ^ e) mod n
    bn_mod_exp(&bn_em, &bn_sig, &bn_exp, &bn_mod);

    // 4. Export EM byte representation (length = sig_len)
    uint8_t em_buf[RSA_MAX_KEY_BYTES];
    bn_to_bytes_be(&bn_em, em_buf, sig_len);

    // 5. Verify PKCS#1 v1.5 Structure: 0x00 0x01 [0xFF... >= 8 bytes] 0x00 [DigestInfo 19 bytes] [Hash 32 bytes]
    if (em_buf[0] != 0x00 || em_buf[1] != 0x01) {
        return false;
    }

    size_t pad_idx = 2;
    while (pad_idx < sig_len && em_buf[pad_idx] == 0xFF) {
        pad_idx++;
    }

    // At least 8 bytes of 0xFF padding required
    if (pad_idx - 2 < 8 || pad_idx >= sig_len || em_buf[pad_idx] != 0x00) {
        return false;
    }
    pad_idx++; // Skip 0x00 delimiter

    // Ensure space for 19-byte DigestInfo + 32-byte Hash
    if (sig_len - pad_idx != 19 + 32) {
        return false;
    }

    // Verify DigestInfo prefix
    if (memcmp(em_buf + pad_idx, SHA256_DIGEST_INFO_PREFIX, 19) != 0) {
        return false;
    }

    // Verify SHA-256 digest
    if (memcmp(em_buf + pad_idx + 19, expected_hash, 32) != 0) {
        return false;
    }

    return true;
}
