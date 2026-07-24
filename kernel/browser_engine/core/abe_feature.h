#ifndef ABE_FEATURE_H
#define ABE_FEATURE_H

#include "../../../sdk/include/abe/abe.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ABE_CAP_SCHEME_HTTP  = (1 << 0),
    ABE_CAP_SCHEME_HTTPS = (1 << 1),
    ABE_CAP_SCHEME_FILE  = (1 << 2),
    ABE_CAP_SCHEME_ABOUT = (1 << 3),
    ABE_CAP_SCHEME_DATA  = (1 << 4),
    ABE_CAP_MULTI_WINDOW = (1 << 5),
    ABE_CAP_MULTI_TAB    = (1 << 6),
    ABE_CAP_NAV_HISTORY  = (1 << 7),
    ABE_CAP_RESOURCE_POOL= (1 << 8),
    ABE_CAP_DIAGNOSTICS  = (1 << 9)
} ABE_CapabilityFlags;

#define ABE_MAX_FEATURES 32

typedef struct {
    char name[64];
    uint32_t phase_introduced;
    bool is_enabled;
    void* feature_ctx;
} ABE_Feature;

typedef struct {
    uint32_t capabilities;
    ABE_Feature features[ABE_MAX_FEATURES];
    uint32_t feature_count;
} ABE_FeatureRegistry;

ABE_Error ABE_Feature_Init(void);
ABE_Error ABE_Feature_Shutdown(void);

bool      ABE_HasCapability(ABE_CapabilityFlags cap);
ABE_Error ABE_RegisterFeature(const char* name, uint32_t phase, bool enabled, void* ctx);
bool      ABE_IsFeatureEnabled(const char* name);

#ifdef __cplusplus
}
#endif

#endif // ABE_FEATURE_H
