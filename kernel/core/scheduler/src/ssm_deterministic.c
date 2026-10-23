#include "kernel/core/scheduler/include/ssm_deterministic.h"
#include "kernel/core/scheduler/include/scheduler.h"

static ATOMS_SSMTelemetry g_ssm_telemetry = {
    .total_evaluations = 0,
    .concordant_decisions = 0,
    .discordant_decisions = 0,
    .fallback_activations = 0,
    .last_eval_latency_cycles = 0,
    .last_selected_score = 0,
    .current_mode = SSM_MODE_SHADOW // Default to Shadow Mode for safe verification
};

static inline uint64_t ssm_rdtsc(void) {
    uint32_t lo, hi;
    __asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}

void ssm_init(ATOMS_SSMMode initial_mode) {
    g_ssm_telemetry.total_evaluations = 0;
    g_ssm_telemetry.concordant_decisions = 0;
    g_ssm_telemetry.discordant_decisions = 0;
    g_ssm_telemetry.fallback_activations = 0;
    g_ssm_telemetry.last_eval_latency_cycles = 0;
    g_ssm_telemetry.last_selected_score = 0;
    g_ssm_telemetry.current_mode = initial_mode;
}

void ssm_set_mode(ATOMS_SSMMode mode) {
    g_ssm_telemetry.current_mode = mode;
}

ATOMS_SSMMode ssm_get_mode(void) {
    return g_ssm_telemetry.current_mode;
}

const ATOMS_SSMTelemetry *ssm_get_telemetry(void) {
    return &g_ssm_telemetry;
}

int32_t ssm_score_candidate(const Task *task, uint32_t current_cpu, uint64_t now_tick) {
    // Invariant 1: Eligibility check (Hard Precondition)
    if (!task) return SSM_INT32_MIN;
    if (task->state != TASK_READY) return SSM_INT32_MIN;
    if (current_cpu >= 64U) return SSM_INT32_MIN;
    if (!(task->affinity_mask & (1ULL << current_cpu))) return SSM_INT32_MIN;
    if (task->assigned_cpu != ATOMS_CPU_NONE && task->assigned_cpu != current_cpu) return SSM_INT32_MIN;

    // Component 1: Base Priority (Q16.16)
    uint8_t prio = task->base_priority <= 31U ? task->base_priority : 31U;
    int32_t s_prio = ssm_to_fixed((int32_t)prio);

    // Component 2: Aging & Starvation Mitigation (Q16.16)
    uint64_t waited = now_tick > task->ready_since_tick ? (now_tick - task->ready_since_tick) : 0;
    uint32_t boost = (uint32_t)(waited / SCHEDULER_AGING_INTERVAL_TICKS);
    uint32_t max_boost = 31U > prio ? (31U - prio) : 0;
    if (boost > max_boost) boost = max_boost;
    int32_t s_aging = ssm_to_fixed((int32_t)boost);

    // Component 3: Cache Locality Bonus vs Cross-Core Migration Penalty
    int32_t s_locality = 0;
    if (task->last_cpu == current_cpu) {
        s_locality = ssm_to_fixed(2); // +2.0 cache warm affinity
    } else if (task->last_cpu != ATOMS_CPU_NONE) {
        s_locality = -ssm_fixed_mul_sat(ssm_to_fixed(1), 0x00018000); // -1.5 cross-core migration penalty
    }

    // Component 4: Remaining Quantum Incentive
    int32_t q = task->quantum;
    if (q < 1) q = 1;
    if (q > 20) q = 20;
    int32_t s_quantum = (q * (SSM_FIXED_ONE / 2)) / 20; // 0.0 to 0.5 incentive

    // Saturated Total Score
    int32_t score = ssm_fixed_add_sat(s_prio, s_aging);
    score = ssm_fixed_add_sat(score, s_locality);
    score = ssm_fixed_add_sat(score, s_quantum);

    return score;
}

Task *ssm_select_best_task(Task **candidates, uint32_t candidate_count, uint32_t current_cpu, uint64_t now_tick) {
    if (!candidates || candidate_count == 0) {
        g_ssm_telemetry.fallback_activations++;
        return 0;
    }

    uint64_t start_cycles = ssm_rdtsc();
    uint32_t eval_limit = candidate_count < SSM_MAX_CANDIDATES ? candidate_count : SSM_MAX_CANDIDATES;

    Task *best_task = 0;
    int32_t best_score = SSM_INT32_MIN;

    for (uint32_t i = 0; i < eval_limit; ++i) {
        Task *candidate = candidates[i];
        if (!candidate) continue;

        int32_t score = ssm_score_candidate(candidate, current_cpu, now_tick);
        if (score == SSM_INT32_MIN) continue; // Ineligible

        if (!best_task || score > best_score) {
            best_task = candidate;
            best_score = score;
        } else if (score == best_score) {
            // Deterministic Tie-Breaking Invariants
            if (candidate->ready_since_tick < best_task->ready_since_tick) {
                best_task = candidate;
                best_score = score;
            } else if (candidate->ready_since_tick == best_task->ready_since_tick && candidate->id < best_task->id) {
                best_task = candidate;
                best_score = score;
            }
        }
    }

    uint64_t end_cycles = ssm_rdtsc();
    g_ssm_telemetry.total_evaluations++;
    g_ssm_telemetry.last_eval_latency_cycles = end_cycles - start_cycles;
    g_ssm_telemetry.last_selected_score = best_score;

    return best_task;
}
