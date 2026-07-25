/*
 * BOS OS — Phase 3: Production TLS & Security Engine
 * bos_session.h — TLS Session Engine & Reuse Cache API
 */

#ifndef BOS_SESSION_H
#define BOS_SESSION_H

#include "kernel/security/include/bos_security_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char     hostname[128];
    uint8_t  session_id[32];
    size_t   session_id_len;
    uint8_t  master_secret[48];
    uint32_t cipher_suite;
    uint64_t created_timestamp;
    uint32_t ttl_sec;
    bool     valid;
} bos_tls_session_t;

/**
 * @brief Initializes TLS session management.
 */
bos_sec_status_t bos_session_manager_init(void);

/**
 * @brief Creates and caches a new TLS session.
 */
bos_sec_status_t bos_tls_session_create(const char* hostname,
                                        const uint8_t session_id[32], size_t id_len,
                                        const uint8_t master_secret[48],
                                        uint32_t cipher_suite, uint32_t ttl_sec,
                                        bos_tls_session_t* out_session);

/**
 * @brief Finds a cached valid TLS session by hostname.
 */
bos_sec_status_t bos_tls_session_find(const char* hostname, bos_tls_session_t* out_session);

/**
 * @brief Invalidates a session by Session ID.
 */
void bos_tls_session_invalidate(const uint8_t session_id[32], size_t id_len);

/**
 * @brief Purges expired sessions from cache.
 */
void bos_tls_session_purge_expired(uint64_t current_time);

#ifdef __cplusplus
}
#endif

#endif /* BOS_SESSION_H */
