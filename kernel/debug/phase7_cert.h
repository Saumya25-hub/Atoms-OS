#ifndef PHASE7_CERT_H
#define PHASE7_CERT_H

#include <stdint.h>
#include <stdbool.h>

// Phase 7 Production Certification Engine
// Runs all subsystem stress tests and generates pass/fail report

typedef struct {
    uint32_t tests_run;
    uint32_t tests_passed;
    uint32_t tests_failed;

    // Memory
    bool pmm_leak_test;
    bool heap_canary_test;
    bool heap_stress_test;
    bool vmm_mapping_test;

    // Scheduler
    bool scheduler_stress_test;
    bool context_switch_test;

    // Filesystem
    bool fat32_stress_test;
    bool vfs_handle_test;

    // Desktop
    bool window_stress_test;
    bool compositor_test;
} Phase7CertResult;

// Run all Phase 7 certification tests (called from kernel main loop on demand)
void phase7_cert_run_all(void);

// Individual test suites
void phase7_cert_memory(Phase7CertResult* result);
void phase7_cert_scheduler(Phase7CertResult* result);
void phase7_cert_filesystem(Phase7CertResult* result);
void phase7_cert_desktop(Phase7CertResult* result);

// PMM leak tracking (alloc vs free counter)
extern volatile uint64_t g_pmm_alloc_count;
extern volatile uint64_t g_pmm_free_count;

// Heap allocation tracking
extern volatile uint64_t g_heap_alloc_count;
extern volatile uint64_t g_heap_free_count;
extern volatile uint64_t g_heap_alloc_bytes;
extern volatile uint64_t g_heap_free_bytes;

// Scheduler tracking
extern volatile uint64_t g_sched_ctx_switches;
extern volatile uint64_t g_sched_task_creates;
extern volatile uint64_t g_sched_task_destroys;

// Exception tracking
extern volatile uint64_t g_exception_count[32];
extern volatile uint64_t g_irq_total_count;

// Print certification report to serial
void phase7_cert_print_report(Phase7CertResult* result);

#endif // PHASE7_CERT_H
