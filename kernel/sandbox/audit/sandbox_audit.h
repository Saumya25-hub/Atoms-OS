/*
 * BOS OS — Phase 4: Production Process Sandbox & Capability Engine
 * sandbox_audit.h — Audit Engine Subsystem Header
 */

#ifndef BOS_SANDBOX_AUDIT_H
#define BOS_SANDBOX_AUDIT_H

#include "kernel/sandbox/include/sandbox_types.h"

#ifdef __cplusplus
extern "C" {
#endif

bos_sandbox_status_t sandbox_audit_init(void);
bos_sandbox_status_t bos_audit_log_event(uint32_t context_id, bos_audit_event_type_t event_type, const char *description, int result);
uint32_t             sandbox_audit_get_count(void);
const bos_audit_record_t* sandbox_audit_get_record(uint32_t index);
void                 sandbox_audit_dump_recent(void);

#ifdef __cplusplus
}
#endif

#endif /* BOS_SANDBOX_AUDIT_H */
