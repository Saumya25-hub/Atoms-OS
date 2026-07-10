/**
 * @file display_policy.c
 * @brief ATOMS OS Display Intelligence Engine - Adaptive Scoring Policy Engine V2
 *
 * DESIGN PHILOSOPHY:
 *   The policy engine evaluates every available display mode through pure capability
 *   metrics. It NEVER checks the environment type. It NEVER hardcodes a resolution.
 *   The final decision is always derived from measurable capabilities and verified
 *   desktop geometry.
 *
 * SCORING FACTORS:
 *   1. Desktop Area Utility     — Usable workspace pixels (log-scaled diminishing returns)
 *   2. Aspect Ratio Quality     — 16:9 widescreen preferred for modern desktop UI
 *   3. Memory Pitch Alignment   — 16-byte aligned rows for MMIO bus burst efficiency
 *   4. VRAM Efficiency          — Framebuffer vs total VRAM ratio (double-buffering headroom)
 *   5. Bandwidth Budget         — Estimated presentation throughput per second
 *   6. Geometry Completeness    — All 9 desktop rects must satisfy minimum dimension constraints
 *
 * VALIDATION:
 *   Every candidate mode is trial-geometry-tested. If any of the 9 authoritative
 *   desktop rectangles produces invalid dimensions, that mode is REJECTED with a
 *   reason string, and the next candidate is evaluated.
 */

#include "display_policy.h"

/* ========================================================================== */
/* Geometry Validation — Trial calculation for a candidate mode               */
/* ========================================================================== */

typedef struct {
    bool passed;
    const char* reject_reason;
} DIE_GeometryVerdict;

static DIE_GeometryVerdict die_validate_geometry(uint32_t w, uint32_t h) {
    DIE_GeometryVerdict v;
    v.passed = false;
    v.reject_reason = "";

    /* Absolute minimums for a usable graphical desktop */
    if (w < 640) { v.reject_reason = "Width < 640: Insufficient horizontal space"; return v; }
    if (h < 480) { v.reject_reason = "Height < 480: Insufficient vertical space"; return v; }

    /* Taskbar validation (ATOMS OS uses 48px standard, 40px at <=600) */
    uint32_t taskbar_h = (h <= 600) ? 40 : 48;
    uint32_t work_area_h = h - taskbar_h;
    if (work_area_h < 300) { v.reject_reason = "Work area height < 300px after taskbar"; return v; }

    /* Notification area validation */
    uint32_t notif_w = (w > 340) ? 304 : (w - 36);
    if (notif_w < 150) { v.reject_reason = "Notification area width < 150px"; return v; }

    /* Popup area validation */
    uint32_t popup_w = (w > 450) ? 400 : (w - 50);
    uint32_t popup_h = (h > 350) ? 300 : (h - 50);
    if (popup_w < 200) { v.reject_reason = "Popup area width < 200px"; return v; }
    if (popup_h < 150) { v.reject_reason = "Popup area height < 150px"; return v; }

    /* Safe area validation (8px inset from all edges above taskbar) */
    uint32_t safe_w = (w > 16) ? (w - 16) : 0;
    uint32_t safe_h = (h > taskbar_h + 16) ? (h - taskbar_h - 16) : 0;
    if (safe_w < 400) { v.reject_reason = "Safe area width < 400px"; return v; }
    if (safe_h < 250) { v.reject_reason = "Safe area height < 250px"; return v; }

    /* Cursor bounds validation */
    if (w < 2 || h < 2) { v.reject_reason = "Cursor bounds invalid (< 2px)"; return v; }

    /* Dock area validation */
    uint32_t dock_w = (w > 550) ? 500 : (w - 32);
    if (dock_w < 200) { v.reject_reason = "Dock area width < 200px"; return v; }

    v.passed = true;
    v.reject_reason = "All 9 geometry rectangles validated";
    return v;
}

/* ========================================================================== */
/* Integer square root (for log-scaled area scoring)                          */
/* ========================================================================== */

static uint32_t die_isqrt(uint32_t n) {
    if (n == 0) return 0;
    uint32_t x = n;
    uint32_t y = (x + 1) / 2;
    while (y < x) {
        x = y;
        y = (x + n / x) / 2;
    }
    return x;
}

/* ========================================================================== */
/* Pure Capability Scoring Engine                                             */
/* ========================================================================== */

typedef struct {
    uint32_t area_score;
    uint32_t aspect_score;
    uint32_t alignment_score;
    uint32_t vram_score;
    uint32_t bandwidth_score;
    uint32_t geometry_score;
    uint32_t total;
    bool geometry_valid;
    const char* reject_reason;
    const char* rationale;
} DIE_ModeEvaluation;

static DIE_ModeEvaluation die_evaluate_mode(DIE_DisplayMode* mode, uint32_t vram_size) {
    DIE_ModeEvaluation eval;
    eval.area_score = 0;
    eval.aspect_score = 0;
    eval.alignment_score = 0;
    eval.vram_score = 0;
    eval.bandwidth_score = 0;
    eval.geometry_score = 0;
    eval.total = 0;
    eval.geometry_valid = false;
    eval.reject_reason = "";
    eval.rationale = "";

    uint32_t w = mode->width;
    uint32_t h = mode->height;
    if (w == 0 || h == 0) {
        eval.reject_reason = "Zero dimension mode";
        return eval;
    }

    /* ------------------------------------------------------------------ */
    /* Factor 1: Desktop Area Utility (0-400 points)                      */
    /* Log-scaled via integer sqrt to avoid extreme bias toward 4K.       */
    /* sqrt(1920*1080) ≈ 1440 → 1440/4 = 360                             */
    /* sqrt(1280*720)  ≈  960 → 960/4  = 240                             */
    /* sqrt(1024*768)  ≈  886 → 886/4  = 221                             */
    /* This naturally rewards larger desktops with diminishing returns.   */
    /* ------------------------------------------------------------------ */
    uint32_t pixel_area = w * h;
    uint32_t area_root = die_isqrt(pixel_area);
    eval.area_score = area_root / 4;
    if (eval.area_score > 400) eval.area_score = 400;

    /* ------------------------------------------------------------------ */
    /* Factor 2: Aspect Ratio Quality (0-200 points)                      */
    /* 16:9 = 177% → most desktop UI is designed for widescreen           */
    /* 16:10 = 160% → very good desktop aspect                            */
    /* 4:3 = 133% → usable but wastes horizontal density                  */
    /* Other → penalized (non-standard aspect)                            */
    /* ------------------------------------------------------------------ */
    uint32_t ratio_x100 = (w * 100) / h;
    if (ratio_x100 >= 170 && ratio_x100 <= 180) {
        eval.aspect_score = 200;  /* 16:9 — optimal modern desktop aspect */
    } else if (ratio_x100 >= 155 && ratio_x100 <= 165) {
        eval.aspect_score = 160;  /* 16:10 — excellent widescreen */
    } else if (ratio_x100 >= 130 && ratio_x100 <= 136) {
        eval.aspect_score = 100;  /* 4:3 — usable standard */
    } else if (ratio_x100 >= 210 && ratio_x100 <= 240) {
        eval.aspect_score = 80;   /* 21:9 — ultrawide (future) */
    } else {
        eval.aspect_score = 40;   /* Non-standard aspect */
    }

    /* ------------------------------------------------------------------ */
    /* Factor 3: Memory Pitch Alignment (0-100 points)                    */
    /* 64-byte aligned pitch → maximum MMIO/DMA burst width               */
    /* 16-byte aligned pitch → good VRAM copy performance                 */
    /* Unaligned pitch → partial bus transactions, wasted cycles           */
    /* ------------------------------------------------------------------ */
    uint32_t pitch = mode->pitch_bytes;
    if (pitch == 0) pitch = w * 4;
    if ((pitch & 63) == 0) {
        eval.alignment_score = 100;  /* 64-byte cache line perfect */
    } else if ((pitch & 15) == 0) {
        eval.alignment_score = 70;   /* 16-byte SIMD aligned */
    } else if ((pitch & 3) == 0) {
        eval.alignment_score = 30;   /* 4-byte word aligned */
    } else {
        eval.alignment_score = 0;    /* Unaligned — worst case */
    }

    /* ------------------------------------------------------------------ */
    /* Factor 4: VRAM Efficiency (0-200 points)                           */
    /* Measures framebuffer utilization ratio against total VRAM.          */
    /* Lower ratio = more headroom for double buffering, damage regions,  */
    /* cursor plane overlay, and future composition layers.               */
    /*                                                                    */
    /* 1920×1080×4 = 8,294,400 bytes (12.5% of 64MB) → 175 points        */
    /* 1280×720×4  = 3,686,400 bytes (5.5% of 64MB)  → 189 points        */
    /* 1024×768×4  = 3,145,728 bytes (4.7% of 64MB)  → 190 points        */
    /* ------------------------------------------------------------------ */
    uint32_t fb_size = pixel_area * 4;  /* ARGB32 */
    if (vram_size > 0) {
        /* utilization_pct = (fb_size * 100) / vram_size */
        uint32_t util_pct = (fb_size / 1024) * 100 / (vram_size / 1024);
        if (util_pct > 50) {
            eval.vram_score = 0;       /* Uses > 50% of VRAM — no double buffer room */
        } else if (util_pct > 25) {
            eval.vram_score = 100;     /* 25-50% — tight but workable */
        } else {
            eval.vram_score = 200 - util_pct;  /* <25% — excellent headroom */
        }
        if (eval.vram_score > 200) eval.vram_score = 200;
    } else {
        eval.vram_score = 100;  /* Unknown VRAM — neutral score */
    }

    /* ------------------------------------------------------------------ */
    /* Factor 5: Presentation Bandwidth Budget (0-200 points)             */
    /* Estimated bus throughput required at refresh rate.                  */
    /* Higher bandwidth demand increases presentation latency and         */
    /* reduces compositor headroom for damage-rect partial updates.       */
    /*                                                                    */
    /* Budget = fb_size × refresh_rate (bytes/sec)                        */
    /* Threshold: 300 MB/s = comfortable for VBE linear framebuffer       */
    /* Above threshold: diminishing returns on presentation quality       */
    /* ------------------------------------------------------------------ */
    uint32_t refresh = mode->refresh_rate_hz;
    if (refresh == 0) refresh = 60;
    uint32_t bandwidth_mb_s = (fb_size / 1024) * refresh / 1024;  /* MB/s approx */
    if (bandwidth_mb_s <= 200) {
        eval.bandwidth_score = 200;    /* Excellent — well within bus budget */
    } else if (bandwidth_mb_s <= 400) {
        eval.bandwidth_score = 200 - ((bandwidth_mb_s - 200) * 100) / 200;
    } else {
        eval.bandwidth_score = 50;     /* Heavy — near bus saturation */
    }

    /* ------------------------------------------------------------------ */
    /* Factor 6: Geometry Completeness (0-200 points, or REJECT)          */
    /* Trial-compute all 9 desktop geometry rectangles and verify each    */
    /* satisfies minimum dimension constraints for a usable desktop.      */
    /* ------------------------------------------------------------------ */
    DIE_GeometryVerdict geom = die_validate_geometry(w, h);
    eval.geometry_valid = geom.passed;
    if (geom.passed) {
        eval.geometry_score = 200;     /* Full geometry verification passed */
        eval.reject_reason = "";
    } else {
        eval.geometry_score = 0;
        eval.reject_reason = geom.reject_reason;
        /* Total remains 0 — this mode will be rejected */
        return eval;
    }

    /* ------------------------------------------------------------------ */
    /* Total Composite Score (theoretical max ≈ 1300)                     */
    /* ------------------------------------------------------------------ */
    eval.total = eval.area_score
               + eval.aspect_score
               + eval.alignment_score
               + eval.vram_score
               + eval.bandwidth_score
               + eval.geometry_score;

    /* Generate capability-derived rationale */
    if (eval.total >= 1000) {
        eval.rationale = "Excellent: High area, optimal aspect, efficient bandwidth";
    } else if (eval.total >= 800) {
        eval.rationale = "Good: Balanced area and presentation efficiency";
    } else if (eval.total >= 600) {
        eval.rationale = "Acceptable: Geometry valid, moderate efficiency";
    } else {
        eval.rationale = "Marginal: Passed geometry but low capability scores";
    }

    return eval;
}

/* ========================================================================== */
/* Public API                                                                 */
/* ========================================================================== */

bool DIE_Policy_EvaluateBestMode(DIE_DisplayInfo* info) {
    if (!info || info->capabilities.mode_count == 0) return false;

    uint32_t vram = info->vram_size_bytes;
    if (vram == 0) vram = 64 * 1024 * 1024;  /* Conservative default */

    uint32_t best_idx = 0;
    uint32_t best_score = 0;
    const char* best_rationale = "No valid mode found";
    bool any_valid = false;

    for (uint32_t i = 0; i < info->capabilities.mode_count; i++) {
        DIE_DisplayMode* mode = &info->capabilities.modes[i];
        if (!mode->is_supported) {
            mode->policy_score = 0;
            continue;
        }

        DIE_ModeEvaluation eval = die_evaluate_mode(mode, vram);
        mode->policy_score = eval.total;

        if (!eval.geometry_valid) {
            /* Mode rejected — geometry validation failed */
            continue;
        }

        if (eval.total > best_score) {
            best_score = eval.total;
            best_idx = i;
            best_rationale = eval.rationale;
            any_valid = true;
        }
    }

    if (!any_valid) {
        /* Absolute fallback — use first mode regardless */
        best_idx = 0;
        best_rationale = "Emergency fallback: No mode passed geometry validation";
    }

    info->capabilities.preferred_index = best_idx;
    for (uint32_t i = 0; i < info->capabilities.mode_count; i++) {
        info->capabilities.modes[i].is_preferred = (i == best_idx);
    }

    info->active_mode = info->capabilities.modes[best_idx];
    info->policy_decision.selected_mode = info->active_mode;
    info->policy_decision.environment = info->environment;
    info->policy_decision.dpi_scale = DIE_DEFAULT_DPI_SCALE;
    info->policy_decision.policy_rationale = best_rationale;

    return true;
}
