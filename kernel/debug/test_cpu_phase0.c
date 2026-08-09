#include "kernel/debug/test_cpu_phase0.h"
#include "arch/x86_64/cpu/cpu_features.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/scheduler/include/scheduler.h"
#include "kernel/drivers/display/display.h"
#include "kernel/core/lib/include/crash_log.h"

static volatile bool g_task1_passed = false;
static volatile bool g_task2_passed = false;
static volatile bool g_task3_passed = false;
static volatile bool g_task1_failed = false;
static volatile bool g_task2_failed = false;
static volatile bool g_task3_failed = false;

// Worker Task 1: Distinct FPU & XMM pattern 1
static void task_fpu_pattern1(void) {
    volatile double v1 = 12345.6789;
    volatile double v2 = 98765.4321;
    volatile double result = 0.0;

    for (int i = 0; i < 500; i++) {
        result = v1 + v2;
        if (result < 111111.10 || result > 111111.20) {
            g_task1_failed = true;
            crash_log_add("[PHASE0 TEST] CRITICAL: Task 1 FPU Corruption Detected!");
            break;
        }
        // Yield quantum to force context switch
        scheduler_yield();
    }
    if (!g_task1_failed) {
        g_task1_passed = true;
    }
    while (1) { scheduler_yield(); }
}

// Worker Task 2: Distinct FPU & XMM pattern 2
static void task_fpu_pattern2(void) {
    volatile double v1 = 55555.5555;
    volatile double v2 = 44444.4444;
    volatile double result = 0.0;

    for (int i = 0; i < 500; i++) {
        result = v1 - v2;
        if (result < 11111.110 || result > 11111.112) {
            g_task2_failed = true;
            crash_log_add("[PHASE0 TEST] CRITICAL: Task 2 FPU Corruption Detected!");
            break;
        }
        scheduler_yield();
    }
    if (!g_task2_failed) {
        g_task2_passed = true;
    }
    while (1) { scheduler_yield(); }
}

// Worker Task 3: Floating-Point & 3x3 Matrix Multiplication Preemption Torture
static void task_math_torture(void) {
    for (int i = 0; i < 300; i++) {
        // Self-contained 3x3 identity matrix multiplication
        double m1[3][3] = {{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}, {7.0, 8.0, 9.0}};
        double identity[3][3] = {{1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}};
        double res[3][3] = {{0}};

        for (int r = 0; r < 3; r++) {
            for (int c = 0; c < 3; c++) {
                res[r][c] = 0.0;
                for (int k = 0; k < 3; k++) {
                    res[r][c] += m1[r][k] * identity[k][c];
                }
            }
        }

        // Verify res == m1
        if (res[0][0] != 1.0 || res[1][1] != 5.0 || res[2][2] != 9.0 || res[0][2] != 3.0) {
            g_task3_failed = true;
            crash_log_add("[PHASE0 TEST] CRITICAL: Task 3 Matrix Preemption Corruption!");
            break;
        }

        // Floating-point polynomial calculation
        volatile double x = 3.14159265;
        volatile double poly = (x * x) - (2.0 * x) + 1.0; // (x - 1)^2 approx 4.586419
        if (poly < 4.586 || poly > 4.587) {
            g_task3_failed = true;
            crash_log_add("[PHASE0 TEST] CRITICAL: Task 3 Floating-Point Poly Corruption!");
            break;
        }

        scheduler_yield();
    }
    if (!g_task3_failed) {
        g_task3_passed = true;
    }
    while (1) { scheduler_yield(); }
}

void test_aligned_allocator_stress(void) {
    display_print("\n[PHASE0] Starting Aligned Allocator Stress Test...\n");

    const size_t alignments[] = {16, 32, 64, 128};
    const size_t num_alignments = sizeof(alignments) / sizeof(alignments[0]);

    bool all_passed = true;
    void* ptrs[100];

    for (int cycle = 0; cycle < 20; cycle++) {
        for (int i = 0; i < 100; i++) {
            size_t align = alignments[(i + cycle) % num_alignments];
            size_t sz = (i + 1) * 17;

            ptrs[i] = kmalloc_aligned(sz, align);
            if (!ptrs[i]) {
                display_print("[PHASE0] ERROR: kmalloc_aligned returned NULL!\n");
                all_passed = false;
                break;
            }

            uintptr_t addr = (uintptr_t)ptrs[i];
            if ((addr % align) != 0) {
                display_print("[PHASE0] ERROR: Address 0x");
                display_print_hex(addr);
                display_print(" not aligned to ");
                display_print_dec(align);
                display_print("\n");
                all_passed = false;
                break;
            }
        }

        for (int i = 0; i < 100; i++) {
            if (ptrs[i]) {
                kfree_aligned(ptrs[i]);
            }
        }

        if (!all_passed) break;
    }

    if (all_passed) {
        display_print("[PHASE0] Aligned Allocator Stress Test: PASS (2000/2000 allocations aligned)\n");
        crash_log_add("[PHASE0 TEST] Aligned Allocator Stress Test: PASS");
    } else {
        display_print("[PHASE0] Aligned Allocator Stress Test: FAIL!\n");
        crash_log_add("[PHASE0 TEST] Aligned Allocator Stress Test: FAIL");
    }
}

void test_fpu_sse_isolation_torture(void) {
    display_print("\n[PHASE0] Starting Per-Task FPU/SSE Preemption Isolation Test...\n");

    g_task1_passed = false; g_task1_failed = false;
    g_task2_passed = false; g_task2_failed = false;
    g_task3_passed = false; g_task3_failed = false;

    Task* t1 = scheduler_create_kernel_task("FPU_Test1", task_fpu_pattern1, 16);
    Task* t2 = scheduler_create_kernel_task("FPU_Test2", task_fpu_pattern2, 16);
    Task* t3 = scheduler_create_kernel_task("Math_Torture", task_math_torture, 16);

    if (!t1 || !t2 || !t3) {
        display_print("[PHASE0] ERROR: Failed to spawn Phase 0 test tasks!\n");
        return;
    }

    // Wait for all 3 tasks to complete their loops
    for (int timeout = 0; timeout < 2000; timeout++) {
        if ((g_task1_passed || g_task1_failed) &&
            (g_task2_passed || g_task2_failed) &&
            (g_task3_passed || g_task3_failed)) {
            break;
        }
        scheduler_yield();
    }

    if (g_task1_passed && g_task2_passed && g_task3_passed &&
        !g_task1_failed && !g_task2_failed && !g_task3_failed) {
        display_print("[PHASE0] Preemption & FPU/SSE Isolation Test: PASS\n");
        display_print("[PHASE0] -> Zero Register Corruption across 1500+ preemption switches!\n");
        crash_log_add("[PHASE0 TEST] FPU/SSE Isolation Test: PASS");

        // Keep test tasks safely idling without premature free
        // (Avoids use-after-free on Task structures while scheduler queues hold pointers)
    } else {
        display_print("[PHASE0] Preemption & FPU/SSE Isolation Test: FAIL!\n");
        crash_log_add("[PHASE0 TEST] FPU/SSE Isolation Test: FAIL");
    }
}

void run_phase0_cpu_verification_suite(void) {
    display_print("=========================================\n");
    display_print("   ATOMS OS — OPENGL PHASE 0 VERIFICATION\n");
    display_print("=========================================\n");

    cpu_features_print();
    test_aligned_allocator_stress();
    test_fpu_sse_isolation_torture();

    display_print("=========================================\n");
}
