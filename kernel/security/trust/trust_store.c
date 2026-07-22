#include "trust_store.h"
#include "kernel/core/lib/include/string.h"

static TrustAnchor g_trust_anchors[4];
static size_t g_anchor_count = 0;

void trust_store_init(void) {
    memset(g_trust_anchors, 0, sizeof(g_trust_anchors));

    // Anchor 1: GTS Root R1
    strncpy(g_trust_anchors[0].name, "GTS Root R1", sizeof(g_trust_anchors[0].name) - 1);
    strncpy(g_trust_anchors[0].organization, "Google Trust Services LLC", sizeof(g_trust_anchors[0].organization) - 1);
    g_trust_anchors[0].is_trusted = true;

    // Anchor 2: GlobalSign Root CA
    strncpy(g_trust_anchors[1].name, "GlobalSign Root CA", sizeof(g_trust_anchors[1].name) - 1);
    strncpy(g_trust_anchors[1].organization, "GlobalSign nv-sa", sizeof(g_trust_anchors[1].organization) - 1);
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

    // Validate Issuer/Subject linkage
    if (intermediate) {
        if (strcmp(leaf->issuer.organization, intermediate->subject.organization) != 0 &&
            strcmp(leaf->issuer.common_name, intermediate->subject.common_name) != 0) {
            return false;
        }
    }

    // Verify root trust anchor acceptance
    if (root_ca) {
        return trust_store_is_ca_trusted(root_ca);
    }

    return trust_store_is_ca_trusted(leaf);
}
