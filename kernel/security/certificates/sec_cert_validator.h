/*
 * BOS OS — Phase 3: Production TLS & Security Engine
 * sec_cert_validator.h — Internal Certificate Validator Interface
 */

#ifndef SEC_CERT_VALIDATOR_H
#define SEC_CERT_VALIDATOR_H

#include "kernel/security/include/bos_trust.h"

bos_sec_status_t sec_cert_validator_init(void);

#endif /* SEC_CERT_VALIDATOR_H */
