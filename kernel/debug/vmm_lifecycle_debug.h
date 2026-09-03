#ifndef VMM_LIFECYCLE_DEBUG_H
#define VMM_LIFECYCLE_DEBUG_H

#include <stdint.h>
#include <stdbool.h>
#include "kernel/core/core_legacy/boot/include/boot_info.h"

/*
 * ============================================================================
 * ATOMS OS — VMM MEMORY LIFECYCLE FORENSICS DASHBOARD
 * TEMPORARY FORENSIC DEBUG BUILD (DESKTOP COMPLETELY BYPASSED)
 * ============================================================================
 */

typedef struct {
    char     cpu_name[48];
    uint32_t cpu_count;
    uint32_t current_cpu;
    uint64_t active_cr3;

    uint64_t total_pages;
    uint64_t pmm_free_pages;
    uint64_t pmm_used_pages;

    uint64_t baseline_free_pages;
    uint64_t baseline_used_pages;

    uint32_t test_pid;
    const char *test_state_str;

    uint64_t pml4_phys;
    uint64_t pdpt_phys;
    uint64_t pd1_phys;
    uint32_t pt_count;
    uint32_t user_pages_mapped;

    uint64_t before_create_free;
    uint64_t after_create_free;
    uint64_t after_terminate_free;
    uint64_t after_reap_free;

    bool pml4_freed;
    bool pdpt_freed;
    bool pd_freed;
    uint32_t pts_freed;
    uint32_t user_pages_freed;

    int64_t net_page_diff_1_cycle;
    int64_t net_page_diff_10_cycles;
    int64_t net_page_diff_50_cycles;
    int64_t net_page_diff_100_cycles;

    const char *vmm_verdict;
    bool test_completed;
} vmm_lifecycle_stats_t;

extern vmm_lifecycle_stats_t g_vmm_debug_stats;

void vmm_lifecycle_debug_init(boot_info_t *boot_info);
void vmm_lifecycle_debug_run(boot_info_t *boot_info);
void vmm_lifecycle_debug_render(void);

#endif // VMM_LIFECYCLE_DEBUG_H
