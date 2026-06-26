#ifndef BOSCAL_H
#define BOSCAL_H

#include "display.h"
#include "boscal_types.h"
#include "geometry.h"

// ----------------------------------------------------
// BOSCAL: Auto Layout Engine Core
// ----------------------------------------------------

// Initialize the Layout Engine with display properties
void BOSCAL_Init(uint32_t width, uint32_t height, uint32_t pitch, uint32_t bpp);

// Display Event Handler (called by Display Driver/VBE on mode switch)
void BOSCAL_OnDisplayChanged(uint32_t new_w, uint32_t new_h);

// Core Engine: Resolves abstract dimensions/anchors into absolute pixel coordinates
BVRect BOSCAL_ResolveLayout(BVRect parent, BVDimension w, BVDimension h, BVAnchor anchor);

// Convenience wrapper: Resolve inside the current Work Area
BVRect BOSCAL_ResolveDesktopLayout(BVDimension w, BVDimension h, BVAnchor anchor);

#endif // BOSCAL_H
