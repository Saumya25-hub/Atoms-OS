#include "framework/include/bos_ui_reflection.h"
#include "kernel/core/lib/include/string.h"

#define BOS_MAX_REGISTERED_CONTROLS 64U
static const BOS_ControlMetadata* g_control_metadata_registry[BOS_MAX_REGISTERED_CONTROLS];
static uint32_t g_control_metadata_count = 0;

void BOS_Reflection_RegisterControl(const BOS_ControlMetadata* meta) {
    if (!meta || !meta->class_name || g_control_metadata_count >= BOS_MAX_REGISTERED_CONTROLS) return;
    g_control_metadata_registry[g_control_metadata_count++] = meta;
}

const BOS_ControlMetadata* BOS_Reflection_FindControl(const char* class_name) {
    if (!class_name) return NULL;

    for (uint32_t i = 0; i < g_control_metadata_count; i++) {
        if (g_control_metadata_registry[i] && 
            strcmp(g_control_metadata_registry[i]->class_name, class_name) == 0) {
            return g_control_metadata_registry[i];
        }
    }
    return NULL;
}
