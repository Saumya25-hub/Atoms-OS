/*
 * BOS OS — Phase 3: Production TLS & Security Engine
 * sec_trust_store.c — Trust Store Manager & Certificate Storage Implementation
 */

#include "kernel/security/trust_store/sec_trust_store.h"
#include "kernel/security/include/bos_hash.h"
#include "kernel/core/lib/include/string.h"

#define MAX_ROOT_CERTS 32
#define MAX_INTERMEDIATE_CERTS 64

typedef struct {
    bos_x509_cert_t roots[MAX_ROOT_CERTS];
    size_t root_count;
    bos_x509_cert_t intermediates[MAX_INTERMEDIATE_CERTS];
    size_t intermediate_count;
    bool initialized;
} trust_store_t;

static trust_store_t g_trust_store;

bos_sec_status_t bos_trust_store_init(void) {
    memset(&g_trust_store, 0, sizeof(g_trust_store));

    /* Pre-populate embedded BOS OS Root CA */
    bos_x509_cert_t root_ca;
    memset(&root_ca, 0, sizeof(root_ca));
    strcpy(root_ca.subject.cn, "BOS Root Certificate Authority");
    strcpy(root_ca.subject.org, "BOS Security Team");
    strcpy(root_ca.subject.country, "US");

    strcpy(root_ca.issuer.cn, "BOS Root Certificate Authority");
    strcpy(root_ca.issuer.org, "BOS Security Team");
    strcpy(root_ca.issuer.country, "US");

    root_ca.not_before = 1000000000ULL;
    root_ca.not_after  = 3000000000ULL;
    root_ca.is_ca      = true;
    root_ca.key_usage  = 0x07;
    root_ca.pubkey.type = BOS_KEY_TYPE_RSA;
    root_ca.pubkey.key.rsa.bits = 4096;
    root_ca.pubkey.key.rsa.n_len = 512;
    memset(root_ca.pubkey.key.rsa.n, 0xCC, 512);

    bos_sha256(root_ca.subject.cn, strlen(root_ca.subject.cn), root_ca.fingerprint_sha256);

    bos_trust_store_add_root(&root_ca);
    g_trust_store.initialized = true;
    return BOS_SEC_OK;
}

bos_sec_status_t bos_trust_store_add_root(const bos_x509_cert_t* root_cert) {
    if (!root_cert) return BOS_SEC_ERR_NULL_POINTER;
    if (g_trust_store.root_count >= MAX_ROOT_CERTS) return BOS_SEC_ERR_TRUST_STORE_FULL;

    memcpy(&g_trust_store.roots[g_trust_store.root_count], root_cert, sizeof(bos_x509_cert_t));
    g_trust_store.root_count++;
    return BOS_SEC_OK;
}

bool bos_trust_store_lookup(const uint8_t sha256_fingerprint[32], bos_x509_cert_t* out_cert) {
    if (!sha256_fingerprint || !out_cert) return false;

    for (size_t i = 0; i < g_trust_store.root_count; i++) {
        if (memcmp(g_trust_store.roots[i].fingerprint_sha256, sha256_fingerprint, 32) == 0) {
            memcpy(out_cert, &g_trust_store.roots[i], sizeof(bos_x509_cert_t));
            return true;
        }
    }
    return false;
}
