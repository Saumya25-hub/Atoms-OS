#include "abe_js_module.h"
#include "../diagnostics/abe_diagnostics.h"
#include "kernel/core/lib/include/string.h"

static ABE_JSModule g_modules[ABE_JS_MAX_MODULES];
static bool g_js_module_initialized = false;

ABE_Error ABE_JSModule_Init(void) {
    memset(g_modules, 0, sizeof(g_modules));
    g_js_module_initialized = true;
    ABE_Log(ABE_LOG_INFO, "JSMODULE", "ABE ES Module Loader Subsystem initialized");
    return ABE_SUCCESS;
}

ABE_Error ABE_JSModule_Shutdown(void) {
    g_js_module_initialized = false;
    return ABE_SUCCESS;
}

ABE_Error ABE_JSModule_Load(const char* specifier, const char* source, size_t len) {
    if (!g_js_module_initialized || !specifier || !source) return ABE_ERR_INVALID_PARAM;

    for (uint32_t i = 0; i < ABE_JS_MAX_MODULES; i++) {
        if (!g_modules[i].in_use) {
            ABE_JSModule* m = &g_modules[i];
            strncpy(m->specifier, specifier, sizeof(m->specifier) - 1);
            m->is_evaluated = true;
            m->in_use = true;
            ABE_Log(ABE_LOG_INFO, "JSMODULE", "Loaded ES Module specifier cleanly");
            return ABE_SUCCESS;
        }
    }
    return ABE_ERR_RESOURCE_EXHAUSTED;
}

ABE_Error ABE_JSModule_Get(const char* specifier, ABE_JSModule** out_mod) {
    if (!g_js_module_initialized || !specifier || !out_mod) return ABE_ERR_INVALID_PARAM;

    for (uint32_t i = 0; i < ABE_JS_MAX_MODULES; i++) {
        if (g_modules[i].in_use && strcmp(g_modules[i].specifier, specifier) == 0) {
            *out_mod = &g_modules[i];
            return ABE_SUCCESS;
        }
    }
    return ABE_ERR_INVALID_PARAM;
}
