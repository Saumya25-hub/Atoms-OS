/*
 * BOSPECTRA V3 — Dynamic Renderer Registry Implementation
 * kernel/media/bospectra/registry/renderer_registry.c
 */

#include "renderer_registry.h"
#include "../debug/bospectra_debug.h"
#include "kernel/core/lib/include/string.h"

static BOSPECTRA_RendererRecord g_renderer_records[BOSPECTRA_MAX_RENDERER_DRIVERS];
static uint32_t g_renderer_record_count = 0;
static bool g_renderer_reg_initialized = false;

void bospectra_v3_renderer_registry_init(void) {
    memset(g_renderer_records, 0, sizeof(g_renderer_records));
    g_renderer_record_count = 0;
    g_renderer_reg_initialized = true;
    bospectra_log("RENDERER_REGISTRY", "BOSPECTRA V3 Renderer Registry Initialized.");
}

void bospectra_v3_renderer_registry_shutdown(void) {
    memset(g_renderer_records, 0, sizeof(g_renderer_records));
    g_renderer_record_count = 0;
    g_renderer_reg_initialized = false;
}

bospectra_error_t bospectra_v3_renderer_register(const BOSPECTRA_RendererRecord* record) {
    if (!g_renderer_reg_initialized) return BOSPECTRA_ERR_NOT_INITIALIZED;
    if (!record || !record->backend_name || !record->backend_vtable) {
        return BOSPECTRA_ERR_INVALID_ARGUMENT;
    }

    /* Reject duplicate renderer types */
    for (uint32_t i = 0; i < g_renderer_record_count; i++) {
        if (g_renderer_records[i].backend_type == record->backend_type) {
            return BOSPECTRA_ERR_STREAM_EXISTS;
        }
    }

    if (g_renderer_record_count >= BOSPECTRA_MAX_RENDERER_DRIVERS) {
        return BOSPECTRA_ERR_BUFFER_OVERFLOW;
    }

    g_renderer_records[g_renderer_record_count++] = *record;
    bospectra_log("RENDERER_REGISTRY", record->backend_name);
    return BOSPECTRA_SUCCESS;
}

const BOSPECTRA_RendererRecord* bospectra_v3_renderer_resolve_best(bospectra_render_backend_t preferred_type) {
    if (!g_renderer_reg_initialized || g_renderer_record_count == 0) return NULL;

    /* Search for exact match */
    for (uint32_t i = 0; i < g_renderer_record_count; i++) {
        if (g_renderer_records[i].backend_type == preferred_type) {
            return &g_renderer_records[i];
        }
    }

    /* Return highest priority renderer */
    const BOSPECTRA_RendererRecord* best = &g_renderer_records[0];
    for (uint32_t i = 1; i < g_renderer_record_count; i++) {
        if (g_renderer_records[i].priority > best->priority) {
            best = &g_renderer_records[i];
        }
    }
    return best;
}

uint32_t bospectra_v3_renderer_get_registered(const BOSPECTRA_RendererRecord** out_records, uint32_t max_count) {
    if (!g_renderer_reg_initialized || !out_records) return 0;
    uint32_t count = (g_renderer_record_count < max_count) ? g_renderer_record_count : max_count;
    for (uint32_t i = 0; i < count; i++) {
        out_records[i] = &g_renderer_records[i];
    }
    return count;
}
