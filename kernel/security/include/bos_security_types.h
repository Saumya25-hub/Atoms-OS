/*
 * BOS OS — Phase 3: Production TLS & Security Engine
 * bos_security_types.h — Core Data Types, Status Codes, and Definitions
 */

#ifndef BOS_SECURITY_TYPES_H
#define BOS_SECURITY_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================
 * Structured Security Status Codes
 * ============================================================ */
typedef enum {
    BOS_SEC_OK                          = 0,
    BOS_SEC_ERR_NULL_POINTER            = -1,
    BOS_SEC_ERR_INVALID_PARAM           = -2,
    BOS_SEC_ERR_NO_MEMORY               = -3,
    BOS_SEC_ERR_BUFFER_TOO_SMALL        = -4,
    BOS_SEC_ERR_NOT_INITIALIZED         = -5,
    BOS_SEC_ERR_ALREADY_INITIALIZED      = -6,
    BOS_SEC_ERR_RNG_FAILURE             = -7,
    BOS_SEC_ERR_HASH_FAILED             = -8,
    BOS_SEC_ERR_AES_FAILED              = -9,
    BOS_SEC_ERR_RSA_KEY_INVALID         = -10,
    BOS_SEC_ERR_RSA_SIG_INVALID         = -11,
    BOS_SEC_ERR_ECC_KEY_INVALID         = -12,
    BOS_SEC_ERR_ECC_SIG_INVALID         = -13,
    BOS_SEC_ERR_CERT_MALFORMED          = -14,
    BOS_SEC_ERR_CERT_EXPIRED            = -15,
    BOS_SEC_ERR_CERT_INVALID_CHAIN      = -16,
    BOS_SEC_ERR_CERT_INVALID_SIG        = -17,
    BOS_SEC_ERR_CERT_WEAK_ALGO          = -18,
    BOS_SEC_ERR_CERT_UNKNOWN_ROOT       = -19,
    BOS_SEC_ERR_CERT_DOMAIN_MISMATCH    = -20,
    BOS_SEC_ERR_TRUST_STORE_FULL        = -21,
    BOS_SEC_ERR_TLS_HANDSHAKE_FAILED    = -22,
    BOS_SEC_ERR_TLS_RECORD_CORRUPT      = -23,
    BOS_SEC_ERR_TLS_CIPHER_UNSUPPORTED  = -24,
    BOS_SEC_ERR_TLS_CLOSED              = -25,
    BOS_SEC_ERR_SESSION_EXPIRED         = -26,
    BOS_SEC_ERR_SESSION_NOT_FOUND       = -27,
    BOS_SEC_ERR_IO                      = -28,
    BOS_SEC_ERR_GENERIC                 = -99
} bos_sec_status_t;

/* ============================================================
 * Cryptographic Algorithm Enums
 * ============================================================ */
typedef enum {
    BOS_HASH_SHA256 = 1,
    BOS_HASH_SHA384 = 2,
    BOS_HASH_SHA512 = 3
} bos_hash_algo_t;

typedef enum {
    BOS_AES_MODE_ECB = 1,
    BOS_AES_MODE_CBC = 2,
    BOS_AES_MODE_CTR = 3,
    BOS_AES_MODE_GCM = 4
} bos_aes_mode_t;

typedef enum {
    BOS_CIPHER_TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256 = 0xC02F,
    BOS_CIPHER_TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256 = 0xC02B,
    BOS_CIPHER_TLS_RSA_WITH_AES_256_CBC_SHA256        = 0x003D,
    BOS_CIPHER_TLS_RSA_WITH_AES_128_CBC_SHA256        = 0x003C
} bos_cipher_suite_id_t;

/* ============================================================
 * Key Structures
 * ============================================================ */
#define BOS_RSA_MAX_KEY_BITS 4096
#define BOS_RSA_MAX_BYTES    (BOS_RSA_MAX_KEY_BITS / 8)

typedef struct {
    uint32_t bits;
    uint8_t  n[BOS_RSA_MAX_BYTES]; /* Modulus */
    size_t   n_len;
    uint8_t  e[8];                /* Public Exponent (typically 65537) */
    size_t   e_len;
    uint8_t  d[BOS_RSA_MAX_BYTES]; /* Private Exponent (0 if public key only) */
    size_t   d_len;
} bos_rsa_key_t;

#define BOS_ECC_P256_BYTES 32

typedef struct {
    uint8_t x[BOS_ECC_P256_BYTES]; /* Curve point X */
    uint8_t y[BOS_ECC_P256_BYTES]; /* Curve point Y */
    uint8_t d[BOS_ECC_P256_BYTES]; /* Private key scalar (0 if public key only) */
    bool    has_private;
} bos_ecc_key_t;

/* ============================================================
 * Public Key Container
 * ============================================================ */
typedef enum {
    BOS_KEY_TYPE_RSA = 1,
    BOS_KEY_TYPE_ECC = 2
} bos_pubkey_type_t;

typedef struct {
    bos_pubkey_type_t type;
    union {
        bos_rsa_key_t rsa;
        bos_ecc_key_t ecc;
    } key;
} bos_pubkey_t;

/* ============================================================
 * Certificate Structures
 * ============================================================ */
#define BOS_X509_MAX_STR      128
#define BOS_X509_MAX_SAN      8
#define BOS_X509_MAX_CERT_RAW 2048

typedef struct {
    char     cn[BOS_X509_MAX_STR];
    char     org[BOS_X509_MAX_STR];
    char     country[32];
} bos_x509_name_t;

typedef struct {
    uint8_t  serial[32];
    size_t   serial_len;
    bos_x509_name_t issuer;
    bos_x509_name_t subject;
    uint64_t not_before; /* Epoch seconds */
    uint64_t not_after;  /* Epoch seconds */
    
    bos_pubkey_t pubkey;
    
    bos_hash_algo_t sig_algo;
    uint8_t  signature[512];
    size_t   sig_len;
    
    bool     is_ca;
    uint32_t key_usage;  /* Bitmask: 1=digitalSignature, 2=keyEncipherment, 4=keyCertSign */
    char     san_dns[BOS_X509_MAX_SAN][BOS_X509_MAX_STR];
    size_t   san_count;
    
    uint8_t  raw_der[BOS_X509_MAX_CERT_RAW];
    size_t   raw_len;
    uint8_t  fingerprint_sha256[32];
} bos_x509_cert_t;

/* ============================================================
 * TLS Connection Handle
 * ============================================================ */
typedef uint32_t bos_tls_handle_t;
#define BOS_TLS_INVALID_HANDLE 0xFFFFFFFF

#ifdef __cplusplus
}
#endif

#endif /* BOS_SECURITY_TYPES_H */
