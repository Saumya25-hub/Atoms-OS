/*
 * BOS OS — Phase 3: Production TLS & Security Engine
 * bos_trust.h — Certificate Validator & Trust Store Manager API
 */

#ifndef BOS_TRUST_H
#define BOS_TRUST_H

#include "kernel/security/include/bos_security_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initializes the Trust Store Manager with trusted Root CAs.
 */
bos_sec_status_t bos_trust_store_init(void);

/**
 * @brief Adds a Root Certificate to the Trust Store.
 */
bos_sec_status_t bos_trust_store_add_root(const bos_x509_cert_t* root_cert);

/**
 * @brief Looks up a certificate in the Trust Store by subject or fingerprint.
 */
bool bos_trust_store_lookup(const uint8_t sha256_fingerprint[32], bos_x509_cert_t* out_cert);

/**
 * @brief Validates an X.509 certificate chain, expiration, signature, and domain match.
 * @param cert Target end-entity certificate.
 * @param chain Optional intermediate CA certificate array.
 * @param chain_count Count of intermediate certificates.
 * @param expected_domain Hostname string to match against SAN / CN.
 * @return BOS_SEC_OK on full validation, or explicit failure error code.
 */
bos_sec_status_t bos_cert_verify(const bos_x509_cert_t* cert,
                                const bos_x509_cert_t* chain,
                                size_t chain_count,
                                const char* expected_domain);

#ifdef __cplusplus
}
#endif

#endif /* BOS_TRUST_H */
