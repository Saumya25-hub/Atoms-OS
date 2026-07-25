/*
 * BOS OS — Phase 4: Production Process Sandbox & Capability Engine
 * sandbox_audit.c — Audit Engine Subsystem Implementation
 */

#include "kernel/sandbox/audit/sandbox_audit.h"
#include "kernel/sandbox/debug/sandbox_debug.h"
#include "kernel/core/lib/include/string.h"

extern void display_print(const char* str);

static bos_audit_record_t g_audit_logs[BOS_MAX_AUDIT_LOGS];
static uint32_t g_audit_head = 0;
static uint32_t g_audit_total_records = 0;

bos_sandbox_status_t sandbox_audit_init(void) {
    memset(g_audit_logs, 0, sizeof(g_audit_logs));
    g_audit_head = 0;
    g_audit_total_records = 0;
    return BOS_SANDBOX_OK;
}

bos_sandbox_status_t bos_audit_log_event(uint32_t context_id, bos_audit_event_type_t event_type, const char *description, int result) {
    uint32_t idx = g_audit_head % BOS_MAX_AUDIT_LOGS;
    bos_audit_record_t *rec = &g_audit_logs[idx];

    rec->timestamp = 100000ULL + g_audit_total_records;
    rec->context_id = context_id;
    rec->pid = (context_id >= 1000) ? (context_id - 1000 + 1) : 0;
    rec->event_type = event_type;
    rec->result_code = result;

    if (description) {
        strncpy(rec->description, description, sizeof(rec->description) - 1);
        rec->description[sizeof(rec->description) - 1] = '\0';
    } else {
        rec->description[0] = '\0';
    }

    g_audit_head++;
    g_audit_total_records++;

    bos_sandbox_debug_log(BOS_TRACE_SANDBOX, "AUDIT EVENT [Type %u, Ctx %u]: %s (Res: %d)",
                          (unsigned)event_type, context_id, rec->description, result);

    return BOS_SANDBOX_OK;
}

uint32_t sandbox_audit_get_count(void) {
    return g_audit_total_records;
}

const bos_audit_record_t* sandbox_audit_get_record(uint32_t index) {
    if (index >= g_audit_total_records) return NULL;
    uint32_t idx = index % BOS_MAX_AUDIT_LOGS;
    return &g_audit_logs[idx];
}

void sandbox_audit_dump_recent(void) {
    display_print("[SANDBOX AUDIT LOG DUMP]\n");
    uint32_t count = g_audit_total_records > 10 ? 10 : g_audit_total_records;
    for (uint32_t i = 0; i < count; i++) {
        uint32_t idx = (g_audit_head - count + i) % BOS_MAX_AUDIT_LOGS;
        const bos_audit_record_t *rec = &g_audit_logs[idx];
        display_print(" - Event ");
        display_print(rec->description);
        display_print("\n");
    }
}
