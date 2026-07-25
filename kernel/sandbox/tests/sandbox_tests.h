/*
 * BOS OS — Phase 4: Production Process Sandbox & Capability Engine
 * sandbox_tests.h — Certification Test Suite Header
 */

#ifndef BOS_SANDBOX_TESTS_H
#define BOS_SANDBOX_TESTS_H

#include "kernel/sandbox/include/sandbox_types.h"

#ifdef __cplusplus
extern "C" {
#endif

bos_sandbox_status_t sandbox_run_certification_tests(void);

#ifdef __cplusplus
}
#endif

#endif /* BOS_SANDBOX_TESTS_H */
