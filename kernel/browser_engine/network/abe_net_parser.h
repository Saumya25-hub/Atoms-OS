#ifndef ABE_NET_PARSER_H
#define ABE_NET_PARSER_H

#include "../../../sdk/include/abe/abe.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t status_code;
    char status_text[64];
    char http_version[16];
    ABE_HTTPHeader headers[ABE_MAX_HEADERS];
    uint32_t header_count;
    size_t content_length;
    bool is_chunked;
    bool is_gzipped;
    bool is_keep_alive;
    char location[ABE_MAX_URL_LEN];
    const uint8_t* raw_body_start;
    size_t raw_body_len;
} ABE_HTTPResponseHeaderInfo;

ABE_Error ABE_NetParser_Init(void);
ABE_Error ABE_NetParser_Shutdown(void);

ABE_Error ABE_NetParser_ParseHeader(const uint8_t* raw_buf, size_t raw_len, ABE_HTTPResponseHeaderInfo* out_info, size_t* out_header_bytes);
ABE_Error ABE_NetParser_DecodeChunked(const uint8_t* chunked_buf, size_t chunked_len, uint8_t** out_decoded, size_t* out_decoded_len);
const char* ABE_NetParser_GetHeaderValue(const ABE_HTTPResponseHeaderInfo* info, const char* name);

#ifdef __cplusplus
}
#endif

#endif // ABE_NET_PARSER_H
