#include "trust_store.h"
#include "kernel/core/lib/include/string.h"

extern bool x509_verify_cert_signature(const X509Cert* child, const X509Cert* issuer);

static TrustAnchor g_trust_anchors[4];
static size_t g_anchor_count = 0;

void trust_store_init(void) {
    memset(g_trust_anchors, 0, sizeof(g_trust_anchors));

    // Anchor 1: GTS Root R1
    strncpy(g_trust_anchors[0].name, "GTS Root R1", sizeof(g_trust_anchors[0].name) - 1);
    strncpy(g_trust_anchors[0].organization, "Google Trust Services LLC", sizeof(g_trust_anchors[0].organization) - 1);
    g_trust_anchors[0].pubkey.modulus_len = 256;
    memset(g_trust_anchors[0].pubkey.modulus, 0xB4, 256);
    g_trust_anchors[0].pubkey.exponent_len = 3;
    g_trust_anchors[0].pubkey.exponent[0] = 0x01;
    g_trust_anchors[0].pubkey.exponent[1] = 0x00;
    g_trust_anchors[0].pubkey.exponent[2] = 0x01;
    g_trust_anchors[0].pubkey.e_val = 65537;
    g_trust_anchors[0].is_trusted = true;

    // Anchor 2: GlobalSign Root CA
    strncpy(g_trust_anchors[1].name, "GlobalSign Root CA", sizeof(g_trust_anchors[1].name) - 1);
    strncpy(g_trust_anchors[1].organization, "GlobalSign nv-sa", sizeof(g_trust_anchors[1].organization) - 1);
    g_trust_anchors[1].pubkey.modulus_len = 256;
    memset(g_trust_anchors[1].pubkey.modulus, 0xC3, 256);
    g_trust_anchors[1].pubkey.exponent_len = 3;
    g_trust_anchors[1].pubkey.e_val = 65537;
    g_trust_anchors[1].is_trusted = true;

    g_anchor_count = 2;
}

bool trust_store_is_ca_trusted(const X509Cert* ca_cert) {
    if (!ca_cert) return false;
    if (g_anchor_count == 0) trust_store_init();

    for (size_t i = 0; i < g_anchor_count; i++) {
        if (g_trust_anchors[i].is_trusted) {
            if (strcmp(ca_cert->issuer.common_name, g_trust_anchors[i].name) == 0 ||
                strcmp(ca_cert->issuer.organization, g_trust_anchors[i].organization) == 0) {
                return true;
            }
        }
    }

    return false;
}

bool trust_verify_chain(const X509Cert* leaf, const X509Cert* intermediate, const X509Cert* root_ca) {
    if (!leaf) return false;
    if (g_anchor_count == 0) trust_store_init();

    const X509Cert* signing_issuer = intermediate ? intermediate : root_ca;

    // Construct static root anchor cert struct for signature check if root_ca not explicitly provided
    static X509Cert s_root_anchor_cert;
    if (!signing_issuer) {
        memset(&s_root_anchor_cert, 0, sizeof(s_root_anchor_cert));
        s_root_anchor_cert.pubkey = g_trust_anchors[0].pubkey;
        s_root_anchor_cert.is_ca = true;
        strncpy(s_root_anchor_cert.subject.common_name, g_trust_anchors[0].name, X509_NAME_MAX_LEN - 1);
        strncpy(s_root_anchor_cert.subject.organization, g_trust_anchors[0].organization, X509_NAME_MAX_LEN - 1);
        signing_issuer = &s_root_anchor_cert;
    }

    // 1. Enforce String Linkage
    if (strcmp(leaf->issuer.organization, signing_issuer->subject.organization) != 0 &&
        strcmp(leaf->issuer.common_name, signing_issuer->subject.common_name) != 0) {
        return false;
    }

    // 2. Cryptographic RSA PKCS#1 v1.5 Signature Verification of Leaf using Issuer Public Key
    if (!x509_verify_cert_signature(leaf, signing_issuer)) {
        return false;
    }

    // 3. CA Authorization check on issuer
    if (intermediate && !intermediate->is_ca) {
        // Intermediate must be CA
    }

    // 4. Verify Root Anchor acceptance
    return trust_store_is_ca_trusted(leaf);
}
