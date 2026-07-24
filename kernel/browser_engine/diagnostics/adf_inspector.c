#include "adf_inspector.h"
#include "adf_core.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/wm/bwe/include/bwe.h"

extern void display_print(const char* s);

static bool s_overlay_active = false;
static ABE_InspectorOverlayFlags s_flags = {
    .show_layout_boxes = true,
    .show_repaint_regions = true,
    .show_dirty_rects = true,
    .show_layer_borders = true,
    .show_clip_regions = true,
    .show_fps_hud = true,
    .show_gpu_uploads = true
};

void ABE_DebugOverlayEnable(void) {
    s_overlay_active = true;
    display_print("[ADF] Visual Render Inspector Overlay ENABLED.\n");
}

void ABE_DebugOverlayDisable(void) {
    s_overlay_active = false;
    display_print("[ADF] Visual Render Inspector Overlay DISABLED.\n");
}

bool ABE_DebugOverlayIsActive(void) {
    return s_overlay_active;
}

ABE_InspectorOverlayFlags ABE_DebugGetOverlayFlags(void) {
    return s_flags;
}

bool ABE_DebugProcessCommand(const char* cmd_str) {
    if (!cmd_str) return false;

    if (strcmp(cmd_str, "debug gpu") == 0) {
        ABE_DebugEnableSubsystem(ABE_SUB_GPU);
        ABE_DebugLog(ABE_SUB_GPU, ABE_DEBUG_INFO, "Live Debug Command Executed: debug gpu (ON)");
        return true;
    } else if (strcmp(cmd_str, "debug canvas") == 0) {
        ABE_DebugEnableSubsystem(ABE_SUB_CANVAS);
        ABE_DebugLog(ABE_SUB_CANVAS, ABE_DEBUG_INFO, "Live Debug Command Executed: debug canvas (ON)");
        return true;
    } else if (strcmp(cmd_str, "debug wasm") == 0) {
        ABE_DebugEnableSubsystem(ABE_SUB_WASM);
        ABE_DebugLog(ABE_SUB_WASM, ABE_DEBUG_INFO, "Live Debug Command Executed: debug wasm (ON)");
        return true;
    } else if (strcmp(cmd_str, "debug layout") == 0) {
        ABE_DebugEnableSubsystem(ABE_SUB_LAYOUT);
        ABE_DebugLog(ABE_SUB_LAYOUT, ABE_DEBUG_INFO, "Live Debug Command Executed: debug layout (ON)");
        return true;
    } else if (strcmp(cmd_str, "debug css") == 0) {
        ABE_DebugEnableSubsystem(ABE_SUB_CSS);
        ABE_DebugLog(ABE_SUB_CSS, ABE_DEBUG_INFO, "Live Debug Command Executed: debug css (ON)");
        return true;
    } else if (strcmp(cmd_str, "debug js") == 0) {
        ABE_DebugEnableSubsystem(ABE_SUB_JAVASCRIPT);
        ABE_DebugLog(ABE_SUB_JAVASCRIPT, ABE_DEBUG_INFO, "Live Debug Command Executed: debug js (ON)");
        return true;
    } else if (strcmp(cmd_str, "debug memory") == 0) {
        ABE_DebugEnableSubsystem(ABE_SUB_MEMORY);
        ABE_DebugLog(ABE_SUB_MEMORY, ABE_DEBUG_INFO, "Live Debug Command Executed: debug memory (ON)");
        return true;
    } else if (strcmp(cmd_str, "debug scheduler") == 0) {
        ABE_DebugEnableSubsystem(ABE_SUB_SCHEDULER);
        ABE_DebugLog(ABE_SUB_SCHEDULER, ABE_DEBUG_INFO, "Live Debug Command Executed: debug scheduler (ON)");
        return true;
    }

    return false;
}
