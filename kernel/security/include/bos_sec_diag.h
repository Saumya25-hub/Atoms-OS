/*
 * BOS OS — Phase 3: Production TLS & Security Engine
 * bos_sec_diag.h — Security Diagnostics & Metrics Engine API
 */

#ifndef BOS_SEC_DIAG_H
#define BOS_SEC_DIAG_H

#include "kernel/security/include/bos_security_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint64_t total_handshakes;
    uint64_t total_handshake_us;
    uint64_t successful_handshakes;
    uint64_t failed_handshakes;
    uint64_t session_reuses;
    uint64_t certs_parsed;
    uint64_t certs_verified;
    uint64_t cert_failures;
    uint64_t crypto_ops_count;
    uint64_t bytes_encrypted;
    uint64_t bytes_decrypted;
} sec_diag_metrics_t;

/**
 * @brief Resets diagnostic metrics.
 */
void sec_diag_reset(void);

/**
 * @brief Records completed TLS handshake telemetry.
 */
void sec_diag_record_handshake(uint64_t duration_us, bool success);

/**
 * @brief Records crypto operation telemetry.
 */
void sec_diag_record_crypto(size_t bytes, bool encrypt);

/**
 * @brief Obtains current diagnostic metrics snapshot.
 */
void sec_diag_get_metrics(sec_diag_metrics_t* metrics);

/**
 * @brief Prints diagnostic performance summary report.
 */
void sec_diag_dump_report(void);

#ifdef __cplusplus
}
#endif

#endif /* BOS_SEC_DIAG_H */
