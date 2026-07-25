/*
 * BOS OS — Phase 3: Production TLS & Security Engine
 * bos_security.h — Master Subsystem Public Facade API
 */

#ifndef BOS_SECURITY_H
#define BOS_SECURITY_H

#include "kernel/security/include/bos_security_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initializes the complete BOS OS Security & TLS Subsystem.
 * 
 * Order of internal engine initialization:
 * 1. Debug & Diagnostics Engine
 * 2. CSPRNG Random Engine
 * 3. Hash Engine (SHA-256/384/512)
 * 4. AES Engine
 * 5. RSA Engine
 * 6. ECC Engine
 * 7. X.509 Certificate Parser
 * 8. Certificate Validator & Trust Store Manager
 * 9. TLS & Session Engine
 * 
 * @return BOS_SEC_OK on success or structured error code.
 */
bos_sec_status_t bos_security_init(void);

/**
 * @brief Checks if the security subsystem is initialized.
 * @return true if initialized, false otherwise.
 */
bool bos_security_is_initialized(void);

/**
 * @brief Executes the complete 30-item Automated Certification Test Suite inside kernel runtime.
 * @return BOS_SEC_OK if all certification tests pass, error code otherwise.
 */
bos_sec_status_t bos_security_run_certification_tests(void);

#ifdef __cplusplus
}
#endif

#endif /* BOS_SECURITY_H */
