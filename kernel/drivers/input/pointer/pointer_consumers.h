#ifndef KERNEL_POINTER_CONSUMERS_H
#define KERNEL_POINTER_CONSUMERS_H

#include "pointer_state.h"

// ============================================================================
// ATOMS OS Input Engine V2 - Phase 3: Publish-Subscribe Consumer Dispatcher
// ============================================================================
// Decouples Pointer Engine from UI rendering and window manager internals.
// ============================================================================

#define POINTER_MAX_CONSUMERS 16

typedef void (*PointerUpdateCallback)(const PointerState* state, void* user_data);

// Initialize consumer notification registry
void pointer_consumers_init(void);

// Register a consumer listener (Desktop, WM, Widgets, Apps, future Cursor Engine)
bool pointer_consumers_register(const char* name, PointerUpdateCallback callback, void* user_data);

// Broadcast an updated PointerState to all registered consumers
void pointer_consumers_notify(const PointerState* state);

#endif // KERNEL_POINTER_CONSUMERS_H
