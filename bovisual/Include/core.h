#ifndef BOVISUAL_CORE_H
#define BOVISUAL_CORE_H

#include "bovisual_types.h"

/**
 * BOVISUAL must expose a stable rendering API.
 * Once the API is finalized, ACE and Rock Engine will depend on it.
 * Internal implementations may evolve (e.g. SIMD, GPU, Double Buffering),
 * but public function signatures should remain stable whenever possible.
 */

// Initialize the BOVISUAL engine with the target framebuffer
bool BOVISUAL_Init(const BVFramebuffer* framebuffer);

// Triggers the main render loop to output to framebuffer
void BOVISUAL_RenderFrame(void);

// Shutdown the engine and release any internal resources
void BOVISUAL_Shutdown(void);

#endif // BOVISUAL_CORE_H
