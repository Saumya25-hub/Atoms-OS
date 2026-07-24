#ifndef ABE_CORE_H
#define ABE_CORE_H

#include "../../../sdk/include/abe/abe.h"
#include "abe_config.h"
#include "abe_feature.h"
#include "../diagnostics/abe_diagnostics.h"
#include "../resource/abe_resource.h"
#include "../process/abe_process.h"
#include "../process/abe_session.h"
#include "../window/abe_window.h"
#include "../tab/abe_tab.h"
#include "../url/abe_url.h"
#include "../navigation/abe_navigation.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    bool is_initialized;
    uint64_t init_timestamp;
    uint32_t engine_id;
    ABE_Config config;
} ABE_Runtime;

ABE_Error ABE_Core_Initialize(const ABE_Config* config);
ABE_Error ABE_Core_Shutdown(void);
bool      ABE_Core_IsInitialized(void);
ABE_Runtime* ABE_Core_GetRuntime(void);

#ifdef __cplusplus
}
#endif

#endif // ABE_CORE_H
