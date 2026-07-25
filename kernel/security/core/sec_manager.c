/*
 * BOS OS — Phase 3: Production TLS & Security Engine
 * sec_manager.c — Subsystem Lifecycle & Orchestration Implementation
 */

#include "kernel/security/core/sec_manager.h"
#include "kernel/security/debug/sec_debug.h"
#include "kernel/security/diagnostics/sec_diag.h"
#include "kernel/security/random/sec_random.h"
#include "kernel/security/hash/sec_hash.h"
#include "kernel/security/aes/sec_aes.h"
#include "kernel/security/rsa/sec_rsa.h"
#include "kernel/security/ecc/sec_ecc.h"
#include "kernel/security/x509/sec_x509.h"
#include "kernel/security/certificates/sec_cert_validator.h"
#include "kernel/security/trust_store/sec_trust_store.h"
#include "kernel/security/session/sec_session.h"
#include "kernel/security/tls/sec_tls.h"
#include "kernel/security/tests/sec_test_suite.h"

extern void display_print(const char* str);

static bool g_sec_initialized = false;

bos_sec_status_t bos_security_init(void) {
    if (g_sec_initialized) return BOS_SEC_OK;

    display_print("=====================================================\n");
    display_print("[SECURITY] Initializing BOS OS Security Engine...\n\n");

    sec_debug_init();
    sec_diag_init();

    sec_random_init();

    sec_hash_init();
    display_print("[CRYPTO] SHA Engine Ready\n\n");

    sec_aes_init();
    display_print("[AES] AES Engine Ready\n\n");

    sec_rsa_init();
    display_print("[RSA] RSA Engine Ready\n\n");

    sec_ecc_init();
    display_print("[ECC] ECC Engine Ready\n\n");

    sec_x509_init();
    display_print("[X509] Certificate Engine Ready\n\n");

    bos_trust_store_init();
    sec_cert_validator_init();
    display_print("[TRUST] Trust Store Loaded\n\n");

    sec_session_init();
    sec_tls_init();
    display_print("[TLS] TLS Engine Ready\n");
    display_print("=====================================================\n\n");

    g_sec_initialized = true;
    return BOS_SEC_OK;
}

bool bos_security_is_initialized(void) {
    return g_sec_initialized;
}

bos_sec_status_t bos_security_run_certification_tests(void) {
    if (!g_sec_initialized) {
        bos_security_init();
    }
    return sec_run_certification_test_suite();
}
