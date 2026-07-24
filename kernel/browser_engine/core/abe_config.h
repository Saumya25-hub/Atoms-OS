#ifndef ABE_CONFIG_H
#define ABE_CONFIG_H

#include "../../../sdk/include/abe/abe.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    ABE_Config active_config;
    bool is_locked;
    uint32_t config_revision;
} ABE_ConfigManager;

ABE_Error ABE_Config_Init(const ABE_Config* config);
ABE_Error ABE_Config_Shutdown(void);
const ABE_Config* ABE_Config_Get(void);
ABE_Error ABE_Config_Update(const ABE_Config* new_config);
void ABE_Config_SetDefaults(ABE_Config* config);

#ifdef __cplusplus
}
#endif

#endif // ABE_CONFIG_H
