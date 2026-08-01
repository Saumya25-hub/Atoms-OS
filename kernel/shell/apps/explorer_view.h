#ifndef BOS_EXPLORER_VIEW_H
#define BOS_EXPLORER_VIEW_H

#include <stdint.h>
#include <stdbool.h>
#include "explorer.h"
#include "kernel/wm/bwe/include/bwe.h"

// ============================================================
// Explorer Renderer — Pure Drawing Functions (Phase 9)
// ============================================================
// These functions ONLY draw. They NEVER access filesystem.
// They consume ExplorerViewItem (BSOM objects) for rendering.
// ============================================================

void Explorer_DrawToolbar(BVFramebuffer* fb, int32_t x, int32_t y, int32_t w, int32_t h, ExplorerContext* ctx);
void Explorer_DrawAddressBar(BVFramebuffer* fb, int32_t x, int32_t y, int32_t w, int32_t h, ExplorerContext* ctx);
void Explorer_DrawSidebar(BVFramebuffer* fb, int32_t x, int32_t y, int32_t w, int32_t h, ExplorerContext* ctx);
void Explorer_DrawFiles(BVFramebuffer* fb, int32_t x, int32_t y, int32_t w, int32_t h, ExplorerContext* ctx);
void Explorer_DrawStatusbar(BVFramebuffer* fb, int32_t x, int32_t y, int32_t w, int32_t h, ExplorerContext* ctx);

void Explorer_RenderWindow(BWE_Window* win);

#endif // BOS_EXPLORER_VIEW_H
