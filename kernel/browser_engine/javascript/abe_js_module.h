#ifndef ABE_JS_MODULE_H
#define ABE_JS_MODULE_H

#include "../../../sdk/include/abe/abe.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ABE_JS_MAX_MODULES 32

typedef struct {
    char specifier[128];
    uint32_t script_handle;
    bool is_evaluated;
    bool in_use;
} ABE_JSModule;

ABE_Error ABE_JSModule_Init(void);
ABE_Error ABE_JSModule_Shutdown(void);

ABE_Error ABE_JSModule_Load(const char* specifier, const char* source, size_t len);
ABE_Error ABE_JSModule_Get(const char* specifier, ABE_JSModule** out_mod);

#ifdef __cplusplus
}
#endif

#endif // ABE_JS_MODULE_H
