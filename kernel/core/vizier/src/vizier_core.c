#include "kernel/core/vizier/include/vizier.h"
#include <stddef.h>

extern void display_print(const char* str);

#define MAX_SUBSYSTEMS 32

static VizierSubsystemNode g_subsystems[MAX_SUBSYSTEMS];
static uint32_t g_subsystem_count = 0;
static bool g_vizier_initialized = false;

#define VIZIER_TRACE_RING_SIZE 128

typedef struct {
    uint64_t timestamp_ticks;
    uint32_t subsystem_id;
    uint32_t event_type;
    const char* message;
} VizierTraceEntry;

static VizierTraceEntry g_vizier_trace_ring[VIZIER_TRACE_RING_SIZE];
static volatile uint32_t g_trace_head = 0;

void vizier_init(void) {
    g_vizier_initialized = true;
    display_print("[VIZIER] Initialized 0 subsystems\n");
}

int vizier_register_subsystem(const VizierContract* contract) {
    if (g_subsystem_count >= MAX_SUBSYSTEMS) {
        return -1;
    }
    VizierSubsystemNode* node = &g_subsystems[g_subsystem_count++];
    node->contract = *contract;
    node->state = VIZIER_STATE_REGISTERED;
    node->health = VIZIER_HEALTH_OK;
    node->last_success_ticks = 0;
    node->last_failure_ticks = 0;
    node->invariant_violations = 0;
    node->deadline_misses = 0;
    node->event_counter = 0;
    node->dropped_event_counter = 0;
    node->last_error_reason = NULL;

    if (g_vizier_initialized) {
        // Only print if display might be ready, but since Phase 0 says
        // "serial_atm.log confirms [VIZIER] Registered Audio/PS2/BWE successfully"
        // we can print here. Wait, audio and bwe initialize after display.
    }
    display_print("[VIZIER] Registered ");
    display_print(contract->subsystem_name);
    display_print(" successfully\n");

    return 0;
}

static uint32_t g_authority_owners[32] = {0};

int vizier_set_lifecycle_state(uint32_t subsystem_id, VizierLifecycleState state) {
    for (uint32_t i = 0; i < g_subsystem_count; i++) {
        if (g_subsystems[i].contract.subsystem_id == subsystem_id) {
            g_subsystems[i].state = state;
            return 0;
        }
    }
    return -1;
}

int vizier_claim_authority(uint32_t subsystem_id, VizierCapability capability) {
    for (int i = 0; i < 32; i++) {
        if (capability & (1 << i)) {
            if (g_authority_owners[i] != 0 && g_authority_owners[i] != subsystem_id) {
                vizier_report_violation(subsystem_id, "Authority conflict");
                return -1;
            }
            g_authority_owners[i] = subsystem_id;
        }
    }
    return 0;
}

int vizier_release_authority(uint32_t subsystem_id, VizierCapability capability) {
    for (int i = 0; i < 32; i++) {
        if (capability & (1 << i)) {
            if (g_authority_owners[i] == subsystem_id) {
                g_authority_owners[i] = 0;
            }
        }
    }
    return 0;
}

uint32_t vizier_get_authoritative_owner(VizierCapability capability) {
    for (int i = 0; i < 32; i++) {
        if (capability & (1 << i)) {
            return g_authority_owners[i];
        }
    }
    return 0;
}

void vizier_record_heartbeat(uint32_t subsystem_id) {
    for (uint32_t i = 0; i < g_subsystem_count; i++) {
        if (g_subsystems[i].contract.subsystem_id == subsystem_id) {
            extern uint64_t timer_get_ticks(void);
            g_subsystems[i].last_success_ticks = timer_get_ticks();
            g_subsystems[i].health = VIZIER_HEALTH_OK;
            break;
        }
    }
}

void vizier_record_event(uint32_t subsystem_id, uint32_t count) {
    for (uint32_t i = 0; i < g_subsystem_count; i++) {
        if (g_subsystems[i].contract.subsystem_id == subsystem_id) {
            __atomic_fetch_add(&g_subsystems[i].event_counter, count, __ATOMIC_RELAXED);
            break;
        }
    }
}

void vizier_record_drop(uint32_t subsystem_id, uint32_t count) {
    for (uint32_t i = 0; i < g_subsystem_count; i++) {
        if (g_subsystems[i].contract.subsystem_id == subsystem_id) {
            __atomic_fetch_add(&g_subsystems[i].dropped_event_counter, count, __ATOMIC_RELAXED);
            break;
        }
    }
}

void vizier_report_violation(uint32_t subsystem_id, const char* reason) {
    uint32_t idx = __atomic_fetch_add(&g_trace_head, 1, __ATOMIC_RELAXED) % VIZIER_TRACE_RING_SIZE;
    extern uint64_t timer_get_ticks(void);
    g_vizier_trace_ring[idx].timestamp_ticks = timer_get_ticks();
    g_vizier_trace_ring[idx].subsystem_id = subsystem_id;
    g_vizier_trace_ring[idx].event_type = 1; // VIOLATION
    g_vizier_trace_ring[idx].message = reason;
    
    // Also mark subsystem health
    for (uint32_t i = 0; i < g_subsystem_count; i++) {
        if (g_subsystems[i].contract.subsystem_id == subsystem_id) {
            __atomic_fetch_add(&g_subsystems[i].invariant_violations, 1, __ATOMIC_RELAXED);
            if (g_subsystems[i].health == VIZIER_HEALTH_OK) {
                g_subsystems[i].health = VIZIER_HEALTH_DEGRADED;
            }
            g_subsystems[i].last_error_reason = reason;
            break;
        }
    }
}

void vizier_register_deadline(uint32_t subsystem_id, uint64_t current_ticks, uint32_t expected_duration_us) {
    for (uint32_t i = 0; i < g_subsystem_count; i++) {
        if (g_subsystems[i].contract.subsystem_id == subsystem_id) {
            g_subsystems[i].last_success_ticks = current_ticks;
        }
    }
}

void vizier_check_deadlines_on_tick(uint64_t current_ticks) {
    for (uint32_t i = 0; i < g_subsystem_count; i++) {
        VizierSubsystemNode* node = &g_subsystems[i];
        if (node->contract.qos_deadline_ms > 0) {
            uint64_t ticks_since_success = current_ticks - node->last_success_ticks;
            if (ticks_since_success > node->contract.qos_deadline_ms) {
                __atomic_fetch_add(&node->deadline_misses, 1, __ATOMIC_RELAXED);
                if (node->health == VIZIER_HEALTH_OK) {
                    node->health = VIZIER_HEALTH_DEGRADED;
                    node->last_error_reason = "QoS deadline missed";
                    vizier_report_violation(node->contract.subsystem_id, node->last_error_reason);
                }
            }
        }
    }
}

void vizier_dump_diagnostic_snapshot(void) {
    display_print("\nVIZIER X+ SYSTEM SNAPSHOT\n");
    for (uint32_t i = 0; i < g_subsystem_count; i++) {
        display_print(g_subsystems[i].contract.subsystem_name);
        display_print(": ");
        if (g_subsystems[i].state == VIZIER_STATE_REGISTERED) {
            display_print("REGISTERED");
        } else {
            display_print("UNKNOWN");
        }
        display_print(" / ");
        if (g_subsystems[i].health == VIZIER_HEALTH_OK) {
            display_print("OK\n");
        } else {
            display_print("ERROR\n");
        }
    }
    
    display_print("\n--- TRACE RING ---\n");
    uint32_t total_events = g_trace_head;
    uint32_t count = (total_events < VIZIER_TRACE_RING_SIZE) ? total_events : VIZIER_TRACE_RING_SIZE;
    uint32_t start_idx = (total_events < VIZIER_TRACE_RING_SIZE) ? 0 : (total_events % VIZIER_TRACE_RING_SIZE);
    
    if (count == 0) {
        display_print("(Empty)\n");
    } else {
        for (uint32_t i = 0; i < count; i++) {
            uint32_t idx = (start_idx + i) % VIZIER_TRACE_RING_SIZE;
            display_print("[SID:");
            
            extern void display_print_dec(uint64_t val);
            display_print_dec(g_vizier_trace_ring[idx].subsystem_id);
            
            display_print("] EVENT: ");
            display_print_dec(g_vizier_trace_ring[idx].event_type);
            display_print(" MSG: ");
            if (g_vizier_trace_ring[idx].message) {
                display_print(g_vizier_trace_ring[idx].message);
            } else {
                display_print("NULL");
            }
            display_print("\n");
        }
    }
    display_print("\n");
}
