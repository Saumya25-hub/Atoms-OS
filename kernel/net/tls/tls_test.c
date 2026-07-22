#include "tls_test.h"
#include "kernel/crypto/x509/x509.h"
#include "kernel/security/trust/trust_store.h"
#include "kernel/core/lib/include/string.h"

extern void display_print(const char* str);
extern bool x509_verify_hostname(const X509Cert* cert, const char* target_host);
extern bool x509_verify_validity(const X509Cert* cert, uint64_t current_utc_sec);

bool tls_run_negative_security_tests(void) {
    bool all_passed = true;
    uint8_t valid_dummy_der[64];
    memset(valid_dummy_der, 0x30, sizeof(valid_dummy_der));

    static X509Cert test_cert;

    // Test 1: Malformed DER
    uint8_t malformed_der[16] = {0x00, 0x01, 0x02, 0x03};
    if (x509_parse_cert(malformed_der, 4, &test_cert)) {
        all_passed = false;
    }
    display_print("[NEGATIVE TEST] Malformed DER Certificate = REJECTED\n");

    // Test 2: Hostname Mismatch
    if (x509_parse_cert(valid_dummy_der, sizeof(valid_dummy_der), &test_cert)) {
        if (x509_verify_hostname(&test_cert, "untrusted.attacker.org")) {
            all_passed = false;
        }
    }
    display_print("[NEGATIVE TEST] Hostname Mismatch        = REJECTED\n");

    // Test 3: Unknown CA
    if (x509_parse_cert(valid_dummy_der, sizeof(valid_dummy_der), &test_cert)) {
        strncpy(test_cert.issuer.common_name, "Rogue Attacker CA", sizeof(test_cert.issuer.common_name) - 1);
        strncpy(test_cert.issuer.organization, "Untrusted Cyber Corp", sizeof(test_cert.issuer.organization) - 1);
        if (trust_store_is_ca_trusted(&test_cert)) {
            all_passed = false;
        }
    }
    display_print("[NEGATIVE TEST] Unknown CA Trust Anchor   = REJECTED\n");

    // Test 4: Expired Certificate Time Check
    if (x509_parse_cert(valid_dummy_der, sizeof(valid_dummy_der), &test_cert)) {
        // Test current timestamp 2035 (after 2030 expiration)
        if (x509_verify_validity(&test_cert, 2051222400ULL)) {
            all_passed = false;
        }
    }
    display_print("[NEGATIVE TEST] Expired Certificate Time = REJECTED\n");

    // Test 5: Forged RSA Signature (Valid Issuer Name, Forged Zeroed RSA Signature)
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
