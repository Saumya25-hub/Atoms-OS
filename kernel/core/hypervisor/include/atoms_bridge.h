/*
 * ATOMS OS — ATOMS Bridge: Guest Diagnostic Command Channel
 * Copyright © 2026 ATOMS OS Project / Saumya Chaudhari
 *
 * UDP port 9998 command/response channel for live guest state inspection.
 * All commands execute inside the VM-exit host context — VMCS is never
 * touched from outside.
 */

#ifndef ATOMS_BRIDGE_H
#define ATOMS_BRIDGE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Bridge Configuration */
#define ATOMS_BRIDGE_PORT           9998
#define ATOMS_BRIDGE_RIP_RING_SIZE  256
#define ATOMS_BRIDGE_EXIT_HISTORY   16

/* Exit history entry */
typedef struct {
    uint32_t reason;
    uint64_t rip;
} AtomsBridgeExitEntry;

/* RIP ring buffer (last 256 exit RIPs) */
typedef struct {
    uint64_t rips[ATOMS_BRIDGE_RIP_RING_SIZE];
    uint32_t head;
    uint32_t count;
} AtomsBridgeRipRing;

/* Last N exit history */
typedef struct {
    AtomsBridgeExitEntry entries[ATOMS_BRIDGE_EXIT_HISTORY];
    uint32_t head;
    uint32_t count;
} AtomsBridgeExitHistory;

/* Initialize the bridge — registers UDP handler on port 9998 */
void atoms_bridge_init(void);

/* Record a VM-exit into the bridge ring buffers.
 * Call this from atoms_vmexit_dispatch() on every exit. */
void atoms_bridge_record_exit(uint32_t reason, uint64_t rip, uint64_t idtr_base, uint32_t idtr_limit);

#ifdef __cplusplus
}
#endif

#endif /* ATOMS_BRIDGE_H */
