#ifndef BOSX_STRESS_H
#define BOSX_STRESS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================
// ATOMS Phase 10 Stress, Leak Audit & Verification Suite
// ============================================================

bool ATOMS_BOSX_Run1000LaunchExitStressTest(void);
bool ATOMS_SLL_Run10000SymbolResolutionStressTest(void);
bool ATOMS_Installer_Run1000InstallUninstallStressTest(void);
bool ATOMS_BOSX_RunResourceLeakAudit(void);

void ATOMS_RunPhase10_VerificationSuite(void);

#ifdef __cplusplus
}
#endif

#endif // BOSX_STRESS_H
