#ifndef SIGNATURES_HTTP_H
#define SIGNATURES_HTTP_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef enum {
    HTTP_BODY_MODE_NONE = 0,
    HTTP_BODY_MODE_CONTENT_LENGTH,
    HTTP_BODY_MODE_CHUNKED,
    HTTP_BODY_MODE_UNTIL_CLOSE
} HttpBodyMode;

typedef struct {
    int          status_code;
    size_t       content_length;
    bool         is_chunked;
    bool         connection_close;
    bool         header_complete;
    HttpBodyMode body_mode;
    size_t       headers_len;
    size_t       raw_len;
    size_t       body_len;
    uint8_t      raw_buf[8192];
    uint8_t      body_buf[4096];
} HttpResponse;

bool http_decode_chunked(const uint8_t* raw_body, size_t raw_body_len, uint8_t* out_body, size_t max_out, size_t* decoded_len);
bool http_get(const char* hostname, const char* path, HttpResponse* resp);
bool https_get(const char* hostname, const char* path, HttpResponse* resp);

#endif // SIGNATURES_HTTP_H
