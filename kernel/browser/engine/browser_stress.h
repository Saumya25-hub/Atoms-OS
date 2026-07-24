#ifndef ATRIX_BROWSER_STRESS_H
#define ATRIX_BROWSER_STRESS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================
// ATRIX Browser Engine Stress & Diagnostics Suite
// ============================================================

bool ATRIX_Browser_Run10000DOMNodeStressTest(void);
bool ATRIX_Browser_Run1000CSSComputationStressTest(void);
bool ATRIX_Browser_Run1000JSVMExecutionStressTest(void);
bool ATRIX_Browser_Run1000PageRenderStressTest(void);

void ATRIX_RunPhase12_VerificationSuite(void);

#ifdef __cplusplus
}
#endif

#endif // ATRIX_BROWSER_STRESS_H
