#include "test_framework.h"
#include "../libbos/include/bos.h"

int g_tests_passed = 0;
int g_tests_failed = 0;
int g_total_tests = 0;

// Helper to print integers
static void print_int(int val) {
    if (val < 0) { bos_print("-"); val = -val; }
    char buf[20]; int i = 18; buf[19] = '\0';
    if (val == 0) { buf[i--] = '0'; }
    else { while (val > 0) { buf[i--] = (val % 10) + '0'; val /= 10; } }
    bos_print(&buf[i+1]);
}

void _start(void) {
    bos_print("\n========================================\n");
    bos_print("  SIGNATURESOS VALIDATION & STRESS TEST \n");
    bos_print("========================================\n");

    g_tests_passed = 0;
    g_tests_failed = 0;
    g_total_tests = 0;

    // Run suites
    test_heap_suite();
    test_scheduler_suite();
    test_filesystem_suite();
    test_bosl_suite();
    test_bishop_suite();
    test_panic_suite();
    test_performance_suite();

    bos_print("\n========================================\n");
    if (g_tests_failed == 0) {
        bos_print("  ALL TESTS PASSED: ");
        print_int(g_tests_passed);
        bos_print(" / ");
        print_int(g_total_tests);
        bos_print("\n");
        bos_print("  SYSTEM IS STABLE.\n");
    } else {
        bos_print("  TESTS FAILED: ");
        print_int(g_tests_failed);
        bos_print(" / ");
        print_int(g_total_tests);
        bos_print("\n");
        bos_print("  SYSTEM IS UNSTABLE.\n");
    }
    bos_print("========================================\n\n");
    bos_exit();
}
