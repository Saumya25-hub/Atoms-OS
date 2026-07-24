#ifndef ABE_WEB_XHR_H
#define ABE_WEB_XHR_H

#include "../../../sdk/include/abe/abe.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ABE_MAX_XHR_INSTANCES 32

typedef enum {
    XHR_STATE_UNSENT = 0,
    XHR_STATE_OPENED = 1,
    XHR_STATE_HEADERS_RECEIVED = 2,
    XHR_STATE_LOADING = 3,
    XHR_STATE_DONE = 4
} ABE_XHRReadyState;

typedef struct {
    ABE_XHRHandle handle;
    ABE_XHRReadyState state;
    char method[16];
    char url[ABE_MAX_URL_LEN];
    uint32_t status;
    char response_text[512];
    bool in_use;
} ABE_XHRInstance;

ABE_Error ABE_WebXHR_Init(void);
ABE_Error ABE_WebXHR_Shutdown(void);

ABE_Error ABE_WebXHR_Create(ABE_XHRHandle* out_xhr);
ABE_Error ABE_WebXHR_Open(ABE_XHRHandle xhr, const char* method, const char* url);
ABE_Error ABE_WebXHR_Send(ABE_XHRHandle xhr, const char* body);
ABE_Error ABE_WebXHR_Abort(ABE_XHRHandle xhr);

#ifdef __cplusplus
}
#endif

#endif // ABE_WEB_XHR_H
