#ifndef KERNEL_TRUST_STORE_H
#define KERNEL_TRUST_STORE_H

#include <stdint.h>
#include <stdbool.h>
#include "kernel/crypto/x509/x509.h"
#include "kernel/crypto/rsa/rsa.h"

typedef struct {
    char name[64];
    char organization[64];
    RsaPublicKey pubkey;
    uint8_t fingerprint_sha256[32];
    bool is_trusted;
} TrustAnchor;

void trust_store_init(void);
bool trust_store_is_ca_trusted(const X509Cert* ca_cert);
bool trust_verify_chain(const X509Cert* leaf, const X509Cert* intermediate, const X509Cert* root_ca);

#endif // KERNEL_TRUST_STORE_H
