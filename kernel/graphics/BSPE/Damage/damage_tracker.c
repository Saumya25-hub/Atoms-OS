/**
 * @file damage_tracker.c
 * @brief BSPE Damage Tracker Production Implementation
 * @status Step 6 Production Implementation
 * 
 * @section PURPOSE
 * Implements hierarchical damage merging and dual-page history tracking:
 * EffectiveDamage = Union(Damage(N), Damage(N-1)).
 * Operates purely on rectangle mathematics without heap allocation, recursion, or VRAM copies.
 */

#include "damage_tracker.h"
#include <stddef.h>

#define BSPE_DT_MAX_RECTS     32
#define BSPE_DT_MAX_INSTANCES 4

typedef struct {
    BOGE_Rect rects[BSPE_DT_MAX_RECTS];
    uint32_t count;
} BSPE_DamagePage;

typedef struct BSPE_DamageTracker_T {
    BSPE_DamagePage history[2]; /* [0] = Page A, [1] = Page B */
    uint32_t current_page;      /* 0 or 1 */
    uint32_t screen_width;
    uint32_t screen_height;
    bool is_allocated;
} BSPE_DamageTrackerInstance;

static BSPE_DamageTrackerInstance g_dt_pool[BSPE_DT_MAX_INSTANCES];

/* --- Pure Rectangle Mathematics Helpers (No Recursion) --- */

static inline int32_t dt_min(int32_t a, int32_t b) { return (a < b) ? a : b; }
static inline int32_t dt_max(int32_t a, int32_t b) { return (a > b) ? a : b; }

static inline int32_t dt_left(const BOGE_Rect* r) { return r->x; }
static inline int32_t dt_top(const BOGE_Rect* r) { return r->y; }
static inline int32_t dt_right(const BOGE_Rect* r) { return r->x + (int32_t)r->width; }
static inline int32_t dt_bottom(const BOGE_Rect* r) { return r->y + (int32_t)r->height; }

static inline BOGE_Rect dt_make_rect(int32_t l, int32_t t, int32_t r, int32_t b) {
    BOGE_Rect res;
    res.x = l;
    res.y = t;
    res.width = (r > l) ? (uint32_t)(r - l) : 0;
    res.height = (b > t) ? (uint32_t)(b - t) : 0;
    return res;
}

static inline bool dt_rect_is_valid(const BOGE_Rect* r) {
    return (r && r->width > 0 && r->height > 0);
}

static inline void dt_rect_clip(BOGE_Rect* r, int32_t max_w, int32_t max_h) {
    int32_t l = dt_left(r);
    int32_t t = dt_top(r);
    int32_t rt = dt_right(r);
    int32_t b = dt_bottom(r);
    
    if (l < 0) l = 0;
    if (t < 0) t = 0;
    if (rt > max_w) rt = max_w;
    if (b > max_h) b = max_h;
    
    *r = dt_make_rect(l, t, rt, b);
}

static inline uint32_t dt_rect_area(const BOGE_Rect* r) {
    if (!dt_rect_is_valid(r)) return 0;
    return r->width * r->height;
}

static inline bool dt_rects_overlap_or_touch(const BOGE_Rect* a, const BOGE_Rect* b) {
    /* Touch or overlap if edges meet or intersect */
    return (dt_left(a) <= dt_right(b) && dt_left(b) <= dt_right(a) &&
            dt_top(a) <= dt_bottom(b) && dt_top(b) <= dt_bottom(a));
}

static inline BOGE_Rect dt_rect_union(const BOGE_Rect* a, const BOGE_Rect* b) {
    int32_t l = dt_min(dt_left(a), dt_left(b));
    int32_t t = dt_min(dt_top(a), dt_top(b));
    int32_t rt = dt_max(dt_right(a), dt_right(b));
    int32_t bm = dt_max(dt_bottom(a), dt_bottom(b));
    return dt_make_rect(l, t, rt, bm);
}

/**
 * @brief Iterative non-recursive algorithm to merge a rectangle into a damage page.
 * Maintains disjoint property: No two rectangles in the list overlap or touch.
 */
static void dt_page_merge_rect(BSPE_DamagePage* page, const BOGE_Rect* new_r) {
    if (!dt_rect_is_valid(new_r)) return;
    
    BOGE_Rect r = *new_r;
    bool merged_any = true;
    
    /* Iterative merging loop (Guaranteed termination without recursion) */
    while (merged_any) {
        merged_any = false;
        for (uint32_t i = 0; i < page->count; i++) {
            if (dt_rects_overlap_or_touch(&page->rects[i], &r)) {
                r = dt_rect_union(&page->rects[i], &r);
                /* Remove slot i by swapping with the last slot */
                page->rects[i] = page->rects[page->count - 1];
                page->count--;
                merged_any = true;
                break; /* Restart scan against updated r */
            }
        }
    }
    
    /* If there is space, append the disjoint rectangle */
    if (page->count < BSPE_DT_MAX_RECTS) {
        page->rects[page->count++] = r;
        return;
    }
    
    /* Fixed Memory Fallback: Array saturated (count == 32).
     * Find the pair of rectangles (including r) whose union introduces minimal extra wasted area.
     */
    uint32_t min_extra_area = 0xFFFFFFFF;
    uint32_t best_i = 0;
    
    for (uint32_t i = 0; i < page->count; i++) {
        BOGE_Rect u = dt_rect_union(&page->rects[i], &r);
        uint32_t extra = dt_rect_area(&u) - dt_rect_area(&page->rects[i]) - dt_rect_area(&r);
        if (extra < min_extra_area) {
            min_extra_area = extra;
            best_i = i;
        }
    }
    
    /* Merge r into best_i */
    page->rects[best_i] = dt_rect_union(&page->rects[best_i], &r);
}

/* --- Public Lifecycle Implementations --- */

BSPE_Error BSPE_DamageTracker_Create(const BSPE_DamageTrackerConfig* config, BSPE_DamageTrackerHandle* out_handle) {
    if (!out_handle) return BSPE_ERR_NULL_POINTER;
    
    for (uint32_t i = 0; i < BSPE_DT_MAX_INSTANCES; i++) {
        if (!g_dt_pool[i].is_allocated) {
            g_dt_pool[i].is_allocated = true;
            g_dt_pool[i].current_page = 0;
            g_dt_pool[i].screen_width  = config ? (config->screen_width ? config->screen_width : 1920) : 1920;
            g_dt_pool[i].screen_height = config ? (config->screen_height ? config->screen_height : 1080) : 1080;
            g_dt_pool[i].history[0].count = 0;
            g_dt_pool[i].history[1].count = 0;
            *out_handle = (BSPE_DamageTrackerHandle)&g_dt_pool[i];
            return BSPE_OK;
        }
    }
    return BSPE_ERR_OUT_OF_MEMORY;
}

void BSPE_DamageTracker_Destroy(BSPE_DamageTrackerHandle handle) {
    if (!handle) return;
    BSPE_DamageTrackerInstance* dt = (BSPE_DamageTrackerInstance*)handle;
    dt->is_allocated = false;
    dt->history[0].count = 0;
    dt->history[1].count = 0;
}

void BSPE_DamageTracker_Reset(BSPE_DamageTrackerHandle handle) {
    if (!handle) return;
    BSPE_DamageTrackerInstance* dt = (BSPE_DamageTrackerInstance*)handle;
    dt->history[0].count = 0;
    dt->history[1].count = 0;
    dt->current_page = 0;
}

void BSPE_DamageTracker_Clear(BSPE_DamageTrackerHandle handle) {
    if (!handle) return;
    BSPE_DamageTrackerInstance* dt = (BSPE_DamageTrackerInstance*)handle;
    dt->history[dt->current_page].count = 0;
}

void BSPE_DamageTracker_AdvanceFrame(BSPE_DamageTrackerHandle handle) {
    if (!handle) return;
    BSPE_DamageTrackerInstance* dt = (BSPE_DamageTrackerInstance*)handle;
    
    /* Toggle active page: old N becomes N-1; old N-1 becomes new N */
    dt->current_page = 1 - dt->current_page;
    /* Clear the new frame N page */
    dt->history[dt->current_page].count = 0;
}

/* --- Core Damage Submission Implementations --- */

BSPE_Error BSPE_DamageTracker_AddRect(BSPE_DamageTrackerHandle handle, const BOGE_Rect* rect) {
    if (!handle || !rect) return BSPE_ERR_NULL_POINTER;
    BSPE_DamageTrackerInstance* dt = (BSPE_DamageTrackerInstance*)handle;
    
    BOGE_Rect r = *rect;
    dt_rect_clip(&r, (int32_t)dt->screen_width, (int32_t)dt->screen_height);
    if (!dt_rect_is_valid(&r)) return BSPE_OK;
    
    dt_page_merge_rect(&dt->history[dt->current_page], &r);
    return BSPE_OK;
}

BSPE_Error BSPE_DamageTracker_AddRegion(BSPE_DamageTrackerHandle handle, const BOGE_Region* region) {
    if (!handle || !region) return BSPE_ERR_NULL_POINTER;
    for (uint32_t i = 0; i < region->rect_count; i++) {
        BSPE_DamageTracker_AddRect(handle, &region->rects[i]);
    }
    return BSPE_OK;
}

BSPE_Error BSPE_DamageTracker_AddWindowDamage(BSPE_DamageTrackerHandle handle, uint32_t window_id, const BOGE_Rect* rect) {
    (void)window_id; /* Pure rectangle math: window ID stripped at BSPE transport boundary */
    return BSPE_DamageTracker_AddRect(handle, rect);
}

uint32_t BSPE_DamageTracker_GetDamageCount(BSPE_DamageTrackerHandle handle) {
    if (!handle) return 0;
    BSPE_DamageTrackerInstance* dt = (BSPE_DamageTrackerInstance*)handle;
    return dt->history[dt->current_page].count;
}

/* --- Effective Damage Evaluation: Union(Damage(N), Damage(N-1)) --- */

BSPE_Error BSPE_DamageTracker_GetEffectiveDamage(BSPE_DamageTrackerHandle handle, BOGE_Rect* out_rects, uint32_t max_rects, uint32_t* out_count) {
    if (!handle || !out_rects || !out_count) return BSPE_ERR_NULL_POINTER;
    BSPE_DamageTrackerInstance* dt = (BSPE_DamageTrackerInstance*)handle;
    
    /* Construct temporary union page on stack (No heap allocation) */
    BSPE_DamagePage union_page;
    union_page.count = 0;
    
    /* 1. Add all rectangles from Damage(N) */
    BSPE_DamagePage* page_n = &dt->history[dt->current_page];
    for (uint32_t i = 0; i < page_n->count; i++) {
        dt_page_merge_rect(&union_page, &page_n->rects[i]);
    }
    
    /* 2. Add all rectangles from Damage(N-1) */
    BSPE_DamagePage* page_n1 = &dt->history[1 - dt->current_page];
    for (uint32_t i = 0; i < page_n1->count; i++) {
        dt_page_merge_rect(&union_page, &page_n1->rects[i]);
    }
    
    /* Output to caller */
    uint32_t copy_count = (union_page.count < max_rects) ? union_page.count : max_rects;
    for (uint32_t i = 0; i < copy_count; i++) {
        out_rects[i] = union_page.rects[i];
    }
    *out_count = copy_count;
    return BSPE_OK;
}

BSPE_Error BSPE_DamageTracker_GetPreviousDamage(BSPE_DamageTrackerHandle handle, BOGE_Rect* out_rects, uint32_t max_rects, uint32_t* out_count) {
    if (!handle || !out_rects || !out_count) return BSPE_ERR_NULL_POINTER;
    BSPE_DamageTrackerInstance* dt = (BSPE_DamageTrackerInstance*)handle;
    
    BSPE_DamagePage* page_n1 = &dt->history[1 - dt->current_page];
    uint32_t copy_count = (page_n1->count < max_rects) ? page_n1->count : max_rects;
    for (uint32_t i = 0; i < copy_count; i++) {
        out_rects[i] = page_n1->rects[i];
    }
    *out_count = copy_count;
    return BSPE_OK;
}

/* --- Self-Test Verification Suite --- */

/* Simple pseudorandom generator for stress testing without stdlib */
static uint32_t dt_rand(uint32_t* seed) {
    *seed = (*seed * 1103515245 + 12345) & 0x7FFFFFFF;
    return *seed;
}

bool BSPE_DamageTracker_RunSelfTest(void) {
    BSPE_DamageTrackerHandle dt = NULL;
    BSPE_DamageTrackerConfig cfg = { .screen_width = 1024, .screen_height = 768 };
    if (BSPE_DamageTracker_Create(&cfg, &dt) != BSPE_OK || !dt) return false;
    
    /* 1. Overlap Test: Rect(10,10, w=40, h=40) + Rect(30,30, w=50, h=50) -> One merged rect (10,10, w=70, h=70) */
    BOGE_Rect r1 = {10, 10, 40, 40};
    BOGE_Rect r2 = {30, 30, 50, 50};
    BSPE_DamageTracker_AddRect(dt, &r1);
    BSPE_DamageTracker_AddRect(dt, &r2);
    if (BSPE_DamageTracker_GetDamageCount(dt) != 1) return false;
    
    /* 2. Adjacent Merge Test: Rect(0,0, w=100, h=50) + Rect(0,50, w=100, h=50) -> One merged rect (0,0, w=100, h=100) */
    BSPE_DamageTracker_Clear(dt);
    BOGE_Rect r3 = {0, 0, 100, 50};
    BOGE_Rect r4 = {0, 50, 100, 50};
    BSPE_DamageTracker_AddRect(dt, &r3);
    BSPE_DamageTracker_AddRect(dt, &r4);
    if (BSPE_DamageTracker_GetDamageCount(dt) != 1) return false;
    
    /* 3. Rectangle Merge Test: Add 3 disjoint, then 1 overlapping all 3 */
    BSPE_DamageTracker_Clear(dt);
    BOGE_Rect d1 = {0, 0, 10, 10};
    BOGE_Rect d2 = {20, 20, 10, 10};
    BOGE_Rect d3 = {40, 40, 10, 10};
    BSPE_DamageTracker_AddRect(dt, &d1);
    BSPE_DamageTracker_AddRect(dt, &d2);
    BSPE_DamageTracker_AddRect(dt, &d3);
    if (BSPE_DamageTracker_GetDamageCount(dt) != 3) return false;
    
    BOGE_Rect d4 = {5, 5, 40, 40};
    BSPE_DamageTracker_AddRect(dt, &d4);
    if (BSPE_DamageTracker_GetDamageCount(dt) != 1) return false;
    
    /* 4. Dual-Page History Verification */
    BSPE_DamageTracker_Reset(dt);
    BOGE_Rect p_n1 = {0, 0, 10, 10};
    BSPE_DamageTracker_AddRect(dt, &p_n1); /* Frame N */
    BSPE_DamageTracker_AdvanceFrame(dt);   /* Now Frame N+1 */
    BOGE_Rect p_n = {100, 100, 20, 20};
    BSPE_DamageTracker_AddRect(dt, &p_n);
    
    BOGE_Rect eff[32];
    uint32_t eff_count = 0;
    BSPE_DamageTracker_GetEffectiveDamage(dt, eff, 32, &eff_count);
    if (eff_count != 2) return false; /* Must contain both p_n1 and p_n! */
    
    BSPE_DamageTracker_AdvanceFrame(dt); /* Now Frame N+2 (p_n1 drops out) */
    BSPE_DamageTracker_GetEffectiveDamage(dt, eff, 32, &eff_count);
    if (eff_count != 1) return false; /* Only p_n remains! */
    
    /* 5. Random Stress Test (500 random rectangles) */
    BSPE_DamageTracker_Reset(dt);
    uint32_t seed = 999;
    for (int i = 0; i < 500; i++) {
        int32_t x = (int32_t)(dt_rand(&seed) % 900);
        int32_t y = (int32_t)(dt_rand(&seed) % 700);
        int32_t w = (int32_t)(dt_rand(&seed) % 100) + 1;
        int32_t h = (int32_t)(dt_rand(&seed) % 100) + 1;
        BOGE_Rect rand_r = { x, y, (uint32_t)w, (uint32_t)h };
        BSPE_DamageTracker_AddRect(dt, &rand_r);
        
        /* Verify invariants: count <= 32 and no two rectangles touch/overlap */
        uint32_t c = BSPE_DamageTracker_GetDamageCount(dt);
        if (c > BSPE_DT_MAX_RECTS) return false;
        BSPE_DamagePage* cur_p = &dt->history[dt->current_page];
        for (uint32_t a = 0; a < c; a++) {
            for (uint32_t b = a + 1; b < c; b++) {
                if (dt_rects_overlap_or_touch(&cur_p->rects[a], &cur_p->rects[b])) return false;
            }
        }
    }
    
    BSPE_DamageTracker_Destroy(dt);
    return true;
}

#ifdef BSPE_TEST_HARNESS
#include <stdio.h>
int main(void) {
    printf("[BSPE Test] Running Damage Tracker Self-Test Suite...\n");
    if (BSPE_DamageTracker_RunSelfTest()) {
        printf("[BSPE Test] ALL TESTS PASSED: Overlap, Adjacent, Merge, Dual-Page, Random Stress!\n");
        return 0;
    } else {
        printf("[BSPE Test] SELF-TEST FAILED!\n");
        return 1;
    }
}
#endif
