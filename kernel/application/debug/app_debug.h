#ifndef ATOMS_APP_DEBUG_H
#define ATOMS_APP_DEBUG_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================
// ATOMS Native App Framework Fast Debug & Stress Suite
// ============================================================

typedef struct {
    uint32_t total_apps_registered;
    uint32_t active_apps_count;
    uint32_t total_launch_cycles;
    uint32_t total_events_processed;
    uint32_t total_timers_fired;
    uint32_t total_memory_leaks_detected;
} ATOMS_AppDebugMetrics;

void ATOMS_AppDebug_Init(void);
void ATOMS_AppDebug_Log(const char* tag, const char* message);
void ATOMS_AppDebug_GetMetrics(ATOMS_AppDebugMetrics* out_metrics);

// Stress & Leak Test Suite
bool ATOMS_AppDebug_Run100LaunchStressTest(void);
bool ATOMS_AppDebug_Run1000EventStressTest(void);
bool ATOMS_AppDebug_RunLeakDetectionAudit(void);

// Integrated Phase 9 Verification Suite Call
void ATOMS_RunPhase9_VerificationSuite(void);

#ifdef __cplusplus
}
#endif

#endif // ATOMS_APP_DEBUG_H
