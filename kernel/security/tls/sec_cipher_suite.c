/*
 * BOS OS — Phase 3: Production TLS & Security Engine
 * sec_cipher_suite.c — Cipher Suite Registry & Negotiation Manager
 */

#include "kernel/security/tls/sec_tls.h"
#include "kernel/core/lib/include/string.h"

static const cipher_suite_info_t SUPPORTED_SUITES[] = {
    { BOS_CIPHER_TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256, "TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256", 2, 1, 1 },
    { BOS_CIPHER_TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256, "TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256", 2, 1, 1 },
    { BOS_CIPHER_TLS_RSA_WITH_AES_256_CBC_SHA256, "TLS_RSA_WITH_AES_256_CBC_SHA256", 1, 2, 1 },
    { BOS_CIPHER_TLS_RSA_WITH_AES_128_CBC_SHA256, "TLS_RSA_WITH_AES_128_CBC_SHA256", 1, 3, 1 }
};

#define SUITE_COUNT (sizeof(SUPPORTED_SUITES) / sizeof(SUPPORTED_SUITES[0]))

const cipher_suite_info_t* sec_cipher_suite_lookup(uint16_t id) {
    for (size_t i = 0; i < SUITE_COUNT; i++) {
        if (SUPPORTED_SUITES[i].id == id) {
            return &SUPPORTED_SUITES[i];
        }
    }
    return NULL;
}
