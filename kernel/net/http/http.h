#ifndef SIGNATURES_HTTP_H
#define SIGNATURES_HTTP_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct {
    int      status_code;
    size_t   content_length;
    bool     is_chunked;
    bool     connection_close;
    bool     header_complete;
    size_t   headers_len;
    size_t   body_len;
    uint8_t  raw_buf[8192];
    size_t   raw_len;
    uint8_t  body_buf[4096];
} HttpResponse;

bool http_get(const char* hostname, const char* path, HttpResponse* resp);

#endif // SIGNATURES_HTTP_H
