#ifndef BGL_H
#define BGL_H

#include "bgl_types.h"
#include "bgl_drawable.h"
#include "bgl_context.h"

// ============================================================
// BGL Public Platform API (Native BOS Interface)
// ============================================================

// Context & Drawable Lifecycle
BGLDrawable* bglCreateDrawableForWindow(uint32_t window_id);
bool         bglDestroyDrawable(BGLDrawable* drawable);

BGLContext*  bglCreateContext(BGLDrawable* drawable);
bool         bglDestroyContext(BGLContext* ctx);

// MakeCurrent Semantics (Task-Local Context Binding)
bool         bglMakeCurrent(BGLContext* ctx, BGLDrawable* drawable);
void         bglReleaseCurrent(void);
BGLContext*  bglGetCurrentContext(void);

// Frame Presentation & Resize Safety
bool         bglSwapBuffers(BGLContext* ctx);
bool         bglResizeDrawable(BGLDrawable* drawable, uint32_t new_w, uint32_t new_h);

// Diagnostic Operations
void         bglDiagnosticClear(BGLContext* ctx, uint32_t color);

// Error Handling
BGLError     bglGetLastError(void);
void         bglSetLastError(BGLError err);

#endif // BGL_H
