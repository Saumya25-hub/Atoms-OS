#ifndef KERNEL_DISPATCHER_PRIORITY_H
#define KERNEL_DISPATCHER_PRIORITY_H

#include <stdint.h>
#include <stdbool.h>

// ============================================================================
// ATOMS OS Input Engine V2 - Phase 4: Event Dispatcher Priority Tiers
// ============================================================================
// Strict priority hierarchy ensuring deterministic event routing.
// ============================================================================

typedef enum {
    DISPATCH_TIER_0_POINTER_ENGINE = 0, // Tier 0: Pointer Engine (State synchronization / feedback)
    DISPATCH_TIER_1_CURSOR_ENGINE  = 1, // Tier 1: Cursor Engine (Hardware cursor rendering plane)
    DISPATCH_TIER_2_DESKTOP        = 2, // Tier 2: Desktop Shell / Login Screen (Global shortcuts, modals)
    DISPATCH_TIER_3_WINDOW_MANAGER = 3, // Tier 3: Window Manager (Hit-testing, window drag/resize, focus)
    DISPATCH_TIER_4_WIDGETS        = 4, // Tier 4: UI Widgets / Controls (Buttons, sliders, text boxes)
    DISPATCH_TIER_5_APPLICATIONS   = 5  // Tier 5: User Applications / Background Tasks
} DispatcherPriorityTier;

#define DISPATCH_TIER_COUNT 6

// Propagation control return codes
typedef enum {
    DISPATCH_CONTINUE = 0, // Event continues propagating to lower priority tiers
    DISPATCH_CONSUME  = 1  // Event is claimed; stop propagation to lower tiers
} DispatchResult;

// Validate if a priority tier is within valid bounds
bool dispatcher_priority_is_valid(DispatcherPriorityTier tier);

// Get human-readable string name for a priority tier
const char* dispatcher_priority_get_name(DispatcherPriorityTier tier);

#endif // KERNEL_DISPATCHER_PRIORITY_H
