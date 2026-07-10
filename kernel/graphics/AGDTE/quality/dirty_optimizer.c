/**
 * @file dirty_optimizer.c
 * @brief AGDTE Phase 5 - Dirty Optimizer Implementation
 * Proximity expansion, bounding box coalescing, fragmentation capping, and 16-byte VRAM alignment.
 */

#include "dirty_optimizer.h"
#include "presentation_diag.h"

#define PROXIMITY_THRESHOLD_PX  32
#define FRAGMENTATION_CAP       4

static uint32_t s_total_coalesced_rectangles = 0;

void AGDTE_DirtyOptimizer_Initialize(void) {
    s_total_coalesced_rectangles = 0;
}

static void agdte_quality_rect_union(BOGE_Rect* out, const BOGE_Rect* a, const BOGE_Rect* b) {
    int32_t min_x = (a->x < b->x) ? a->x : b->x;
    int32_t min_y = (a->y < b->y) ? a->y : b->y;
    
    int32_t max_x_a = a->x + (int32_t)a->width;
    int32_t max_x_b = b->x + (int32_t)b->width;
    int32_t max_x = (max_x_a > max_x_b) ? max_x_a : max_x_b;

    int32_t max_y_a = a->y + (int32_t)a->height;
    int32_t max_y_b = b->y + (int32_t)b->height;
    int32_t max_y = (max_y_a > max_y_b) ? max_y_a : max_y_b;

    out->x = min_x;
    out->y = min_y;
    out->width = (uint32_t)(max_x - min_x);
    out->height = (uint32_t)(max_y - min_y);
}

static bool agdte_quality_should_coalesce(const BOGE_Rect* a, const BOGE_Rect* b) {
    /* Check spatial overlap or proximity within PROXIMITY_THRESHOLD_PX */
    int32_t a_right = a->x + (int32_t)a->width;
    int32_t a_bottom = a->y + (int32_t)a->height;
    int32_t b_right = b->x + (int32_t)b->width;
    int32_t b_bottom = b->y + (int32_t)b->height;

    if (a_right + PROXIMITY_THRESHOLD_PX >= b->x && b_right + PROXIMITY_THRESHOLD_PX >= a->x &&
        a_bottom + PROXIMITY_THRESHOLD_PX >= b->y && b_bottom + PROXIMITY_THRESHOLD_PX >= a->y) {
        
        /* Evaluate bounding area overhead: coalesce if union area <= 1.30 * (a + b area) */
        BOGE_Rect u;
        agdte_quality_rect_union(&u, a, b);
        uint64_t union_area = (uint64_t)u.width * u.height;
        uint64_t sum_area = ((uint64_t)a->width * a->height) + ((uint64_t)b->width * b->height);
        if (union_area <= (sum_area + (sum_area >> 2) + (sum_area >> 4))) {
            return true;
        }
        /* If directly touching or overlapping, always coalesce */
        if (a_right >= b->x && b_right >= a->x && a_bottom >= b->y && b_bottom >= a->y) {
            return true;
        }
    }
    return false;
}

AGDTE_Error AGDTE_DirtyOptimizer_OptimizeRequest(AGDTE_PresentRequest* req) {
    if (!req) return AGDTE_ERR_NULL_POINTER;

    /* Full-screen repaint pass-through */
    if (req->dirty_count == 0) {
        return AGDTE_OK;
    }

    uint32_t count = req->dirty_count;
    if (count > AGDTE_MAX_DIRTY_RECTS) {
        count = AGDTE_MAX_DIRTY_RECTS;
    }

    /* Step 1: Proximity & Overlap Coalescing Loop */
    bool merged = true;
    while (merged && count > 1) {
        merged = false;
        for (uint32_t i = 0; i < count; i++) {
            for (uint32_t j = i + 1; j < count; j++) {
                if (agdte_quality_should_coalesce(&req->dirty_rects[i], &req->dirty_rects[j])) {
                    agdte_quality_rect_union(&req->dirty_rects[i], &req->dirty_rects[i], &req->dirty_rects[j]);
                    req->dirty_rects[j] = req->dirty_rects[count - 1];
                    count--;
                    s_total_coalesced_rectangles++;
                    AGDTE_PresentationDiag_RecordDirtyMerge(req->display_id);
                    merged = true;
                    break;
                }
            }
            if (merged) break;
        }
    }

    /* Step 2: Fragmentation Cap (If count > 4 after coalescing, merge all into macroscopic bounding rect) */
    if (count > FRAGMENTATION_CAP) {
        BOGE_Rect macro_union = req->dirty_rects[0];
        for (uint32_t i = 1; i < count; i++) {
            agdte_quality_rect_union(&macro_union, &macro_union, &req->dirty_rects[i]);
            s_total_coalesced_rectangles++;
            AGDTE_PresentationDiag_RecordDirtyMerge(req->display_id);
        }
        req->dirty_rects[0] = macro_union;
        count = 1;
    }

    /* Step 3: VRAM Cache-Line Alignment (Align X and Width to 4-pixel / 16-byte boundaries) */
    for (uint32_t i = 0; i < count; i++) {
        BOGE_Rect* r = &req->dirty_rects[i];
        if (r->x < 0) r->x = 0;
        if (r->y < 0) r->y = 0;

        int32_t aligned_x = r->x & ~3; /* Align down to nearest multiple of 4 pixels (16 bytes at 32bpp) */
        int32_t right_x = r->x + (int32_t)r->width;
        int32_t aligned_right = (right_x + 3) & ~3; /* Align up to multiple of 4 */

        r->x = aligned_x;
        r->width = (uint32_t)(aligned_right - aligned_x);
        if (r->width == 0) r->width = 4;
    }

    req->dirty_count = count;
    return AGDTE_OK;
}

uint32_t AGDTE_DirtyOptimizer_GetTotalCoalescedCount(void) {
    return s_total_coalesced_rectangles;
}
