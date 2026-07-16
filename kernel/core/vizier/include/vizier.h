#ifndef VIZIER_H
#define VIZIER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Subsystem Health States
typedef enum {
    VIZIER_HEALTH_OK = 0,
    VIZIER_HEALTH_DEGRADED,       // Non-fatal contract violation or missed QoS deadline
    VIZIER_HEALTH_FAILED,         // Fatal invariant failure or hardware halt
    VIZIER_HEALTH_OFFLINE         // Subsystem disabled by policy fallback
} VizierHealthStatus;

// Subsystem Lifecycle States
typedef enum {
    VIZIER_STATE_UNINITIALIZED = 0,
    VIZIER_STATE_REGISTERED,
    VIZIER_STATE_ACTIVE,
    VIZIER_STATE_SUSPENDED,
    VIZIER_STATE_FALLBACK
} VizierLifecycleState;

// Authoritative Capability Domains
typedef enum {
    VIZIER_CAP_INPUT_POINTER_RAW = (1 << 0),   // Authority over raw hardware mouse packets
    VIZIER_CAP_INPUT_NORMALIZED  = (1 << 1),   // Authority over canonical event queue
    VIZIER_CAP_COORD_TRANSFORM   = (1 << 2),   // Authority over screen/logical coordinate math
    VIZIER_CAP_STORAGE_IO        = (1 << 3),   // Authority over disk read/write arbitration
    VIZIER_CAP_DMA_ALLOC         = (1 << 4),   // Authority over coherent physical DMA buffers
    VIZIER_CAP_AUDIO_REALTIME    = (1 << 5),   // Authority over AC97 playback pump & timing
    VIZIER_CAP_PRESENTATION      = (1 << 6)    // Authority over VRAM surface flipping
} VizierCapability;

// Subsystem Contract Declaration
typedef struct {
    const char* subsystem_name;
    uint32_t    subsystem_id;
    uint32_t    capabilities_provided;         // Bitmask of VizierCapability
    uint32_t    capabilities_required;         // Bitmask of dependencies
    uint32_t    max_realtime_latency_us;       // Max expected execution time per pump/check
    uint32_t    qos_deadline_ms;               // Required preemption interval (0 if non-realtime)
    void        (*on_health_check)(void);      // Periodic non-blocking telemetry callback
    bool        (*on_fault_degrade)(void);     // Fallback handler triggered by Vizier on fault
} VizierContract;

// Subsystem Runtime Handle
typedef struct {
    VizierContract       contract;
    VizierLifecycleState state;
    VizierHealthStatus   health;
    uint64_t             last_success_ticks;
    uint64_t             last_failure_ticks;
    uint32_t             invariant_violations;
    uint32_t             deadline_misses;
    uint32_t             event_counter;
    uint32_t             dropped_event_counter;
    const char*          last_error_reason;
} VizierSubsystemNode;

// ============================================================================
// VIZIER PUBLIC GOVERNANCE API
// ============================================================================

// Initialization & Lifecycle
void vizier_init(void);
int  vizier_register_subsystem(const VizierContract* contract);
int  vizier_set_lifecycle_state(uint32_t subsystem_id, VizierLifecycleState state);

// Authoritative Owner Arbitration
int  vizier_claim_authority(uint32_t subsystem_id, VizierCapability capability);
uint32_t vizier_get_authoritative_owner(VizierCapability capability);

// Runtime Invariant & Health Monitoring (O(1) Lockless/Atomic Counters)
void vizier_record_heartbeat(uint32_t subsystem_id);
void vizier_record_event(uint32_t subsystem_id, uint32_t count);
void vizier_record_drop(uint32_t subsystem_id, uint32_t count);
void vizier_report_violation(uint32_t subsystem_id, const char* reason);
void vizier_check_deadlines_on_tick(uint64_t current_ticks); // Called once per 10ms by timer

// Diagnostic Snapshot
void vizier_dump_diagnostic_snapshot(void);

// Constants
#define VIZIER_SUBSYSTEM_INPUT_CORE     100
#define VIZIER_SUBSYSTEM_POINTER_ENGINE 101
#define VIZIER_SUBSYSTEM_BWE            102
#define VIZIER_SUBSYSTEM_AGDTE          103
#define VIZIER_SUBSYSTEM_BSPE           104
#define VIZIER_SUBSYSTEM_AUDIO          105
#define VIZIER_SUBSYSTEM_STORAGE        106

#endif // VIZIER_H
