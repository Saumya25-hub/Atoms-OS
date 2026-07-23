#ifndef TEST_CPU_PHASE0_H
#define TEST_CPU_PHASE0_H

#include <stdbool.h>

void test_aligned_allocator_stress(void);
void test_fpu_sse_isolation_torture(void);
void run_phase0_cpu_verification_suite(void);

#endif // TEST_CPU_PHASE0_H
