/**
 * @file agdte_refresh_controller.c
 * @brief ATOMEGearDisplayTrainEngine (AGDTE) Refresh Coordination Controller
 * @status Phase 4 Display Timing Optimization Layer
 *
 * @section PURPOSE
 * Coordinates refresh rate profiles (`AGDTE_RefreshProfile`) and display sync modes
 * across multi-monitor configurations without dynamic allocation or floating point math.
 */

#include "../include/agdte.h"

static AGDTE_RefreshProfile s_profiles[AGDTE_MAX_DISPLAYS];

AGDTE_Error AGDTE_RefreshController_Init(void) {
    for (uint32_t i = 0; i < AGDTE_MAX_DISPLAYS; i++) {
        s_profiles[i].display_id = i;
        s_profiles[i].current_refresh_hz = 60;
        s_profiles[i].cadence_mode = AGDTE_CADENCE_60HZ_FIXED;
        s_profiles[i].refresh_interval_us = 16666;
        s_profiles[i].synchronized = false;
    }
    return AGDTE_OK;
}

AGDTE_Error AGDTE_RefreshController_SetProfile(uint32_t display_id, uint32_t refresh_hz, AGDTE_CadenceMode mode) {
    if (display_id >= AGDTE_MAX_DISPLAYS) {
        return AGDTE_ERR_INVALID_DISPLAY;
    }

    AGDTE_RefreshProfile* prof = &s_profiles[display_id];
    prof->current_refresh_hz = (refresh_hz > 0) ? refresh_hz : 60;
    prof->cadence_mode = mode;

    switch (prof->current_refresh_hz) {
        case 60:  prof->refresh_interval_us = 16666; break;
        case 75:  prof->refresh_interval_us = 13333; break;
        case 90:  prof->refresh_interval_us = 11111; break;
        case 120: prof->refresh_interval_us = 8333;  break;
        case 144: prof->refresh_interval_us = 6944;  break;
        case 240: prof->refresh_interval_us = 4166;  break;
        default:  prof->refresh_interval_us = 1000000ULL / prof->current_refresh_hz; break;
    }

    prof->synchronized = true;
    AGDTE_Pacer_SetRate(display_id, prof->current_refresh_hz);

    return AGDTE_OK;
}

uint64_t AGDTE_RefreshController_GetIntervalUs(uint32_t display_id) {
    if (display_id >= AGDTE_MAX_DISPLAYS) {
        return 16666;
    }
    return s_profiles[display_id].refresh_interval_us;
}

AGDTE_Error AGDTE_RefreshController_SynchronizeTimers(uint64_t current_time_us) {
    for (uint32_t i = 0; i < AGDTE_MAX_DISPLAYS; i++) {
        if (s_profiles[i].synchronized) {
            AGDTE_VSync_Poll(i, current_time_us);
        }
    }
    return AGDTE_OK;
}

AGDTE_RefreshProfile* AGDTE_RefreshController_GetProfile(uint32_t display_id) {
    if (display_id >= AGDTE_MAX_DISPLAYS) {
        return 0;
    }
    return &s_profiles[display_id];
}
