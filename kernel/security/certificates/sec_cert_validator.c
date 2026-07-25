/*
 * BOS OS — Phase 3: Production TLS & Security Engine
 * sec_cert_validator.c — Certificate Validation Engine Implementation
 */

#include "kernel/security/certificates/sec_cert_validator.h"
#include "kernel/security/include/bos_rsa.h"
#include "kernel/security/include/bos_ecc.h"
#include "kernel/security/include/bos_hash.h"
#include "kernel/core/lib/include/string.h"

static const char* bos_strchr(const char* s, int c) {
    while (*s) {
        if (*s == (char)c) return s;
        s++;
    }
    return (*s == (char)c) ? s : NULL;
}

static bool match_domain_name(const char* cert_name, const char* expected_domain) {
    if (!cert_name || !expected_domain) return false;
    if (strcmp(cert_name, expected_domain) == 0) return true;

    /* Check wildcard match e.g. *.bos.org matching api.bos.org */
    if (cert_name[0] == '*' && cert_name[1] == '.') {
        const char* cert_suffix = cert_name + 2;
        const char* dot = bos_strchr(expected_domain, '.');
        if (dot) {
            const char* exp_suffix = dot + 1;
            if (strcmp(cert_suffix, exp_suffix) == 0) return true;
        }
    }
    return false;
}

bos_sec_status_t bos_cert_verify(const bos_x509_cert_t* cert,
                                const bos_x509_cert_t* chain,
                                size_t chain_count,
                                const char* expected_domain) {
    if (!cert) return BOS_SEC_ERR_NULL_POINTER;

    /* 1. Check Expiration Timestamp */
    uint64_t now = 1721000000ULL; /* Simulated current time */
    if (cert->not_before > 0 && now < cert->not_before) return BOS_SEC_ERR_CERT_EXPIRED;
    if (cert->not_after > 0 && now > cert->not_after) return BOS_SEC_ERR_CERT_EXPIRED;

    /* 2. Check Domain Matching if expected_domain is supplied */
    if (expected_domain && strlen(expected_domain) > 0) {
        bool domain_matched = false;
        if (match_domain_name(cert->subject.cn, expected_domain)) {
            domain_matched = true;
        }
        for (size_t i = 0; i < cert->san_count; i++) {
            if (match_domain_name(cert->san_dns[i], expected_domain)) {
                domain_matched = true;
                break;
            }
        }
        if (!domain_matched) return BOS_SEC_ERR_CERT_DOMAIN_MISMATCH;
    }

    /* 3. Check Signature Algorithm */
    if (cert->sig_algo == 0) return BOS_SEC_ERR_CERT_WEAK_ALGO;

    /* 4. Validate Chain & Signature */
    if (chain_count > 0 && chain != NULL) {
        /* Verify end-entity cert against intermediate/root parent */
        const bos_x509_cert_t* parent = &chain[0];
        if (strcmp(cert->issuer.cn, parent->subject.cn) != 0) {
            return BOS_SEC_ERR_CERT_INVALID_CHAIN;
        }

        /* Verify signature with parent public key */
        uint8_t hash[32];
        bos_sha256(cert->raw_der, cert->raw_len > 0 ? cert->raw_len : 128, hash);

        if (parent->pubkey.type == BOS_KEY_TYPE_RSA) {
            if (bos_rsa_verify(&parent->pubkey.key.rsa, hash, cert->signature, cert->sig_len) != BOS_SEC_OK) {
                return BOS_SEC_ERR_CERT_INVALID_SIG;
            }
        } else if (parent->pubkey.type == BOS_KEY_TYPE_ECC) {
            uint8_t r[32], s[32];
            memcpy(r, cert->signature, 32);
            memcpy(s, cert->signature + 32, 32);
            if (bos_ecc_verify(&parent->pubkey.key.ecc, hash, r, s) != BOS_SEC_OK) {
                return BOS_SEC_ERR_CERT_INVALID_SIG;
            }
        }
    } else {
        /* Self-signed root or direct root check */
        if (strcmp(cert->issuer.cn, cert->subject.cn) != 0) {
            /* Unknown root issuer */
            return BOS_SEC_ERR_CERT_UNKNOWN_ROOT;
        }
    }

    return BOS_SEC_OK;
}

bos_sec_status_t sec_cert_validator_init(void) {
    return BOS_SEC_OK;
}
