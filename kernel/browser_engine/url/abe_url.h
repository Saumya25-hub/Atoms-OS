#ifndef ABE_URL_H
#define ABE_URL_H

#include "../../../sdk/include/abe/abe.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ABE_SCHEME_UNKNOWN = 0,
    ABE_SCHEME_HTTP = 1,
    ABE_SCHEME_HTTPS = 2,
    ABE_SCHEME_FILE = 3,
    ABE_SCHEME_ABOUT = 4,
    ABE_SCHEME_DATA = 5
} ABE_URLScheme;

typedef struct {
    char key[128];
    char value[256];
} ABE_URLQueryParam;

#define ABE_MAX_QUERY_PARAMS 16

typedef struct {
    ABE_URLScheme scheme;
    char scheme_str[16];
    char host[256];
    uint16_t port;
    char path[1024];
    char query[512];
    char fragment[256];
    char raw_url[ABE_MAX_URL_LEN];
    ABE_URLQueryParam query_params[ABE_MAX_QUERY_PARAMS];
    uint32_t query_param_count;
    bool is_valid;
    bool is_default_port;
} ABE_URL;

ABE_Error ABE_URL_Init(void);
ABE_Error ABE_ParseURL(const char* raw_url, ABE_URL* out_url);
bool      ABE_ValidateURL(const ABE_URL* url);
ABE_Error ABE_NormalizePath(const char* raw_path, char* out_path, size_t max_len);
ABE_Error ABE_ResolveRelativeURL(const char* base_url, const char* relative_url, char* out_resolved, size_t max_len);
ABE_Error ABE_BuildURLString(const ABE_URL* url, char* out_buf, size_t max_len);

#ifdef __cplusplus
}
#endif

#endif // ABE_URL_H
