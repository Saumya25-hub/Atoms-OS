#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include "../libbos/include/bos.h"
#include <stdbool.h>

// =============================================
// BOS OS - Test Framework Macros
// =============================================

extern int g_tests_passed;
extern int g_tests_failed;
extern int g_total_tests;

#define TEST_SUITE_START(name) \
    bos_print("\n========================================\n"); \
    bos_print(" SUITE: " name "\n"); \
    bos_print("========================================\n");

#define TEST_PASS(name) \
    do { \
        bos_print("[TEST] "); bos_print(name); \
        int len = 0; const char* p = name; while (*p) { len++; p++; } \
        for (int i = len; i < 30; i++) bos_print("."); \
        bos_print(" PASS\n"); \
        g_tests_passed++; g_total_tests++; \
    } while(0)

#define TEST_FAIL(name, reason) \
    do { \
        bos_print("[TEST] "); bos_print(name); \
        int len = 0; const char* p = name; while (*p) { len++; p++; } \
        for (int i = len; i < 30; i++) bos_print("."); \
        bos_print(" FAIL ("); bos_print(reason); bos_print(")\n"); \
        g_tests_failed++; g_total_tests++; \
    } while(0)

#define ASSERT(condition, name) \
    do { \
        if (condition) { \
            TEST_PASS(name); \
        } else { \
            TEST_FAIL(name, #condition); \
        } \
    } while(0)

// Module entry points
void test_heap_suite(void);
void test_scheduler_suite(void);
void test_filesystem_suite(void);
void test_bosl_suite(void);
void test_bishop_suite(void);
void test_panic_suite(void);
void test_performance_suite(void);

#endif // TEST_FRAMEWORK_H
