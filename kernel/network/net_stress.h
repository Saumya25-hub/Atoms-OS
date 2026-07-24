#ifndef ATOMS_NET_STRESS_H
#define ATOMS_NET_STRESS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================
// ATOMS Phase 11 Networking Stress & Diagnostics Suite
// ============================================================

bool ATOMS_Net_Run100000PacketStressTest(void);
bool ATOMS_Net_Run10000TCPConnectionStressTest(void);
bool ATOMS_Net_Run1000DNSLookupStressTest(void);
bool ATOMS_Net_Run1000HTTPDownloadStressTest(void);
bool ATOMS_Net_RunSocketLeakAudit(void);

void ATOMS_RunPhase11_VerificationSuite(void);

#ifdef __cplusplus
}
#endif

#endif // ATOMS_NET_STRESS_H
