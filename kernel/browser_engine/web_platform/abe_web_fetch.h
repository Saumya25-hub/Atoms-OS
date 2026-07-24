#ifndef ABE_WEB_FETCH_H
#define ABE_WEB_FETCH_H

#include "../../../sdk/include/abe/abe.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ABE_MAX_FETCH_REQUESTS 32

typedef struct {
    ABE_FetchRequestHandle handle;
    ABE_RequestHandle net_req_handle;
    char url[ABE_MAX_URL_LEN];
    uint32_t status_code;
    char status_text[64];
    uint8_t* response_body;
    size_t body_len;
    bool is_aborted;
    bool in_use;
} ABE_FetchRequest;

ABE_Error ABE_WebFetch_Init(void);
ABE_Error ABE_WebFetch_Shutdown(void);

ABE_Error ABE_WebFetch_Perform(ABE_JSContextHandle ctx, const char* url, const ABE_HTTPRequest* init, ABE_FetchRequestHandle* out_req);
ABE_Error ABE_WebFetch_Abort(ABE_FetchRequestHandle handle);
ABE_Error ABE_WebFetch_GetResponseText(ABE_FetchRequestHandle handle, char* out_buf, size_t max_len);

#ifdef __cplusplus
}
#endif

#endif // ABE_WEB_FETCH_H
