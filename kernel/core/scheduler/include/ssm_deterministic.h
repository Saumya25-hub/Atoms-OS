#ifndef ATOMS_SSM_DETERMINISTIC_H
#define ATOMS_SSM_DETERMINISTIC_H

#include <stdint.h>
#include <stdbool.h>
#include "kernel/core/scheduler/include/task.h"

#define SSM_FIXED_SHIFT 16
#define SSM_FIXED_ONE   (1 << SSM_FIXED_SHIFT)
#define SSM_INT32_MAX   0x7FFFFFFF
#define SSM_INT32_MIN   ((int32_t)0x80000000)

#define SSM_MAX_CANDIDATES 32U

typedef enum {
    SSM_MODE_DISABLED = 0,    // Pure baseline scheduler (Round-Robin / Aging)
    SSM_MODE_SHADOW   = 1,    // SSM calculates score in background; baseline acts
    SSM_MODE_ACTIVE   = 2     // SSM decisions actively placed and dispatched
} ATOMS_SSMMode;

typedef struct {
    uint64_t total_evaluations;
    uint64_t concordant_decisions;
    uint64_t discordant_decisions;
    uint64_t fallback_activations;
    uint64_t last_eval_latency_cycles;
    int32_t  last_selected_score;
    ATOMS_SSMMode current_mode;
} ATOMS_SSMTelemetry;

/* Pure Reentrant Fixed-Point Math Functions (Q16.16) */
static inline int32_t ssm_to_fixed(int32_t val) {
    return val << SSM_FIXED_SHIFT;
}

static inline int32_t ssm_fixed_add_sat(int32_t a, int32_t b) {
    int64_t res = (int64_t)a + (int64_t)b;
    if (res > SSM_INT32_MAX) return SSM_INT32_MAX;
    if (res < SSM_INT32_MIN) return SSM_INT32_MIN;
    return (int32_t)res;
}

static inline int32_t ssm_fixed_sub_sat(int32_t a, int32_t b) {
    int64_t res = (int64_t)a - (int64_t)b;
    if (res > SSM_INT32_MAX) return SSM_INT32_MAX;
    if (res < SSM_INT32_MIN) return SSM_INT32_MIN;
    return (int32_t)res;
}

static inline int32_t ssm_fixed_mul_sat(int32_t a, int32_t b) {
    int64_t res = ((int64_t)a * (int64_t)b) >> SSM_FIXED_SHIFT;
    if (res > SSM_INT32_MAX) return SSM_INT32_MAX;
    if (res < SSM_INT32_MIN) return SSM_INT32_MIN;
    return (int32_t)res;
}

/* Engine Public API */
void ssm_init(ATOMS_SSMMode initial_mode);
void ssm_set_mode(ATOMS_SSMMode mode);
ATOMS_SSMMode ssm_get_mode(void);
const ATOMS_SSMTelemetry *ssm_get_telemetry(void);

/* Deterministic Candidate Evaluation */
int32_t ssm_score_candidate(const Task *task, uint32_t current_cpu, uint64_t now_tick);
Task *ssm_select_best_task(Task **candidates, uint32_t candidate_count, uint32_t current_cpu, uint64_t now_tick);

#endif // ATOMS_SSM_DETERMINISTIC_H
