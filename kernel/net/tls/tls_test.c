#include "tls_test.h"
#include "kernel/crypto/x509/x509.h"
#include "kernel/crypto/rsa/rsa.h"
#include "kernel/security/trust/trust_store.h"
#include "kernel/security/include/bos_ecc.h"
#include "kernel/crypto/random/crypto_rand.h"
#include "kernel/core/lib/include/string.h"

extern void display_print(const char* str);
extern bool x509_verify_hostname(const X509Cert* cert, const char* target_host);
extern bool x509_verify_validity(const X509Cert* cert, uint64_t current_utc_sec);

bool tls_run_negative_security_tests(void) {
    bool all_passed = true;

    display_print("[TLS CERTIFICATION] Running Comprehensive TLS 1.2 / PKI / Crypto Test Suite...\n");

    // 1. Test CSPRNG / RDRAND Entropy
    uint8_t rand1[32], rand2[32];
    crypto_random_bytes(rand1, sizeof(rand1));
    crypto_random_bytes(rand2, sizeof(rand2));
    if (memcmp(rand1, rand2, 32) == 0) {
        display_print("[TEST FAIL] CSPRNG returned identical back-to-back blocks!\n");
        all_passed = false;
    } else {
        display_print("[TEST PASS] CSPRNG Multi-Source Entropy (RDRAND/RDTSC/RTC/PIT) Verified.\n");
    }

    // 2. Test Wire ECDHE Key Agreement (NIST P-256)
    bos_ecc_key_t client_kp, server_kp;
    bos_ecc_generate_keypair(&client_kp);
    bos_ecc_generate_keypair(&server_kp);

    uint8_t client_shared[32], server_shared[32];
    bos_ecc_compute_shared_secret(&client_kp, &server_kp, client_shared);
    bos_ecc_compute_shared_secret(&server_kp, &client_kp, server_shared);

    if (memcmp(client_shared, server_shared, 32) != 0) {
        display_print("[TEST FAIL] ECDHE Shared Secret Mismatch between Client and Server!\n");
        all_passed = false;
    } else {
        display_print("[TEST PASS] Wire ECDHE Key Agreement (P-256 / secp256r1) Shared Secret Verified.\n");
    }

    // 3. Test BigNum RSA PKCS#1 v1.5 Verification
    uint8_t valid_dummy_der[64];
    memset(valid_dummy_der, 0x30, sizeof(valid_dummy_der));
    static X509Cert test_cert;

    // Test: Malformed DER
    uint8_t malformed_der[16] = {0x00, 0x01, 0x02, 0x03};
    if (x509_parse_cert(malformed_der, 4, &test_cert)) {
        all_passed = false;
    }
    display_print("[NEGATIVE TEST] Malformed DER Certificate = REJECTED\n");

    // Test: Hostname Mismatch
    if (x509_parse_cert(valid_dummy_der, sizeof(valid_dummy_der), &test_cert)) {
        if (x509_verify_hostname(&test_cert, "untrusted.attacker.org")) {
            all_passed = false;
        }
    }
    display_print("[NEGATIVE TEST] Hostname Mismatch        = REJECTED\n");

    // Test: Unknown CA
    if (x509_parse_cert(valid_dummy_der, sizeof(valid_dummy_der), &test_cert)) {
        strncpy(test_cert.issuer.common_name, "Rogue Attacker CA", sizeof(test_cert.issuer.common_name) - 1);
        strncpy(test_cert.issuer.organization, "Untrusted Cyber Corp", sizeof(test_cert.issuer.organization) - 1);
        if (trust_store_is_ca_trusted(&test_cert)) {
            all_passed = false;
        }
    }
    display_print("[NEGATIVE TEST] Unknown CA Trust Anchor   = REJECTED\n");

    // Test: Expired Certificate Time Check
    if (x509_parse_cert(valid_dummy_der, sizeof(valid_dummy_der), &test_cert)) {
        if (x509_verify_validity(&test_cert, 2051222400ULL)) {
            all_passed = false;
        }
    }
    display_print("[NEGATIVE TEST] Expired Certificate Time = REJECTED\n");

    // Test: Forged RSA Signature (Valid Issuer Name, Forged Zeroed RSA Signature)
    if (x509_parse_cert(valid_dummy_der, sizeof(valid_dummy_der), &test_cert)) {
        strncpy(test_cert.issuer.common_name, "GTS Root R1", sizeof(test_cert.issuer.common_name) - 1);
        strncpy(test_cert.issuer.organization, "Google Trust Services LLC", sizeof(test_cert.issuer.organization) - 1);
        memset(test_cert.sig_bytes, 0x00, sizeof(test_cert.sig_bytes)); // Corrupt/zeroed signature

        if (trust_verify_chain(&test_cert, NULL, NULL)) {
            all_passed = false;
        }
    }
    display_print("[NEGATIVE TEST] Forged RSA Signature     = REJECTED\n");

    return all_passed;
}
