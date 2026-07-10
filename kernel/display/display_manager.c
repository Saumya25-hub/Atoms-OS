/**
 * @file display_manager.c
 * @brief ATOMS OS Display Intelligence Engine (DIE) V1 Master Bridge Implementation
 */

#include "display_manager.h"
#include "display_detection.h"
#include "display_capabilities.h"
#include "display_policy.h"
#include "display_geometry.h"
#include "display_layout.h"
#include "display_diag.h"

static DIE_DisplayInfo s_active_displays[DIE_MAX_DISPLAYS];
static bool s_die_initialized = false;

void DIE_Initialize(void) {
    if (s_die_initialized) return;

    /* Initialize primary display structure inside static pool */
    for (uint32_t i = 0; i < DIE_MAX_DISPLAYS; i++) {
        s_active_displays[i].display_id = i;
        s_active_displays[i].is_active = false;
    }

    DIE_DisplayInfo* primary = &s_active_displays[0];
    primary->is_active = true;
    primary->dpi_scale = DIE_DEFAULT_DPI_SCALE;
    primary->last_reconfigure_time_us = 0;

    /* 1. Detect physical hardware or virtual machine environment */
    DIE_Detection_DetectEnvironment(&primary->environment, primary->environment_name, primary->controller_name);

    /* 2. Collect display capabilities and VRAM parameters */
    DIE_Capabilities_Collect(primary);

    /* 3. Evaluate display policy to select optimal resolution and pitch */
    DIE_EvaluateAndApplyPolicy(0);

    /* 4. Authoritatively calculate all 9 display geometry rectangles */
    DIE_Geometry_Calculate(primary);

    /* 5. Synchronize geometry and capabilities across kernel, WM, and AGDTE */
    DIE_SyncWithKernelAndAGDTE(0);

    s_die_initialized = true;

    /* 6. Dump official diagnostics to boot console */
    DIE_Diag_Dump(0);
}

DIE_DisplayInfo* DIE_GetPrimaryDisplay(void) {
    return &s_active_displays[0];
}

DIE_DisplayInfo* DIE_GetDisplay(uint32_t display_id) {
    if (display_id >= DIE_MAX_DISPLAYS) return 0;
    return &s_active_displays[display_id];
}

const DIE_DisplayGeometry* DIE_GetPrimaryGeometry(void) {
    return &s_active_displays[0].geometry;
}

bool DIE_EvaluateAndApplyPolicy(uint32_t display_id) {
    DIE_DisplayInfo* info = DIE_GetDisplay(display_id);
    if (!info || !info->is_active) return false;

    bool ok = DIE_Policy_EvaluateBestMode(info);
    if (ok) {
        DIE_Geometry_Calculate(info);
    }
    return ok;
}

void DIE_SyncWithKernelAndAGDTE(uint32_t display_id) {
    DIE_DisplayInfo* info = DIE_GetDisplay(display_id);
    if (!info || !info->is_active) return;

    DIE_Layout_UpdateSubsystems(info);
}
