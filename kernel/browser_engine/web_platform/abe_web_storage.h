#ifndef ABE_WEB_STORAGE_H
#define ABE_WEB_STORAGE_H

#include "../../../sdk/include/abe/abe.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ABE_MAX_STORAGE_ITEMS 64

typedef struct {
    char key[64];
    char val[256];
    bool in_use;
} ABE_StorageItem;

typedef struct {
    ABE_StorageHandle handle;
    bool is_session;
    ABE_StorageItem items[ABE_MAX_STORAGE_ITEMS];
    uint32_t count;
    bool in_use;
} ABE_WebStorage;

ABE_Error ABE_WebStorage_Init(void);
ABE_Error ABE_WebStorage_Shutdown(void);

ABE_Error ABE_WebStorage_GetLocal(ABE_StorageHandle* out_storage);
ABE_Error ABE_WebStorage_GetSession(ABE_StorageHandle* out_storage);

ABE_Error ABE_WebStorage_Set(ABE_StorageHandle storage, const char* key, const char* val);
ABE_Error ABE_WebStorage_Get(ABE_StorageHandle storage, const char* key, char* out_buf, size_t max_len);
ABE_Error ABE_WebStorage_Remove(ABE_StorageHandle storage, const char* key);
ABE_Error ABE_WebStorage_Clear(ABE_StorageHandle storage);

#ifdef __cplusplus
}
#endif

#endif // ABE_WEB_STORAGE_H
