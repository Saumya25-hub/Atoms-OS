// Engine 1: AGP Graphics Runtime Manager
#include "../include/agp_api.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/drivers/display/display.h"

static bool s_agp_initialized = false;
static AGPContext* s_current_context = NULL;
static uint32_t s_frame_count = 0;

int32_t AGP_Init(void) {
    if (s_agp_initialized) return 0;
    
    display_print("[AGP] Initializing ATOMS Graphics Platform (AGP V1.0)...\n");
    s_agp_initialized = true;
    s_current_context = NULL;
    s_frame_count = 0;

    display_print("[AGP] Runtime Manager online. Ring 3 OpenGL platform ready.\n");
    return 0;
}

void AGP_Shutdown(void) {
    if (!s_agp_initialized) return;
    display_print("[AGP] Shutting down ATOMS Graphics Platform...\n");
    s_agp_initialized = false;
}

AGPContext* AGP_GetCurrentContext(void) {
    return s_current_context;
}

void AGP_SetCurrentContextInternal(AGPContext* ctx) {
    s_current_context = ctx;
}
