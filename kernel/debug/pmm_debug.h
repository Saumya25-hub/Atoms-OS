#ifndef PMM_DEBUG_H
#define PMM_DEBUG_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "kernel/core/core_legacy/boot/include/boot_info.h"

#define PMM_DEBUG_MAX_CPUS 8U

typedef struct {
    // Hardware & Memory Inventory
    uint64_t total_memory_bytes;
    uint64_t usable_memory_bytes;
    uint64_t reserved_memory_bytes;
    uint64_t total_frames;
    uint64_t bitmap_address;
    uint64_t bitmap_size_bytes;

    // Live Page Counters
    uint64_t baseline_free_pages;
    uint64_t current_free_pages;
    uint64_t current_used_pages;
    uint64_t reserved_pages;

    // Telemetry Markers
    uint64_t last_alloc_phys;
    uint64_t last_free_phys;
    uint32_t alloc_failures;
    uint32_t invalid_frees;
    uint32_t double_frees;

    // Per-CPU Access Matrix
    uint64_t cpu_pmm_calls[PMM_DEBUG_MAX_CPUS];

    // Stress Benchmark Results
    struct {
        uint64_t alloc_1p_delta;
        uint64_t alloc_8p_delta;
        uint64_t alloc_16p_delta;
        uint64_t alloc_64p_delta;
        uint64_t alloc_256p_delta;
        uint64_t alloc_1024p_delta;
        bool     stress_1p_pass;
        bool     stress_8p_pass;
        bool     stress_16p_pass;
        bool     stress_64p_pass;
        bool     stress_256p_pass;
        bool     stress_1024p_pass;
    } stress;

    // Verdicts
    const char *concurrency_status_str;
    const char *pmm_verdict_str;
    const char *arch_verdict_str;
} PMMDebugStats;

void pmm_debug_init(boot_info_t *boot_info);
void pmm_debug_render(void);
void pmm_debug_run(boot_info_t *boot_info);

#endif /* PMM_DEBUG_H */
