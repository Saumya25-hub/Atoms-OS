#ifndef ABE_WEB_URL_H
#define ABE_WEB_URL_H

#include "../../../sdk/include/abe/abe.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char scheme[16];
    char host[128];
    uint16_t port;
    char path[256];
    char query[256];
    char fragment[64];
    char origin[256];
} ABE_URLComponents;

ABE_Error ABE_WebURL_Init(void);
ABE_Error ABE_WebURL_Shutdown(void);

ABE_Error ABE_WebURL_Parse(const char* url_str, ABE_URLComponents* out_comp);
ABE_Error ABE_WebURL_EncodeComponent(const char* str, char* out_buf, size_t max_len);
ABE_Error ABE_WebURL_DecodeComponent(const char* str, char* out_buf, size_t max_len);

#ifdef __cplusplus
}
#endif

#endif // ABE_WEB_URL_H
