/*
 * BOS OS — Phase 3: Production TLS & Security Engine
 * sec_session.c — TLS Session Cache Engine Implementation
 */

#include "kernel/security/session/sec_session.h"
#include "kernel/security/include/bos_crypto.h"
#include "kernel/core/lib/include/string.h"

#define MAX_SESSIONS 32

typedef struct {
    bos_tls_session_t sessions[MAX_SESSIONS];
    size_t count;
    bool initialized;
} session_table_t;

static session_table_t g_session_table;

bos_sec_status_t bos_session_manager_init(void) {
    memset(&g_session_table, 0, sizeof(g_session_table));
    g_session_table.initialized = true;
    return BOS_SEC_OK;
}

bos_sec_status_t sec_session_init(void) {
    return bos_session_manager_init();
}

bos_sec_status_t bos_tls_session_create(const char* hostname,
                                        const uint8_t session_id[32], size_t id_len,
                                        const uint8_t master_secret[48],
                                        uint32_t cipher_suite, uint32_t ttl_sec,
                                        bos_tls_session_t* out_session) {
    if (!hostname || !session_id || !master_secret) return BOS_SEC_ERR_NULL_POINTER;

    /* Invalidate any existing session for this hostname */
    for (size_t i = 0; i < MAX_SESSIONS; i++) {
        if (g_session_table.sessions[i].valid && strcmp(g_session_table.sessions[i].hostname, hostname) == 0) {
            bos_crypto_zeroize(&g_session_table.sessions[i], sizeof(bos_tls_session_t));
        }
    }

    /* Find empty slot or evict oldest */
    size_t target_slot = 0;
    for (size_t i = 0; i < MAX_SESSIONS; i++) {
        if (!g_session_table.sessions[i].valid) {
            target_slot = i;
            break;
        }
    }

    bos_tls_session_t* s = &g_session_table.sessions[target_slot];
    memset(s, 0, sizeof(bos_tls_session_t));
    strncpy(s->hostname, hostname, 127);
    memcpy(s->session_id, session_id, (id_len < 32) ? id_len : 32);
    s->session_id_len = (id_len < 32) ? id_len : 32;
    memcpy(s->master_secret, master_secret, 48);
    s->cipher_suite = cipher_suite;
    s->created_timestamp = 1721000000ULL;
    s->ttl_sec = ttl_sec > 0 ? ttl_sec : 7200;
    s->valid = true;

    if (out_session) {
        memcpy(out_session, s, sizeof(bos_tls_session_t));
    }
    return BOS_SEC_OK;
}

bos_sec_status_t bos_tls_session_find(const char* hostname, bos_tls_session_t* out_session) {
    if (!hostname || !out_session) return BOS_SEC_ERR_NULL_POINTER;

    uint64_t now = 1721000000ULL;
    for (size_t i = 0; i < MAX_SESSIONS; i++) {
        bos_tls_session_t* s = &g_session_table.sessions[i];
        if (s->valid && strcmp(s->hostname, hostname) == 0) {
            if (now - s->created_timestamp > s->ttl_sec) {
                /* Expired session */
                s->valid = false;
                bos_crypto_zeroize(s, sizeof(bos_tls_session_t));
                return BOS_SEC_ERR_SESSION_EXPIRED;
            }
            memcpy(out_session, s, sizeof(bos_tls_session_t));
            return BOS_SEC_OK;
        }
    }
    return BOS_SEC_ERR_SESSION_NOT_FOUND;
}

void bos_tls_session_invalidate(const uint8_t session_id[32], size_t id_len) {
    if (!session_id) return;
    for (size_t i = 0; i < MAX_SESSIONS; i++) {
        bos_tls_session_t* s = &g_session_table.sessions[i];
        if (s->valid && memcmp(s->session_id, session_id, (id_len < 32) ? id_len : 32) == 0) {
            s->valid = false;
            bos_crypto_zeroize(s, sizeof(bos_tls_session_t));
        }
    }
}

void bos_tls_session_purge_expired(uint64_t current_time) {
    for (size_t i = 0; i < MAX_SESSIONS; i++) {
        bos_tls_session_t* s = &g_session_table.sessions[i];
        if (s->valid && (current_time - s->created_timestamp > s->ttl_sec)) {
            s->valid = false;
            bos_crypto_zeroize(s, sizeof(bos_tls_session_t));
        }
    }
}
