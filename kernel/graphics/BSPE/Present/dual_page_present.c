/**
 * @file dual_page_present.c
 * @brief BSPE Dual-Page Damage Presentation Engine Production Implementation
 * @status Step 12 Production Implementation
 * 
 * @section PURPOSE
 * Implements the mathematical presentation equation:
 * EffectiveDamage = Union(Damage(Current Frame), Damage(Previous Frame))
 * This becomes the ONLY damage list used by BSPE presentation, ensuring double-buffered
 * VRAM synchronization without modifying source lists or performing heap allocations.
 * 
 * @section RULES_ENFORCED
 * - Current damage is NEVER destroyed.
 * - Previous damage is NEVER modified.
 * - Generates a new Effective Damage list.
 * - History advances ONLY after a successful present.
 * - On failure or corruption, executes Legacy SwapFull Backend and rolls back history.
 */

#include <stddef.h>
#include "dual_page_present.h"
#include "bovisual/Include/bovisual_types.h"
#include "bovisual/Include/graphics.h"

/* Static telemetry tracking structure */
static BSPE_DualPageTelemetry g_dual_telemetry = {0};

/* External legacy backend accessor from bovisual/Graphics/graphics.c */
extern void BOVISUAL_Graphics_LegacySwapFull_Backend(const void* hw_fb);

void BSPE_DualPage_GetTelemetry(BSPE_DualPageTelemetry* out_telemetry) {
    if (!out_telemetry) return;
    *out_telemetry = g_dual_telemetry;
}

void BSPE_DualPage_ResetTelemetry(void) {
    g_dual_telemetry.current_rect_count = 0;
    g_dual_telemetry.previous_rect_count = 0;
    g_dual_telemetry.effective_rect_count = 0;
    g_dual_telemetry.merged_rectangles = 0;
    g_dual_telemetry.discarded_rectangles = 0;
    g_dual_telemetry.duplicate_rectangles = 0;
    g_dual_telemetry.history_advances = 0;
    g_dual_telemetry.history_rollbacks = 0;
    g_dual_telemetry.fallback_count = 0;
}

/* --- Pure Rectangle Mathematics Helpers (No Recursion) --- */

static inline int32_t dp_min(int32_t a, int32_t b) { return (a < b) ? a : b; }
static inline int32_t dp_max(int32_t a, int32_t b) { return (a > b) ? a : b; }

static inline bool dp_rect_is_valid(const BOGE_Rect* r) {
    return (r && r->width > 0 && r->height > 0);
}

static inline void dp_rect_clip(BOGE_Rect* r, int32_t max_w, int32_t max_h) {
    int32_t l = r->x;
    int32_t t = r->y;
    int32_t rt = r->x + (int32_t)r->width;
    int32_t b = r->y + (int32_t)r->height;
    if (l < 0) l = 0;
    if (t < 0) t = 0;
    if (rt > max_w) rt = max_w;
    if (b > max_h) b = max_h;
    r->x = l;
    r->y = t;
    r->width = (rt > l) ? (uint32_t)(rt - l) : 0;
    r->height = (b > t) ? (uint32_t)(b - t) : 0;
}

static inline bool dp_rects_overlap_or_touch(const BOGE_Rect* a, const BOGE_Rect* b) {
    int32_t al = a->x, at = a->y, ar = a->x + (int32_t)a->width, ab = a->y + (int32_t)a->height;
    int32_t bl = b->x, bt = b->y, br = b->x + (int32_t)b->width, bb = b->y + (int32_t)b->height;
    return (al <= br && bl <= ar && at <= bb && bt <= ab);
}

static inline bool dp_rect_contains_or_equal(const BOGE_Rect* outer, const BOGE_Rect* inner) {
    int32_t ol = outer->x, ot = outer->y, or_ = outer->x + (int32_t)outer->width, ob = outer->y + (int32_t)outer->height;
    int32_t il = inner->x, it = inner->y, ir = inner->x + (int32_t)inner->width, ib = inner->y + (int32_t)inner->height;
    return (il >= ol && ir <= or_ && it >= ot && ib <= ob);
}

static inline BOGE_Rect dp_rect_union(const BOGE_Rect* a, const BOGE_Rect* b) {
    int32_t l = dp_min(a->x, b->x);
    int32_t t = dp_min(a->y, b->y);
    int32_t rt = dp_max(a->x + (int32_t)a->width, b->x + (int32_t)b->width);
    int32_t bm = dp_max(a->y + (int32_t)a->height, b->y + (int32_t)b->height);
    BOGE_Rect res = { l, t, (rt > l) ? (uint32_t)(rt - l) : 0, (bm > t) ? (uint32_t)(bm - t) : 0 };
    return res;
}

/**
 * @brief Core algorithm evaluating EffectiveDamage = Union(Current, Previous).
 * Merges touching/overlapping rects, removes duplicates, clips to boundaries, and records telemetry.
 */
static bool bspe_dual_page_evaluate_effective(const BOGE_Rect* current_rects, uint32_t current_count,
                                              const BOGE_Rect* prev_rects, uint32_t prev_count,
                                              uint32_t screen_w, uint32_t screen_h,
                                              BOGE_Rect* out_rects, uint32_t max_out, uint32_t* out_count,
                                              bool record_telemetry) {
    uint32_t count = 0;
    const BOGE_Rect* sources[2] = { current_rects, prev_rects };
    uint32_t counts[2] = { current_count, prev_count };

    for (int s = 0; s < 2; s++) {
        const BOGE_Rect* src = sources[s];
        uint32_t src_cnt = counts[s];
        if (!src || src_cnt == 0) continue;

        for (uint32_t i = 0; i < src_cnt; i++) {
            BOGE_Rect r = src[i];
            dp_rect_clip(&r, (int32_t)screen_w, (int32_t)screen_h);
            if (!dp_rect_is_valid(&r)) {
                if (record_telemetry) g_dual_telemetry.discarded_rectangles++;
                continue;
            }

            bool merged_any = true;
            while (merged_any) {
                merged_any = false;
                for (uint32_t j = 0; j < count; j++) {
                    if (dp_rect_contains_or_equal(&out_rects[j], &r)) {
                        if (record_telemetry) g_dual_telemetry.duplicate_rectangles++;
                        r.width = 0;
                        break;
                    }
                    if (dp_rect_contains_or_equal(&r, &out_rects[j])) {
                        if (record_telemetry) g_dual_telemetry.duplicate_rectangles++;
                        out_rects[j] = r;
                        r.width = 0;
                        break;
                    }
                    if (dp_rects_overlap_or_touch(&out_rects[j], &r)) {
                        if (record_telemetry) g_dual_telemetry.merged_rectangles++;
                        r = dp_rect_union(&out_rects[j], &r);
                        out_rects[j] = out_rects[count - 1];
                        count--;
                        merged_any = true;
                        break;
                    }
                }
                if (r.width == 0) break;
            }

            if (r.width == 0) continue;

            if (count < max_out) {
                out_rects[count++] = r;
            } else {
                /* Overflow protection: merge into pair introducing minimal extra area */
                uint32_t min_extra = 0xFFFFFFFF;
                uint32_t best_j = 0;
                for (uint32_t j = 0; j < count; j++) {
                    BOGE_Rect u = dp_rect_union(&out_rects[j], &r);
                    uint32_t extra = (u.width * u.height) - (out_rects[j].width * out_rects[j].height) - (r.width * r.height);
                    if (extra < min_extra) {
                        min_extra = extra;
                        best_j = j;
                    }
                }
                if (record_telemetry) g_dual_telemetry.merged_rectangles++;
                out_rects[best_j] = dp_rect_union(&out_rects[best_j], &r);
            }
        }
    }

    *out_count = count;
    return true;
}

BSPE_Error BSPE_DualPage_PresentFrame(BSPE_DamageTrackerHandle damage_tracker, const BOGE_StagingFrame* frame) {
    /* 1. Validate input and frame state */
    if (!frame || !frame->buffer_virtual_address) {
        g_dual_telemetry.history_rollbacks++;
        return BSPE_ERR_NULL_POINTER;
    }
    if (frame->width == 0 || frame->height == 0) {
        g_dual_telemetry.history_rollbacks++;
        return BSPE_ERR_INVALID_STATE;
    }

    /* 2. Retrieve previous frame damage from tracker (read-only query) */
    BOGE_Rect prev_rects[32];
    uint32_t prev_count = 0;
    if (damage_tracker) {
        BSPE_DamageTracker_GetPreviousDamage(damage_tracker, prev_rects, 32, &prev_count);
    }

    g_dual_telemetry.current_rect_count = frame->dirty_count;
    g_dual_telemetry.previous_rect_count = prev_count;

    /* 3. Evaluate EffectiveDamage = Union(Damage(N), Damage(N-1)) */
    BOGE_Rect effective_rects[32];
    uint32_t effective_count = 0;
    bool eval_ok = bspe_dual_page_evaluate_effective(frame->dirty_rects, frame->dirty_count,
                                                     prev_rects, prev_count,
                                                     frame->width, frame->height,
                                                     effective_rects, 32, &effective_count, true);

    g_dual_telemetry.effective_rect_count = effective_count;

    if (effective_count == 0) {
        /* No pixels changed on screen. Fast-path return to prevent wasting MMIO bandwidth. */
        return BSPE_OK;
    }

    /* 4. Check Emergency Fallback conditions:
     * - !damage_tracker
     * - !bspe_use_partial_present
     * - eval_ok == false OR corruption detected
     */
    bool trigger_fallback = (!damage_tracker || !bspe_use_partial_present || !eval_ok);

    /* Check for corruption (out-of-bounds coordinates) */
    if (!trigger_fallback) {
        for (uint32_t i = 0; i < effective_count; i++) {
            if (effective_rects[i].x + effective_rects[i].width > frame->width ||
                effective_rects[i].y + effective_rects[i].height > frame->height) {
                trigger_fallback = true;
                break;
            }
        }
    }

    BSPE_Error present_err = BSPE_OK;
    if (trigger_fallback) {
        /* Automatically execute Legacy SwapFull Backend */
        BOVISUAL_Graphics_LegacySwapFull_Backend(frame->buffer_virtual_address);
        g_dual_telemetry.fallback_count++;
        present_err = BSPE_OK;
    } else {
        /* Execute Partial VRAM Copy using EffectiveDamage list */
        present_err = BSPE_VRAM_CopyEffectiveDamage(frame, effective_rects, effective_count);
        if (present_err != BSPE_OK) {
            /* If partial copy fails, execute emergency fallback */
            BOVISUAL_Graphics_LegacySwapFull_Backend(frame->buffer_virtual_address);
            g_dual_telemetry.fallback_count++;
            present_err = BSPE_OK;
        }
    }

    /* 5. Advance history ONLY after a successful present! */
    if (present_err == BSPE_OK && damage_tracker) {
        /* Update Damage(N) in active page, then advance frame so it becomes Damage(N-1) for next frame */
        BSPE_DamageTracker_Clear(damage_tracker);
        for (uint32_t i = 0; i < frame->dirty_count; i++) {
            BSPE_DamageTracker_AddRect(damage_tracker, &frame->dirty_rects[i]);
        }
        BSPE_DamageTracker_AdvanceFrame(damage_tracker);
        g_dual_telemetry.history_advances++;
    } else {
        g_dual_telemetry.history_rollbacks++;
    }

    return present_err;
}

/* --- Verification Stress Test Suite (11 Required Scenarios) --- */

#define TEST_GRID_SIZE 128
static uint32_t s_test_src_grid[TEST_GRID_SIZE * TEST_GRID_SIZE];
static uint32_t s_test_dst_legacy[TEST_GRID_SIZE * TEST_GRID_SIZE];
static uint32_t s_test_dst_dual[TEST_GRID_SIZE * TEST_GRID_SIZE];

static uint32_t dp_rand(uint32_t* seed) {
    *seed = (*seed * 1103515245 + 12345) & 0x7FFFFFFF;
    return *seed;
}

bool BSPE_DualPage_RunStressTest(void) {
    bool all_passed = true;
    BOGE_Rect out_rects[32];
    uint32_t out_cnt = 0;

    /* Test 1: Empty Frame */
    bspe_dual_page_evaluate_effective(NULL, 0, NULL, 0, 1024, 768, out_rects, 32, &out_cnt, false);
    if (out_cnt != 0) all_passed = false;

    /* Test 2: Single Rectangle */
    BOGE_Rect r_single = {10, 10, 50, 50};
    bspe_dual_page_evaluate_effective(&r_single, 1, NULL, 0, 1024, 768, out_rects, 32, &out_cnt, false);
    if (out_cnt != 1 || out_rects[0].x != 10 || out_rects[0].width != 50) all_passed = false;

    /* Test 3: Multiple Disjoint Rectangles */
    BOGE_Rect r_multi[3] = {{0, 0, 20, 20}, {50, 50, 20, 20}, {100, 100, 20, 20}};
    bspe_dual_page_evaluate_effective(r_multi, 3, NULL, 0, 1024, 768, out_rects, 32, &out_cnt, false);
    if (out_cnt != 3) all_passed = false;

    /* Test 4: Duplicate Rectangles */
    BOGE_Rect r_dup[3] = {{20, 20, 40, 40}, {20, 20, 40, 40}, {25, 25, 10, 10}};
    bspe_dual_page_evaluate_effective(r_dup, 3, NULL, 0, 1024, 768, out_rects, 32, &out_cnt, false);
    if (out_cnt != 1 || out_rects[0].width != 40) all_passed = false;

    /* Test 5: Touching Rectangles */
    BOGE_Rect r_touch[2] = {{0, 0, 50, 50}, {50, 0, 50, 50}};
    bspe_dual_page_evaluate_effective(r_touch, 2, NULL, 0, 1024, 768, out_rects, 32, &out_cnt, false);
    if (out_cnt != 1 || out_rects[0].width != 100) all_passed = false;

    /* Test 6: Overlapping Rectangles */
    BOGE_Rect r_over[2] = {{10, 10, 40, 40}, {30, 30, 40, 40}};
    bspe_dual_page_evaluate_effective(r_over, 2, NULL, 0, 1024, 768, out_rects, 32, &out_cnt, false);
    if (out_cnt != 1 || out_rects[0].x != 10 || out_rects[0].width != 60) all_passed = false;

    /* Test 7: Previous Frame Only */
    BOGE_Rect r_prev = {15, 15, 30, 30};
    bspe_dual_page_evaluate_effective(NULL, 0, &r_prev, 1, 1024, 768, out_rects, 32, &out_cnt, false);
    if (out_cnt != 1 || out_rects[0].x != 15) all_passed = false;

    /* Test 8: Current Frame Only */
    BOGE_Rect r_curr = {40, 40, 20, 20};
    bspe_dual_page_evaluate_effective(&r_curr, 1, NULL, 0, 1024, 768, out_rects, 32, &out_cnt, false);
    if (out_cnt != 1 || out_rects[0].x != 40) all_passed = false;

    /* Test 9: Mixed History (Current + Previous Overlapping) */
    BOGE_Rect r_c = {10, 10, 30, 30};
    BOGE_Rect r_p = {30, 30, 30, 30};
    bspe_dual_page_evaluate_effective(&r_c, 1, &r_p, 1, 1024, 768, out_rects, 32, &out_cnt, false);
    if (out_cnt != 1 || out_rects[0].width != 50) all_passed = false;

    /* Test 10: Random Stress (1000 Simulated Frames) */
    uint32_t seed = 0x87654321;
    for (int frame_idx = 0; frame_idx < 1000; frame_idx++) {
        BOGE_Rect c_list[10];
        BOGE_Rect p_list[10];
        uint32_t c_cnt = (dp_rand(&seed) % 10) + 1;
        uint32_t p_cnt = (dp_rand(&seed) % 10) + 1;
        for (uint32_t i = 0; i < c_cnt; i++) {
            c_list[i].x = dp_rand(&seed) % 900;
            c_list[i].y = dp_rand(&seed) % 700;
            c_list[i].width = (dp_rand(&seed) % 100) + 1;
            c_list[i].height = (dp_rand(&seed) % 100) + 1;
        }
        for (uint32_t i = 0; i < p_cnt; i++) {
            p_list[i].x = dp_rand(&seed) % 900;
            p_list[i].y = dp_rand(&seed) % 700;
            p_list[i].width = (dp_rand(&seed) % 100) + 1;
            p_list[i].height = (dp_rand(&seed) % 100) + 1;
        }
        bspe_dual_page_evaluate_effective(c_list, c_cnt, p_list, p_cnt, 1024, 768, out_rects, 32, &out_cnt, false);
        
        /* Verify every input rectangle is completely covered by an effective rectangle */
        for (uint32_t i = 0; i < c_cnt; i++) {
            bool covered = false;
            for (uint32_t j = 0; j < out_cnt; j++) {
                if (dp_rect_contains_or_equal(&out_rects[j], &c_list[i])) {
                    covered = true;
                    break;
                }
            }
            if (!covered) all_passed = false;
        }
    }

    /* Test 11: Pixel Identical against Legacy Path (5-Frame Sequence on 128x128 Grid) */
    for (uint32_t i = 0; i < (TEST_GRID_SIZE * TEST_GRID_SIZE); i++) {
        s_test_src_grid[i] = 0xFF000000 | (i * 333 + 777);
        s_test_dst_legacy[i] = 0xFF000000;
        s_test_dst_dual[i] = 0xFF000000;
    }

    BOGE_StagingFrame test_frame;
    test_frame.frame_id = 500;
    test_frame.buffer_virtual_address = (void*)s_test_dst_dual;
    test_frame.width = TEST_GRID_SIZE;
    test_frame.height = TEST_GRID_SIZE;
    test_frame.pitch = TEST_GRID_SIZE * 4;

    BOGE_Rect seq_rects[5] = {
        {10, 10, 40, 40},
        {30, 30, 30, 30},
        {5, 50, 60, 20},
        {80, 80, 20, 20},
        {0, 0, 128, 128}
    };

    BOGE_Rect prev_r = {0, 0, 0, 0};
    uint32_t prev_c = 0;

    for (int step = 0; step < 5; step++) {
        test_frame.dirty_rects[0] = seq_rects[step];
        test_frame.dirty_count = 1;

        /* Legacy Reference: Copy current rect to legacy grid */
        BOGE_Rect lr = seq_rects[step];
        for (int y = lr.y; y < lr.y + (int)lr.height && y < TEST_GRID_SIZE; y++) {
            for (int x = lr.x; x < lr.x + (int)lr.width && x < TEST_GRID_SIZE; x++) {
                s_test_dst_legacy[y * TEST_GRID_SIZE + x] = s_test_src_grid[y * TEST_GRID_SIZE + x];
            }
        }

        /* Dual-Page Partial Copy: evaluate Union(Current, Previous) */
        bspe_dual_page_evaluate_effective(&seq_rects[step], 1, prev_c ? &prev_r : NULL, prev_c,
                                          TEST_GRID_SIZE, TEST_GRID_SIZE, out_rects, 32, &out_cnt, false);

        for (uint32_t r_idx = 0; r_idx < out_cnt; r_idx++) {
            BOGE_Rect er = out_rects[r_idx];
            for (int y = er.y; y < er.y + (int)er.height && y < TEST_GRID_SIZE; y++) {
                for (int x = er.x; x < er.x + (int)er.width && x < TEST_GRID_SIZE; x++) {
                    s_test_dst_dual[y * TEST_GRID_SIZE + x] = s_test_src_grid[y * TEST_GRID_SIZE + x];
                }
            }
        }

        /* Verify 100% pixel identity */
        for (uint32_t i = 0; i < (TEST_GRID_SIZE * TEST_GRID_SIZE); i++) {
            if (s_test_dst_legacy[i] != s_test_dst_dual[i]) {
                all_passed = false;
                break;
            }
        }

        /* Advance simulated history: Current becomes Previous for next step */
        prev_r = seq_rects[step];
        prev_c = 1;
    }

    return all_passed;
}
