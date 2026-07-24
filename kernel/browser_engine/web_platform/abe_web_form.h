#ifndef ABE_WEB_FORM_H
#define ABE_WEB_FORM_H

#include "../../../sdk/include/abe/abe.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ABE_MAX_FORM_ENTRIES 16

typedef struct {
    char key[64];
    char val[128];
    bool in_use;
} ABE_FormDataEntry;

typedef struct {
    ABE_FormDataEntry entries[ABE_MAX_FORM_ENTRIES];
    uint32_t count;
} ABE_FormDataStruct;

ABE_Error ABE_WebForm_Init(void);
ABE_Error ABE_WebForm_Shutdown(void);

ABE_Error ABE_WebForm_AppendData(ABE_FormDataStruct* form, const char* key, const char* val);

#ifdef __cplusplus
}
#endif

#endif // ABE_WEB_FORM_H
