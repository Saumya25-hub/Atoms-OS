/*
 * ATOMS OS — Phase 1 Java Runtime Foundation Verification Suite
 * Copyright © 2026 ATOMS OS Project / Saumya Chaudhari
 */

#ifndef ATOMS_PHASE1_RUNTIME_TEST_H
#define ATOMS_PHASE1_RUNTIME_TEST_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t total_tests;
    uint32_t passed_tests;
    uint32_t failed_tests;
    bool all_passed;
} ATOMS_Phase1RuntimeReport;

bool ATOMS_RunPhase1_JavaRuntimeFoundationTests(ATOMS_Phase1RuntimeReport *out_report);

#ifdef __cplusplus
}
#endif

#endif /* ATOMS_PHASE1_RUNTIME_TEST_H */
