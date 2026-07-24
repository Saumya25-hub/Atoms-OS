#include "browser_url.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/wm/bwe/include/bwe.h"

extern void bwe_log(const char* level, const char* msg);

static void str_copy_limit(char* dest, const char* src, uint32_t limit) {
    if (!dest || !src || limit == 0) return;
    uint32_t i = 0;
    while (src[i] && i < limit - 1) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

void ATRIX_URL_Init(void) {
    bwe_log("INFO", "ATRIX Real Address Bar & URL Scheme Parser Initialized");
}

bool ATRIX_URL_ValidateScheme(const char* input) {
    if (!input) return false;
    if (strncmp(input, "http://", 7) == 0) return true;
    if (strncmp(input, "https://", 8) == 0) return true;
    if (strncmp(input, "file://", 7) == 0) return true;
    if (strncmp(input, "atrix://", 8) == 0) return true;
    return false;
}

ATRIX_ParsedURL ATRIX_URL_Parse(const char* raw_input) {
    ATRIX_ParsedURL res = {0};
    if (!raw_input || raw_input[0] == '\0') {
        res.is_valid = false;
        return res;
    }

    str_copy_limit(res.raw_url, raw_input, sizeof(res.raw_url));
    res.port = 80;

    if (strncmp(raw_input, "https://", 8) == 0) {
        res.scheme = URL_SCHEME_HTTPS;
        res.port = 443;
        const char* start = raw_input + 8;
        str_copy_limit(res.host, start, sizeof(res.host));
        str_copy_limit(res.path, "/", sizeof(res.path));
        res.is_valid = true;
    } else if (strncmp(raw_input, "http://", 7) == 0) {
        res.scheme = URL_SCHEME_HTTP;
        const char* start = raw_input + 7;
        str_copy_limit(res.host, start, sizeof(res.host));
        str_copy_limit(res.path, "/", sizeof(res.path));
        res.is_valid = true;
    } else if (strncmp(raw_input, "file://", 7) == 0) {
        res.scheme = URL_SCHEME_FILE;
        str_copy_limit(res.host, "localhost", sizeof(res.host));
        str_copy_limit(res.path, raw_input + 7, sizeof(res.path));
        res.is_valid = true;
    } else if (strncmp(raw_input, "atrix://", 8) == 0) {
        res.scheme = URL_SCHEME_ATRIX;
        str_copy_limit(res.host, "internal", sizeof(res.host));
        str_copy_limit(res.path, raw_input + 8, sizeof(res.path));
        res.is_valid = true;
    } else {
        res.scheme = URL_SCHEME_HTTP;
        str_copy_limit(res.host, raw_input, sizeof(res.host));
        str_copy_limit(res.path, "/", sizeof(res.path));
        res.is_valid = true;
    }

    return res;
}
