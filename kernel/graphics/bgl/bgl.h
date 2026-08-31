#ifndef BGL_H
#define BGL_H

#include "bgl_types.h"
#include "bgl_drawable.h"
#include "bgl_context.h"

// ============================================================
// BGL Public Platform API (Native BOS Interface)
// ============================================================

#ifdef __cplusplus
extern "C" {
#endif

// MakeCurrent Semantics (Task-Local Context Binding)
bool         bglMakeCurrent(BGLContext* ctx, BGLDrawable* drawable);
void         bglReleaseCurrent(void);
BGLContext*  bglGetCurrentContext(void);

// Frame Presentation
bool         bglSwapBuffers(BGLContext* ctx);

// Error Handling
BGLError     bglGetLastError(void);
void         bglSetLastError(BGLError err);

#ifdef __cplusplus
}
#endif

#endif // BGL_H

