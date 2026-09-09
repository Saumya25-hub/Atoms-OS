/*
 * ATOMS Platform Adaptation Layer (APAL)
 * Master Lifecycle Implementation
 */

#include "include/apal.h"

#define APAL_VERSION_STRING "APAL 1.0.0 (Native ATOMS OS x86_64)"

static bool g_apal_initialized = false;

apal_status_t apal_init(void) {
    if (g_apal_initialized) return APAL_OK;
    g_apal_initialized = true;
    return APAL_OK;
}

apal_status_t apal_shutdown(void) {
    if (!g_apal_initialized) return APAL_OK;
    g_apal_initialized = false;
    return APAL_OK;
}

const char *apal_get_version_string(void) {
    return APAL_VERSION_STRING;
}
