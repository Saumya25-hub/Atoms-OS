/*
 * BOS OS — Phase 3: Production TLS & Security Engine
 * sec_random.h — Cryptographically Secure Random Engine Interface
 */

#ifndef SEC_RANDOM_H
#define SEC_RANDOM_H

#include "kernel/security/include/bos_random.h"

bos_sec_status_t sec_random_init(void);

#endif /* SEC_RANDOM_H */
