/*
 * BOS OS — Phase 3: Production TLS & Security Engine
 * bos_sec_debug.h — Isolated Security Debugging & Tracing Engine API
 */

#ifndef BOS_SEC_DEBUG_H
#define BOS_SEC_DEBUG_H

#include "kernel/security/include/bos_security_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BOS_SEC_TRACE_NONE        0x00000000
#define BOS_SEC_TRACE_TLS         0x00000001
#define BOS_SEC_TRACE_HANDSHAKE   0x00000002
#define BOS_SEC_TRACE_CERT        0x00000004
#define BOS_SEC_TRACE_CRYPTO      0x00000008
#define BOS_SEC_TRACE_RANDOM      0x00000010
#define BOS_SEC_TRACE_SESSION     0x00000020
#define BOS_SEC_TRACE_TIMING      0x00000040
#define BOS_SEC_TRACE_ALL         0xFFFFFFFF

/**
 * @brief Sets active debug trace flags.
 */
void sec_debug_set_trace_mask(uint32_t mask);

/**
 * @brief Gets current active debug trace flags.
 */
uint32_t sec_debug_get_trace_mask(void);

/**
 * @brief Logs debug message if target flag is enabled in trace mask.
 */
void sec_debug_log(uint32_t flag, const char* module, const char* fmt, ...);

#ifdef __cplusplus
}
#endif

#endif /* BOS_SEC_DEBUG_H */
