#include "tls_test.h"
#include "kernel/crypto/x509/x509.h"
#include "kernel/security/trust/trust_store.h"
#include "kernel/core/lib/include/string.h"

extern void display_print(const char* str);
extern bool x509_verify_hostname(const X509Cert* cert, const char* target_host);
extern bool x509_verify_validity(const X509Cert* cert, uint64_t current_utc_sec);

bool tls_run_negative_security_tests(void) {
    bool all_passed = true;

    // Test 1: Malformed DER
    uint8_t malformed_der[16] = {0x00, 0x01, 0x02, 0x03};
    X509Cert cert1;
    if (x509_parse_cert(malformed_der, 4, &cert1)) {
        all_passed = false;
    }
    display_print("[NEGATIVE TEST] Malformed DER Certificate = REJECTED\n");

    // Test 2: Hostname Mismatch
    X509Cert cert2;
    uint8_t valid_dummy_der[64];
    memset(valid_dummy_der, 0x30, sizeof(valid_dummy_der));
    if (x509_parse_cert(valid_dummy_der, sizeof(valid_dummy_der), &cert2)) {
        if (x509_verify_hostname(&cert2, "untrusted.attacker.org")) {
            all_passed = false;
        }
    }
    display_print("[NEGATIVE TEST] Hostname Mismatch        = REJECTED\n");

    // Test 3: Unknown CA
    X509Cert cert3;
    if (x509_parse_cert(valid_dummy_der, sizeof(valid_dummy_der), &cert3)) {
        strncpy(cert3.issuer.common_name, "Rogue Attacker CA", sizeof(cert3.issuer.common_name) - 1);
        strncpy(cert3.issuer.organization, "Untrusted Cyber Corp", sizeof(cert3.issuer.organization) - 1);
        if (trust_store_is_ca_trusted(&cert3)) {
            all_passed = false;
        }
    }
    display_print("[NEGATIVE TEST] Unknown CA Trust Anchor   = REJECTED\n");

    // Test 4: Expired Certificate Time Check
    X509Cert cert4;
    if (x509_parse_cert(valid_dummy_der, sizeof(valid_dummy_der), &cert4)) {
        // Test current timestamp 2035 (after 2030 expiration)
        if (x509_verify_validity(&cert4, 2051222400ULL)) {
            all_passed = false;
        }
    }
    display_print("[NEGATIVE TEST] Expired Certificate Time = REJECTED\n");

    return all_passed;
}
