/**
 * @file agdte_vsync.c
 * @brief ATOMEGearDisplayTrainEngine (AGDTE) Native VSync Abstraction Layer
 * @status Phase 4 Display Timing Optimization Layer
 *
 * @section PURPOSE
 * Decouples scheduler synchronization requests (`RequestSync`) from backend hardware
 * VSync execution (`OnVBI_IRQ`, `query_vsync`). Provides precise vertical blanking
 * interval (`vbi_interval_us`) tracking for VBE, VMware SVGA II, VirtIO, and future GPUs.
 */

#include "../include/agdte.h"

static AGDTE_VSyncState s_vsync_states[AGDTE_MAX_DISPLAYS];

AGDTE_Error AGDTE_VSync_Init(uint32_t display_id) {
    if (display_id >= AGDTE_MAX_DISPLAYS) {
        return AGDTE_ERR_INVALID_DISPLAY;
    }

    AGDTE_VSyncState* vs = &s_vsync_states[display_id];
    vs->display_id = display_id;
    vs->status = AGDTE_VSYNC_STATE_IDLE;
    vs->requested_target_us = 0;
    vs->last_vbi_timestamp_us = 0;
    vs->vbi_interval_us = 16666; /* Default 60Hz interval */
    vs->irq_trigger_count = 0;
    vs->enabled = true;

    return AGDTE_OK;
}

AGDTE_Error AGDTE_VSync_RequestSync(uint32_t display_id, uint64_t target_time_us) {
    if (display_id >= AGDTE_MAX_DISPLAYS) {
        return AGDTE_ERR_INVALID_DISPLAY;
    }

    AGDTE_VSyncState* vs = &s_vsync_states[display_id];
    if (!vs->enabled) {
        return AGDTE_ERR_UNSUPPORTED;
    }

    vs->requested_target_us = target_time_us;
    vs->status = AGDTE_VSYNC_STATE_REQUESTED;
    return AGDTE_OK;
}

AGDTE_Error AGDTE_VSync_OnVBI_IRQ(uint32_t display_id, uint64_t irq_timestamp_us) {
    if (display_id >= AGDTE_MAX_DISPLAYS) {
        return AGDTE_ERR_INVALID_DISPLAY;
    }

    AGDTE_VSyncState* vs = &s_vsync_states[display_id];
    if (vs->last_vbi_timestamp_us > 0 && irq_timestamp_us > vs->last_vbi_timestamp_us) {
        uint64_t interval = irq_timestamp_us - vs->last_vbi_timestamp_us;
        /* Smooth filter: 75% old interval + 25% new interval */
        vs->vbi_interval_us = (vs->vbi_interval_us * 3 + interval) / 4;
    }

    vs->last_vbi_timestamp_us = irq_timestamp_us;
    vs->irq_trigger_count++;

    if (vs->status == AGDTE_VSYNC_STATE_REQUESTED) {
        if (vs->requested_target_us == 0 || irq_timestamp_us >= vs->requested_target_us) {
            vs->status = AGDTE_VSYNC_STATE_TRIGGERED;
        }
    }

    /* Keep display state in sync */
    AGDTE_DisplayState* disp = AGDTE_Display_GetState(display_id);
    if (disp) {
        disp->last_vbi_timestamp_us = irq_timestamp_us;
    }

    return AGDTE_OK;
}

AGDTE_Error AGDTE_VSync_Poll(uint32_t display_id, uint64_t current_time_us) {
    if (display_id >= AGDTE_MAX_DISPLAYS) {
        return AGDTE_ERR_INVALID_DISPLAY;
    }

    AGDTE_VSyncState* vs = &s_vsync_states[display_id];
    AGDTE_DisplayState* disp = AGDTE_Display_GetState(display_id);
    if (!disp || !disp->active) {
        return AGDTE_ERR_INVALID_DISPLAY;
    }

    const AGDTE_BackendOps* ops = AGDTE_Backend_GetOps(disp->backend_type);
    if (ops && ops->query_vsync) {
        bool vbi_active = false;
        uint64_t vbi_ts = 0;
        if (ops->query_vsync(display_id, &vbi_active, &vbi_ts) == AGDTE_OK && vbi_active) {
            AGDTE_VSync_OnVBI_IRQ(display_id, (vbi_ts > 0) ? vbi_ts : current_time_us);
            return AGDTE_OK;
        }
    }

    /* Fallback simulation based on known cadence if hardware VBI IRQ not active */
    if (vs->last_vbi_timestamp_us == 0) {
        vs->last_vbi_timestamp_us = current_time_us;
    } else if (current_time_us >= vs->last_vbi_timestamp_us + vs->vbi_interval_us) {
        uint64_t ticks_passed = (current_time_us - vs->last_vbi_timestamp_us) / vs->vbi_interval_us;
        uint64_t sim_vbi = vs->last_vbi_timestamp_us + (ticks_passed * vs->vbi_interval_us);
        AGDTE_VSync_OnVBI_IRQ(display_id, sim_vbi);
    }

    return AGDTE_OK;
}

bool AGDTE_VSync_IsSyncReady(uint32_t display_id, uint64_t current_time_us) {
    if (display_id >= AGDTE_MAX_DISPLAYS) {
        return true;
    }
    AGDTE_DisplayState* disp = AGDTE_Display_GetState(display_id);
    if (!disp || disp->cadence_mode != AGDTE_CADENCE_VSYNC_IRQ) {
        /* For fixed interval cadence (60Hz, 75Hz, 144Hz, Immediate), Frame Pacer governs deadlines */
        return true;
    }
    AGDTE_VSyncState* vs = &s_vsync_states[display_id];
    if (vs->status == AGDTE_VSYNC_STATE_TRIGGERED) {
        vs->status = AGDTE_VSYNC_STATE_IDLE;
        return true;
    }
    if (vs->requested_target_us > 0 && current_time_us >= vs->requested_target_us) {
        vs->status = AGDTE_VSYNC_STATE_IDLE;
        return true;
    }
    if (vs->requested_target_us == 0 && current_time_us >= vs->last_vbi_timestamp_us) {
        return true;
    }
    return false;
}

AGDTE_VSyncState* AGDTE_VSync_GetState(uint32_t display_id) {
    if (display_id >= AGDTE_MAX_DISPLAYS) {
        return 0;
    }
    return &s_vsync_states[display_id];
}
