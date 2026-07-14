/**
 * @file bdce_snapshot.h
 * @brief ATOMS OS BDCE (BOS Display Contract Engine) - Display Snapshot Structure Definition
 * @note Phase A — Architectural Foundation Only. Structure definition only (`Do NOT populate or freeze`).
 */

#ifndef ATOMS_OS_BDCE_SNAPSHOT_H
#define ATOMS_OS_BDCE_SNAPSHOT_H

#include "bdce_types.h"

/**
 * @brief Immutable frame snapshot enclosing all 9 Pillars of State for a single rendering tick.
 * Ensures strict temporal consistency across concurrent pipeline stages.
 */
typedef struct {
    uint64_t               snapshot_sequence_id;
    uint64_t               capture_timestamp_us;
    BDCE_PhysicalState     physical;
    BDCE_LogicalState      logical;
    BDCE_SurfaceState      surface;
    BDCE_TemporalState     temporal;
    BDCE_FrameState        frame;
    BDCE_PresentationState presentation;
    BDCE_DamageState       damage;
    BDCE_OwnershipState    ownership;
    BDCE_CapabilityState   capability;
} BDCE_DisplaySnapshot;

#endif /* ATOMS_OS_BDCE_SNAPSHOT_H */
