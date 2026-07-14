/**
 * @file bdce_authority.h
 * @brief ATOMS OS BDCE (BOS Display Contract Engine) - Public Authority Layer Declarations
 * @note Phase A — Architectural Foundation Only. Zero algorithms, rendering, or behavior changes.
 */

#ifndef ATOMS_OS_BDCE_AUTHORITY_H
#define ATOMS_OS_BDCE_AUTHORITY_H

#include "bdce_types.h"
#include "bdce_context.h"
#include "bdce_snapshot.h"

/* ========================================================================== */
/* BDCE Context & Authority Public Declarations (`Phase A Stubs`)             */
/* ========================================================================== */

/**
 * @brief Retrieves the primary authoritative display context pointer.
 * @return Pointer to the primary BDCE_Context (`dormant in Phase A`).
 */
BDCE_Context* BDCE_GetPrimaryContext(void);

/**
 * @brief Retrieves the immutable Physical State from a display context.
 */
const BDCE_PhysicalState* BDCE_GetPhysicalState(const BDCE_Context* ctx);

/**
 * @brief Retrieves the immutable Logical State from a display context.
 */
const BDCE_LogicalState* BDCE_GetLogicalState(const BDCE_Context* ctx);

/**
 * @brief Retrieves the immutable Surface State from a display context.
 */
const BDCE_SurfaceState* BDCE_GetSurfaceState(const BDCE_Context* ctx);

/**
 * @brief Retrieves the immutable Temporal State from a display context.
 */
const BDCE_TemporalState* BDCE_GetTemporalState(const BDCE_Context* ctx);

/**
 * @brief Retrieves the immutable Frame State from a display context.
 */
const BDCE_FrameState* BDCE_GetFrameState(const BDCE_Context* ctx);

/**
 * @brief Retrieves the immutable Presentation State from a display context.
 */
const BDCE_PresentationState* BDCE_GetPresentationState(const BDCE_Context* ctx);

/**
 * @brief Retrieves the immutable Damage State from a display context.
 */
const BDCE_DamageState* BDCE_GetDamageState(const BDCE_Context* ctx);

/**
 * @brief Retrieves the current Ownership State enum from a display context.
 */
BDCE_OwnershipState BDCE_GetOwnershipState(const BDCE_Context* ctx);

/**
 * @brief Retrieves the immutable Capability State from a display context.
 */
const BDCE_CapabilityState* BDCE_GetCapabilityState(const BDCE_Context* ctx);

/* ========================================================================== */
/* Phase B: Read-Only State Seeding (`Passive Observation Layer`)             */
/* ========================================================================== */

/**
 * @brief Seeds BDCE by passively reading current physical, logical, capability, and temporal values.
 * @note Strictly read-only observation. Zero calculations, zero derived values, zero control transfer.
 * @param boot_info Pointer to multiboot boot_info_t struct (const void* to avoid header coupling).
 * @param hw_fb Pointer to hardware framebuffer BVFramebuffer (const void* to avoid header coupling).
 */
void BDCE_SeedFromCurrentSystem(const void* boot_info, const void* hw_fb);

#endif /* ATOMS_OS_BDCE_AUTHORITY_H */

