#ifndef ABE_NET_HTTP_H
#define ABE_NET_HTTP_H

#include "../../../sdk/include/abe/abe.h"

#ifdef __cplusplus
extern "C" {
#endif

ABE_Error ABE_NetHTTP_Init(void);
ABE_Error ABE_NetHTTP_Shutdown(void);

ABE_Error ABE_NetHTTP_CreateRequest(ABE_HTTPMethod method, const char* url_str, ABE_HTTPRequest* out_req);
ABE_Error ABE_NetHTTP_AddHeader(ABE_HTTPRequest* req, const char* name, const char* value);
ABE_Error ABE_NetHTTP_SerializeRequest(const ABE_HTTPRequest* req, uint8_t** out_buf, size_t* out_len);
void      ABE_NetHTTP_FreeRequest(ABE_HTTPRequest* req);

#ifdef __cplusplus
}
#endif

#endif // ABE_NET_HTTP_H
