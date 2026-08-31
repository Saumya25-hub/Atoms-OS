/*
 * ATOMS OS — Phase 7 Userspace & C/C++ Runtime 20-Test Verification Suite
 * Copyright © 2026 ATOMS OS Project / Saumya Chaudhari
 */

#ifndef ATOMS_RUNTIME_TEST_SUITE_H
#define ATOMS_RUNTIME_TEST_SUITE_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t tests_total;
    uint32_t tests_passed;
    uint32_t tests_failed;
    bool all_passed;
} ATOMS_RuntimeTestReport;

bool ATOMS_RunPhase7_RuntimeVerificationSuite(ATOMS_RuntimeTestReport *out_report);

#ifdef __cplusplus
}
#endif

#endif /* ATOMS_RUNTIME_TEST_SUITE_H */
