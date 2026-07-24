#include "abe_net_parser.h"
#include "../diagnostics/abe_diagnostics.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/lib/include/string.h"

static bool g_parser_initialized = false;

ABE_Error ABE_NetParser_Init(void) {
    g_parser_initialized = true;
    ABE_Log(ABE_LOG_INFO, "PARSER", "ABE HTTP Response Parser & Chunked Decoder V1.0 initialized");
    return ABE_SUCCESS;
}

ABE_Error ABE_NetParser_Shutdown(void) {
    g_parser_initialized = false;
    return ABE_SUCCESS;
}

static char ToLowerChar(char c) {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

static int StrCaseCmp(const char* s1, const char* s2) {
    while (*s1 && *s2) {
        if (ToLowerChar(*s1) != ToLowerChar(*s2)) return 1;
        s1++; s2++;
    }
    return (*s1 == '\0' && *s2 == '\0') ? 0 : 1;
}

const char* ABE_NetParser_GetHeaderValue(const ABE_HTTPResponseHeaderInfo* info, const char* name) {
    if (!info || !name) return NULL;
    for (uint32_t i = 0; i < info->header_count; i++) {
        if (StrCaseCmp(info->headers[i].name, name) == 0) {
            return info->headers[i].value;
        }
    }
    return NULL;
}

static uint32_t ParseHex(const char* str, size_t len) {
    uint32_t val = 0;
    for (size_t i = 0; i < len; i++) {
        char c = str[i];
        uint32_t digit = 0;
        if (c >= '0' && c <= '9') digit = c - '0';
        else if (c >= 'a' && c <= 'f') digit = 10 + (c - 'a');
        else if (c >= 'A' && c <= 'F') digit = 10 + (c - 'A');
        else break;
        val = (val << 4) | digit;
    }
    return val;
}

ABE_Error ABE_NetParser_ParseHeader(const uint8_t* raw_buf, size_t raw_len, ABE_HTTPResponseHeaderInfo* out_info, size_t* out_header_bytes) {
    if (!g_parser_initialized || !raw_buf || raw_len == 0 || !out_info || !out_header_bytes) return ABE_ERR_INVALID_PARAM;
    memset(out_info, 0, sizeof(ABE_HTTPResponseHeaderInfo));

    // Find end of headers "\r\n\r\n"
    const char* str = (const char*)raw_buf;
    const char* header_end = strstr(str, "\r\n\r\n");
    if (!header_end) return ABE_ERR_NET_PARSE_FAILED;

    size_t header_bytes = (size_t)(header_end - str) + 4;
    *out_header_bytes = header_bytes;

    // 1. Status Line: HTTP/1.1 200 OK
    const char* line_end = strstr(str, "\r\n");
    if (!line_end) return ABE_ERR_NET_PARSE_FAILED;

    const char* space1 = strstr(str, " ");
    if (!space1 || space1 > line_end) return ABE_ERR_NET_PARSE_FAILED;

    size_t ver_len = (size_t)(space1 - str);
    if (ver_len >= sizeof(out_info->http_version)) ver_len = sizeof(out_info->http_version) - 1;
    strncpy(out_info->http_version, str, ver_len);
    out_info->http_version[ver_len] = '\0';

    const char* code_start = space1 + 1;
    out_info->status_code = 0;
    for (int i = 0; i < 3 && code_start[i] >= '0' && code_start[i] <= '9'; i++) {
        out_info->status_code = out_info->status_code * 10 + (code_start[i] - '0');
    }

    const char* space2 = strstr(code_start, " ");
    if (space2 && space2 < line_end) {
        size_t text_len = (size_t)(line_end - (space2 + 1));
        if (text_len >= sizeof(out_info->status_text)) text_len = sizeof(out_info->status_text) - 1;
        strncpy(out_info->status_text, space2 + 1, text_len);
        out_info->status_text[text_len] = '\0';
    }

    // 2. Parse Headers
    const char* ptr = line_end + 2;
    while (ptr < header_end && out_info->header_count < ABE_MAX_HEADERS) {
        const char* next_line = strstr(ptr, "\r\n");
        if (!next_line || next_line == ptr) break;

        const char* colon = strstr(ptr, ":");
        if (colon && colon < next_line) {
            size_t name_len = (size_t)(colon - ptr);
            if (name_len >= sizeof(out_info->headers[0].name)) name_len = sizeof(out_info->headers[0].name) - 1;

            ABE_HTTPHeader* h = &out_info->headers[out_info->header_count++];
            strncpy(h->name, ptr, name_len);
            h->name[name_len] = '\0';

            const char* val_start = colon + 1;
            while (*val_start == ' ' || *val_start == '\t') val_start++;
            size_t val_len = (size_t)(next_line - val_start);
            if (val_len >= sizeof(h->value)) val_len = sizeof(h->value) - 1;
            strncpy(h->value, val_start, val_len);
            h->value[val_len] = '\0';
        }
        ptr = next_line + 2;
    }

    // Extract common fields
    const char* cl_val = ABE_NetParser_GetHeaderValue(out_info, "Content-Length");
    if (cl_val) {
        uint32_t val = 0;
        for (size_t i = 0; cl_val[i] >= '0' && cl_val[i] <= '9'; i++) {
            val = val * 10 + (cl_val[i] - '0');
        }
        out_info->content_length = val;
    }

    const char* te_val = ABE_NetParser_GetHeaderValue(out_info, "Transfer-Encoding");
    if (te_val && strstr(te_val, "chunked") != NULL) {
        out_info->is_chunked = true;
    }

    const char* ce_val = ABE_NetParser_GetHeaderValue(out_info, "Content-Encoding");
    if (ce_val && (strstr(ce_val, "gzip") != NULL || strstr(ce_val, "deflate") != NULL)) {
        out_info->is_gzipped = true;
    }

    const char* conn_val = ABE_NetParser_GetHeaderValue(out_info, "Connection");
    if (conn_val && StrCaseCmp(conn_val, "keep-alive") == 0) {
        out_info->is_keep_alive = true;
    }

    const char* loc_val = ABE_NetParser_GetHeaderValue(out_info, "Location");
    if (loc_val) {
        strncpy(out_info->location, loc_val, sizeof(out_info->location) - 1);
    }

    out_info->raw_body_start = raw_buf + header_bytes;
    out_info->raw_body_len = raw_len - header_bytes;
    return ABE_SUCCESS;
}

ABE_Error ABE_NetParser_DecodeChunked(const uint8_t* chunked_buf, size_t chunked_len, uint8_t** out_decoded, size_t* out_decoded_len) {
    if (!chunked_buf || chunked_len == 0 || !out_decoded || !out_decoded_len) return ABE_ERR_INVALID_PARAM;

    uint8_t* decoded = (uint8_t*)kmalloc(chunked_len + 1);
    if (!decoded) return ABE_ERR_OUT_OF_MEMORY;

    size_t decoded_offset = 0;
    const char* ptr = (const char*)chunked_buf;
    const char* end = ptr + chunked_len;

    while (ptr < end) {
        const char* crlf = strstr(ptr, "\r\n");
        if (!crlf) break;

        size_t size_str_len = (size_t)(crlf - ptr);
        uint32_t chunk_size = ParseHex(ptr, size_str_len);
        if (chunk_size == 0) break; // Final chunk '0\r\n\r\n'

        ptr = crlf + 2;
        if (ptr + chunk_size > end) break;

        memcpy(decoded + decoded_offset, ptr, chunk_size);
        decoded_offset += chunk_size;

        ptr += chunk_size;
        if (ptr + 2 <= end && ptr[0] == '\r' && ptr[1] == '\n') {
            ptr += 2;
        }
    }

    decoded[decoded_offset] = '\0';
    ABE_Diag_RecordMemoryAlloc(chunked_len + 1);
    *out_decoded = decoded;
    *out_decoded_len = decoded_offset;
    return ABE_SUCCESS;
}
