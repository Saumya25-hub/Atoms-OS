#include "../Include/sds.h"
#include <stddef.h>

#define SDS_MAX_PROVIDERS 4

static SDS_OutputProvider g_providers[SDS_MAX_PROVIDERS];
static uint32_t g_provider_count = 0;
static SDS_OutputLevel g_output_level = SDS_LEVEL_DEVELOPER; // Default

void SDS_Init(void) {
    g_provider_count = 0;
    g_output_level = SDS_LEVEL_DEVELOPER;
}

void SDS_SetOutputLevel(SDS_OutputLevel level) {
    g_output_level = level;
}

SDS_OutputLevel _SDS_Core_GetOutputLevel(void) {
    return g_output_level;
}

bool SDS_RegisterOutputProvider(SDS_OutputProvider provider) {
    if (g_provider_count >= SDS_MAX_PROVIDERS) return false;
    
    g_providers[g_provider_count++] = provider;
    return true;
}

void SDS_ClearOutputProviders(void) {
    g_provider_count = 0;
}

// Internal accessor for the console module to dispatch the formatted string
void _SDS_Core_DispatchOutput(const char* formatted_string, const SDS_DiagnosticObject* obj) {
    if (g_output_level == SDS_LEVEL_SILENT) {
        return; // Complete silence
    }
    
    for (uint32_t i = 0; i < g_provider_count; i++) {
        if (g_providers[i].write_string) {
            g_providers[i].write_string(formatted_string);
        }
        if (g_providers[i].write_raw) {
            g_providers[i].write_raw(obj);
        }
    }
}

// Fallback weak implementations for OS metrics if not provided by kernel
__attribute__((weak)) uint32_t _SDS_Platform_GetTimestamp(void) { return 0; }
__attribute__((weak)) uint32_t _SDS_Platform_GetThreadID(void) { return 0; }
__attribute__((weak)) uint32_t _SDS_Platform_GetCPUCore(void) { return 0; }

__attribute__((weak)) void _SDS_Halt_System(void) {
    while (1) {
        // Halt
    }
}
