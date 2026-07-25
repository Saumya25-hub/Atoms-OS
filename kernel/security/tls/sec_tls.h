/*
 * BOS OS — Phase 3: Production TLS & Security Engine
 * sec_tls.h — Internal TLS Subsystem & Cipher Negotiation Interface
 */

#ifndef SEC_TLS_H
#define SEC_TLS_H

#include "kernel/security/include/bos_tls.h"

bos_sec_status_t sec_tls_init(void);

typedef struct {
    uint16_t id;
    const char* name;
    uint8_t key_exchange; /* 1 = RSA, 2 = ECDHE */
    uint8_t cipher;       /* 1 = AES-128-GCM, 2 = AES-256-CBC, 3 = AES-128-CBC */
    uint8_t mac;          /* 1 = SHA256, 2 = SHA384 */
} cipher_suite_info_t;

const cipher_suite_info_t* sec_cipher_suite_lookup(uint16_t id);

#endif /* SEC_TLS_H */
