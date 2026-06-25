#include "../test_framework.h"
#include "../../bishopmath/include/bishop_math.h"

// Direct syscall to get uptime ticks (assuming SYS_UPTIME is 3)
static uint64_t get_uptime(void) {
    uint64_t ticks;
    __asm__ volatile (
        "syscall"
        : "=a"(ticks)
        : "a"(3) // SYS_UPTIME
        : "rcx", "r11", "memory"
    );
    return ticks;
}

static void print_time(uint64_t ticks) {
    char buf[20]; int i = 18; buf[19] = '\0';
    if (ticks == 0) { buf[i--] = '0'; }
    else { while (ticks > 0) { buf[i--] = (ticks % 10) + '0'; ticks /= 10; } }
    bos_print(&buf[i+1]);
    bos_print(" ticks\n");
}

void test_performance_suite(void) {
    TEST_SUITE_START("Performance Benchmarks");

    // We don't fail based on performance, we just log it.
    
    // 1. Math benchmark
    uint64_t start = get_uptime();
    for (int i = 0; i < 10000; i++) {
        double out;
        bishop_sin(1.0, &out);
        bishop_sqrt(144.0, &out);
    }
    uint64_t end = get_uptime();
    
    bos_print("  [PERF] 10K Math Operations : ");
    print_time(end - start);
    
    // 2. Heap benchmark
    start = get_uptime();
    for (int i = 0; i < 10000; i++) {
        // Just calling yield to benchmark syscall overhead
        bos_yield();
    }
    end = get_uptime();
    
    bos_print("  [PERF] 10K Syscalls (Yield): ");
    print_time(end - start);
    
    TEST_PASS("Performance Logging Completed");
}
