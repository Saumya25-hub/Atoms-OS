/**
 * @file profiler_tests.c
 * @brief Performance Profiler Engine Self-Verification Unit Tests
 */

#include "../include/profiler.h"
#include "kernel/drivers/display/display.h"

#if BOS_ENABLE_PROFILER

bool bos_profiler_run_unit_tests(void) {
    display_print("[PPE_TEST] Running Performance Profiler Engine Unit Tests...\n");
    
    // Test 1: RDTSC monotonicity
    uint64_t t1 = bos_profiler_rdtsc();
    for (volatile int i = 0; i < 10000; i++);
    uint64_t t2 = bos_profiler_rdtsc();
    
    if (t2 <= t1) {
        display_print("[PPE_TEST] FAIL: RDTSC non-monotonic!\n");
        return false;
    }
    
    // Test 2: Hotspot recording
    BOS_ProfScopeToken tok = bos_profiler_scope_enter("unit_test_synthetic_hotspot");
    for (volatile int j = 0; j < 50000; j++);
    bos_profiler_scope_exit(&tok);
    
    BOS_ProfStats stats;
    bos_profiler_get_stats(&stats);
    
    bool found_hotspot = false;
    for (uint32_t k = 0; k < stats.hotspot_count; k++) {
        if (stats.top_hotspots[k].func_name && 
            stats.top_hotspots[k].func_name[0] == 'u') {
            found_hotspot = true;
            break;
        }
    }
    
    if (!found_hotspot) {
        display_print("[PPE_TEST] FAIL: Hotspot tracker failed to capture scope!\n");
        return false;
    }
    
    display_print("[PPE_TEST] PASS: All Performance Profiler Unit Tests Passed Succeeded.\n");
    return true;
}

#endif /* BOS_ENABLE_PROFILER */
