#include "../include/terminal_api.h"
#include "kernel/drivers/display/display.h"

// ============================================================
// Terminal.BOSX — Environment Runtime Engine
// Per-session environment variable store.
// In production, delegates to ADVAPI32.sll for persistence.
// ============================================================

static TERMINAL_ENV_VAR s_env_vars[TERMINAL_MAX_ENV_VARS];
static uint32_t         s_env_count = 0;

static void safe_strcpy(char* dst, const char* src, uint32_t max) {
    uint32_t i = 0;
    while (src[i] && i < max - 1) { dst[i] = src[i]; i++; }
    dst[i] = '\0';
}

void terminal_environment_init(void) {
    s_env_count = 0;
    // Seed default variables
    safe_strcpy(s_env_vars[0].key,   "OS",    128); safe_strcpy(s_env_vars[0].value, "ATOMS OS", 512); s_env_count++;
    safe_strcpy(s_env_vars[1].key,   "PATH",  128); safe_strcpy(s_env_vars[1].value, "/system/bin;/apps", 512); s_env_count++;
    safe_strcpy(s_env_vars[2].key,   "SHELL", 128); safe_strcpy(s_env_vars[2].value, "Terminal.BOSX", 512); s_env_count++;
    display_print("[TERMINAL_ENV] Environment Runtime Initialized (3 default vars set).\n");
}

bool terminal_environment_set(const char* key, const char* value) {
    if (!key || !value) return false;
    // Search existing
    for (uint32_t i = 0; i < s_env_count; i++) {
        const char* k = s_env_vars[i].key;
        const char* q = key;
        bool eq = true;
        while (*q && *k) { if (*q != *k) { eq = false; break; } q++; k++; }
        if (eq && *q == '\0' && *k == '\0') {
            safe_strcpy(s_env_vars[i].value, value, 512);
            return true;
        }
    }
    if (s_env_count >= TERMINAL_MAX_ENV_VARS) return false;
    safe_strcpy(s_env_vars[s_env_count].key,   key,   128);
    safe_strcpy(s_env_vars[s_env_count].value, value, 512);
    s_env_count++;
    return true;
}

const char* terminal_environment_get(const char* key) {
    if (!key) return 0;
    for (uint32_t i = 0; i < s_env_count; i++) {
        const char* k = s_env_vars[i].key;
        const char* q = key;
        bool eq = true;
        while (*q && *k) { if (*q != *k) { eq = false; break; } q++; k++; }
        if (eq && *q == '\0' && *k == '\0') return s_env_vars[i].value;
    }
    return 0;
}

uint32_t terminal_environment_count(void) { return s_env_count; }
