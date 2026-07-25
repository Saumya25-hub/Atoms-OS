/*
 * BOS OS — Phase 3: Production TLS & Security Engine
 * bos_x509.h — X.509 Certificate Parser & Public Key Infrastructure API
 */

#ifndef BOS_X509_H
#define BOS_X509_H

#include "kernel/security/include/bos_security_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Parses a DER or PEM encoded X.509 certificate.
 * @param buffer Pointer to raw bytes (PEM string or DER binary).
 * @param len Length of buffer.
 * @param cert Pointer to output certificate structure.
 * @return BOS_SEC_OK on successful parsing.
 */
bos_sec_status_t bos_cert_parse(const uint8_t* buffer, size_t len, bos_x509_cert_t* cert);

/**
 * @brief Frees resources attached to an X.509 certificate structure.
 */
void bos_cert_free(bos_x509_cert_t* cert);

#ifdef __cplusplus
}
#endif

#endif /* BOS_X509_H */
