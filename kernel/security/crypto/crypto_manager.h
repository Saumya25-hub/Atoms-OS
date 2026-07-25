/*
 * BOS OS — Phase 3: Production TLS & Security Engine
 * crypto_manager.h — Cryptographic Subsystem Interface & HMAC Engine
 */

#ifndef CRYPTO_MANAGER_H
#define CRYPTO_MANAGER_H

#include "kernel/security/include/bos_crypto.h"

bos_sec_status_t crypto_manager_init(void);

#endif /* CRYPTO_MANAGER_H */
