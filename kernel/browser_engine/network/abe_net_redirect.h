#ifndef ABE_NET_REDIRECT_H
#define ABE_NET_REDIRECT_H

#include "../../../sdk/include/abe/abe.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ABE_MAX_REDIRECT_CHAIN 10

typedef struct {
    char source_url[ABE_MAX_URL_LEN];
    char target_url[ABE_MAX_URL_LEN];
    uint32_t status_code;
    uint64_t timestamp;
} ABE_RedirectEntry;

typedef struct {
    ABE_RedirectEntry chain[ABE_MAX_REDIRECT_CHAIN];
    uint32_t redirect_count;
} ABE_RedirectHistory;

ABE_Error ABE_NetRedirect_Init(void);
ABE_Error ABE_NetRedirect_Shutdown(void);

bool      ABE_NetRedirect_IsRedirectCode(uint32_t status_code);
ABE_Error ABE_NetRedirect_ProcessRedirect(ABE_RedirectHistory* history, const char* current_url, uint32_t status_code, const char* location_header, ABE_HTTPMethod orig_method, char* out_new_url, ABE_HTTPMethod* out_new_method);

#ifdef __cplusplus
}
#endif

#endif // ABE_NET_REDIRECT_H
