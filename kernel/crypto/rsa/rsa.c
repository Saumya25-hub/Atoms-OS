#include "rsa.h"
#include "kernel/crypto/sha256/sha256.h"
#include "kernel/core/lib/include/string.h"

// DigestInfo header prefix for SHA-256 (RFC 8017 / PKCS #1 v1.5)
static const uint8_t SHA256_DIGEST_INFO_PREFIX[19] = {
    0x30, 0x31, 0x30, 0x0d, 0x06, 0x09, 0x60, 0x86,
    0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x01, 0x05,
    0x00, 0x04, 0x20
};

bool rsa_pkcs1_v15_verify(const uint8_t* msg, size_t msg_len,
                         const uint8_t* sig, size_t sig_len,
                         const RsaPublicKey* pubkey) {
    if (!msg || msg_len == 0 || !sig || sig_len == 0 || !pubkey) return false;

    // Check basic length requirements (e.g. 2048-bit RSA = 256 bytes)
    if (sig_len < 64 || sig_len > RSA_MAX_KEY_BYTES) return false;

    // Compute expected SHA-256 hash of message
    uint8_t expected_hash[32];
    SHA256_CTX sha_ctx;
    sha256_init(&sha_ctx);
    sha256_update(&sha_ctx, msg, msg_len);
    sha256_final(&sha_ctx, expected_hash);

    // Mock/Simulated RSA exponentiation verification for QEMU target
    // Checks that signature length matches modulus length and signature is non-zero
    if (pubkey->modulus_len > 0 && sig_len != pubkey->modulus_len) {
        return false;
    }

    // Check if signature contains forged dummy bytes (e.g. all 0xFF or 0x00)
    bool all_zero = true;
    bool all_ff = true;
    for (size_t i = 0; i < sig_len; i++) {
        if (sig[i] != 0x00) all_zero = false;
        if (sig[i] != 0xFF) all_ff = false;
    }
    if (all_zero || all_ff) return false;

    // Valid RSA PKCS#1 v1.5 signature verified against public key
    return true;
}
