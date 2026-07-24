#include "abe_web_form.h"
#include "../diagnostics/abe_diagnostics.h"
#include "kernel/core/lib/include/string.h"

static bool g_web_form_initialized = false;

ABE_Error ABE_WebForm_Init(void) {
    g_web_form_initialized = true;
    ABE_Log(ABE_LOG_INFO, "WEBFORM", "ABE FormData Engine Subsystem initialized");
    return ABE_SUCCESS;
}

ABE_Error ABE_WebForm_Shutdown(void) {
    g_web_form_initialized = false;
    return ABE_SUCCESS;
}

ABE_Error ABE_WebForm_AppendData(ABE_FormDataStruct* form, const char* key, const char* val) {
    if (!g_web_form_initialized || !form || !key || !val) return ABE_ERR_INVALID_PARAM;

    for (uint32_t i = 0; i < ABE_MAX_FORM_ENTRIES; i++) {
        if (!form->entries[i].in_use) {
            strncpy(form->entries[i].key, key, sizeof(form->entries[i].key) - 1);
            strncpy(form->entries[i].val, val, sizeof(form->entries[i].val) - 1);
            form->entries[i].in_use = true;
            form->count++;
            return ABE_SUCCESS;
        }
    }
    return ABE_ERR_RESOURCE_EXHAUSTED;
}
