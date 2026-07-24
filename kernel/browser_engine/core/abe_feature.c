#include "abe_feature.h"
#include "../diagnostics/abe_diagnostics.h"
#include "kernel/core/lib/include/string.h"

static ABE_FeatureRegistry g_feature_reg;

ABE_Error ABE_Feature_Init(void) {
    memset(&g_feature_reg, 0, sizeof(ABE_FeatureRegistry));
    g_feature_reg.capabilities = ABE_CAP_SCHEME_HTTP | ABE_CAP_SCHEME_HTTPS |
                                 ABE_CAP_SCHEME_FILE | ABE_CAP_SCHEME_ABOUT |
                                 ABE_CAP_SCHEME_DATA | ABE_CAP_MULTI_WINDOW |
                                 ABE_CAP_MULTI_TAB   | ABE_CAP_NAV_HISTORY  |
                                 ABE_CAP_RESOURCE_POOL | ABE_CAP_DIAGNOSTICS;

    ABE_RegisterFeature("EngineCore", 1, true, NULL);
    ABE_RegisterFeature("WindowManager", 1, true, NULL);
    ABE_RegisterFeature("TabEngine", 1, true, NULL);
    ABE_RegisterFeature("URLEngine", 1, true, NULL);
    ABE_RegisterFeature("NavigationEngine", 1, true, NULL);
    ABE_RegisterFeature("ResourceManager", 1, true, NULL);
    ABE_RegisterFeature("Diagnostics", 1, true, NULL);

    ABE_Log(ABE_LOG_INFO, "FEATURE", "ABE Capability Manager & Feature Registry initialized");
    return ABE_SUCCESS;
}

ABE_Error ABE_Feature_Shutdown(void) {
    memset(&g_feature_reg, 0, sizeof(ABE_FeatureRegistry));
    return ABE_SUCCESS;
}

bool ABE_HasCapability(ABE_CapabilityFlags cap) {
    return (g_feature_reg.capabilities & cap) == cap;
}

ABE_Error ABE_RegisterFeature(const char* name, uint32_t phase, bool enabled, void* ctx) {
    if (!name || g_feature_reg.feature_count >= ABE_MAX_FEATURES) return ABE_ERR_INVALID_PARAM;
    for (uint32_t i = 0; i < g_feature_reg.feature_count; i++) {
        if (strcmp(g_feature_reg.features[i].name, name) == 0) {
            g_feature_reg.features[i].is_enabled = enabled;
            g_feature_reg.features[i].feature_ctx = ctx;
            return ABE_SUCCESS;
        }
    }

    ABE_Feature* f = &g_feature_reg.features[g_feature_reg.feature_count++];
    strncpy(f->name, name, sizeof(f->name) - 1);
    f->phase_introduced = phase;
    f->is_enabled = enabled;
    f->feature_ctx = ctx;
    return ABE_SUCCESS;
}

bool ABE_IsFeatureEnabled(const char* name) {
    if (!name) return false;
    for (uint32_t i = 0; i < g_feature_reg.feature_count; i++) {
        if (strcmp(g_feature_reg.features[i].name, name) == 0) {
            return g_feature_reg.features[i].is_enabled;
        }
    }
    return false;
}
