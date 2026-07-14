/**
 * @file bdce_context.h
 * @brief ATOMS OS BDCE (BOS Display Contract Engine) - Display Context Structure Definition
 * @note Phase A — Architectural Foundation Only. Zero algorithms or rendering logic.
 */

#ifndef ATOMS_OS_BDCE_CONTEXT_H
#define ATOMS_OS_BDCE_CONTEXT_H

#include "bdce_types.h"

/**
 * @brief Opaque handle and authoritative container for a single display context.
 * Today there will be one primary context; tomorrow there may be many (multi-monitor).
 */
typedef struct BDCE_Context {
    uint32_t             context_id;
    bool                 is_active;
    BDCE_PhysicalState   physical_state;
    BDCE_LogicalState    logical_state;
    BDCE_SurfaceState    surface_state;
    BDCE_TemporalState   temporal_state;
    BDCE_FrameState      frame_state;
    BDCE_PresentationState presentation_state;
    BDCE_DamageState     damage_state;
    BDCE_OwnershipState  ownership_state;
    BDCE_CapabilityState capability_state;
} BDCE_Context;

#endif /* ATOMS_OS_BDCE_CONTEXT_H */
