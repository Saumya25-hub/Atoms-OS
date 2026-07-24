#include "abe_url.h"
#include "../diagnostics/abe_diagnostics.h"
#include "kernel/core/lib/include/string.h"

extern void display_print(const char* s);
extern void display_print_dec(uint32_t val);

ABE_Error ABE_URL_Init(void) {
    ABE_Log(ABE_LOG_INFO, "URL", "ABE URL Engine V1.0 initialized");
    return ABE_SUCCESS;
}

static bool IsAlpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static bool IsDigit(char c) {
    return c >= '0' && c <= '9';
}

static char ToLower(char c) {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

static char* FindLastChar(const char* s, char c) {
    if (!s) return NULL;
    char* last = NULL;
    while (*s != '\0') {
        if (*s == c) last = (char*)s;
        s++;
    }
    return last;
}

static uint16_t ParsePort(const char* str, size_t len) {
    uint32_t val = 0;
    for (size_t i = 0; i < len; i++) {
        if (!IsDigit(str[i])) break;
        val = val * 10 + (str[i] - '0');
        if (val > 65535) return 0;
    }
    return (uint16_t)val;
}

static void ParseQueryParams(ABE_URL* url) {
    if (!url || url->query[0] == '\0') return;
    url->query_param_count = 0;

    const char* ptr = url->query;
    while (*ptr != '\0' && url->query_param_count < ABE_MAX_QUERY_PARAMS) {
        const char* eq = strstr(ptr, "=");
        const char* amp = strstr(ptr, "&");

        if (!eq || (amp && eq > amp)) {
            // Key without value
            size_t key_len = amp ? (size_t)(amp - ptr) : strlen(ptr);
            if (key_len >= sizeof(url->query_params[0].key)) key_len = sizeof(url->query_params[0].key) - 1;
            strncpy(url->query_params[url->query_param_count].key, ptr, key_len);
            url->query_params[url->query_param_count].key[key_len] = '\0';
            url->query_params[url->query_param_count].value[0] = '\0';
            url->query_param_count++;
            if (!amp) break;
            ptr = amp + 1;
        } else {
            size_t key_len = (size_t)(eq - ptr);
            if (key_len >= sizeof(url->query_params[0].key)) key_len = sizeof(url->query_params[0].key) - 1;
            strncpy(url->query_params[url->query_param_count].key, ptr, key_len);
            url->query_params[url->query_param_count].key[key_len] = '\0';

            const char* val_start = eq + 1;
            size_t val_len = amp ? (size_t)(amp - val_start) : strlen(val_start);
            if (val_len >= sizeof(url->query_params[0].value)) val_len = sizeof(url->query_params[0].value) - 1;
            strncpy(url->query_params[url->query_param_count].value, val_start, val_len);
            url->query_params[url->query_param_count].value[val_len] = '\0';
            url->query_param_count++;
            if (!amp) break;
            ptr = amp + 1;
        }
    }
}

ABE_Error ABE_NormalizePath(const char* raw_path, char* out_path, size_t max_len) {
    if (!raw_path || !out_path || max_len == 0) return ABE_ERR_INVALID_PARAM;

    char stack[16][128];
    uint32_t stack_top = 0;

    const char* p = raw_path;
    while (*p != '\0') {
        while (*p == '/') p++;
        if (*p == '\0') break;

        const char* start = p;
        while (*p != '\0' && *p != '/') p++;
        size_t seg_len = (size_t)(p - start);

        if (seg_len == 1 && start[0] == '.') {
            // Ignore current directory '.'
            continue;
        } else if (seg_len == 2 && start[0] == '.' && start[1] == '.') {
            // Parent directory '..'
            if (stack_top > 0) {
                stack_top--;
            }
        } else if (seg_len > 0) {
            if (stack_top < 16) {
                if (seg_len >= 128) seg_len = 127;
                strncpy(stack[stack_top], start, seg_len);
                stack[stack_top][seg_len] = '\0';
                stack_top++;
            }
        }
    }

    out_path[0] = '\0';
    if (stack_top == 0) {
        strncpy(out_path, "/", max_len - 1);
        return ABE_SUCCESS;
    }

    size_t current_len = 0;
    for (uint32_t i = 0; i < stack_top; i++) {
        if (current_len + 1 < max_len) {
            out_path[current_len++] = '/';
            out_path[current_len] = '\0';
        }
        size_t seg_len = strlen(stack[i]);
        if (current_len + seg_len < max_len) {
            strncpy(out_path + current_len, stack[i], seg_len);
            current_len += seg_len;
            out_path[current_len] = '\0';
        }
    }
    return ABE_SUCCESS;
}

ABE_Error ABE_ParseURL(const char* raw_url, ABE_URL* out_url) {
    if (!raw_url || !out_url) return ABE_ERR_INVALID_PARAM;
    memset(out_url, 0, sizeof(ABE_URL));

    size_t raw_len = strlen(raw_url);
    if (raw_len == 0 || raw_len >= ABE_MAX_URL_LEN) return ABE_ERR_INVALID_URL;
    strncpy(out_url->raw_url, raw_url, sizeof(out_url->raw_url) - 1);

    const char* cursor = raw_url;

    // 1. Extract Scheme
    const char* scheme_end = strstr(cursor, ":");
    if (!scheme_end) return ABE_ERR_INVALID_URL;

    size_t scheme_len = (size_t)(scheme_end - cursor);
    if (scheme_len >= sizeof(out_url->scheme_str)) return ABE_ERR_INVALID_URL;

    for (size_t i = 0; i < scheme_len; i++) {
        out_url->scheme_str[i] = ToLower(cursor[i]);
    }
    out_url->scheme_str[scheme_len] = '\0';

    if (strcmp(out_url->scheme_str, "http") == 0) {
        out_url->scheme = ABE_SCHEME_HTTP;
        out_url->port = 80;
        out_url->is_default_port = true;
    } else if (strcmp(out_url->scheme_str, "https") == 0) {
        out_url->scheme = ABE_SCHEME_HTTPS;
        out_url->port = 443;
        out_url->is_default_port = true;
    } else if (strcmp(out_url->scheme_str, "file") == 0) {
        out_url->scheme = ABE_SCHEME_FILE;
    } else if (strcmp(out_url->scheme_str, "about") == 0) {
        out_url->scheme = ABE_SCHEME_ABOUT;
    } else if (strcmp(out_url->scheme_str, "data") == 0) {
        out_url->scheme = ABE_SCHEME_DATA;
    } else {
        return ABE_ERR_INVALID_URL;
    }

    cursor = scheme_end + 1;

    // Handle special schemes (about: and data:)
    if (out_url->scheme == ABE_SCHEME_ABOUT || out_url->scheme == ABE_SCHEME_DATA) {
        strncpy(out_url->path, cursor, sizeof(out_url->path) - 1);
        out_url->is_valid = true;
        ABE_Diag_RecordURLParsed();
        return ABE_SUCCESS;
    }

    // Expect "//" for http, https, file
    if (cursor[0] == '/' && cursor[1] == '/') {
        cursor += 2;
    } else if (out_url->scheme != ABE_SCHEME_FILE) {
        return ABE_ERR_INVALID_URL;
    }

    // 2. Extract Host & Port (for HTTP / HTTPS / FILE)
    if (out_url->scheme == ABE_SCHEME_HTTP || out_url->scheme == ABE_SCHEME_HTTPS) {
        const char* host_end = cursor;
        while (*host_end != '\0' && *host_end != '/' && *host_end != ':' && *host_end != '?' && *host_end != '#') {
            host_end++;
        }

        size_t host_len = (size_t)(host_end - cursor);
        if (host_len == 0 || host_len >= sizeof(out_url->host)) return ABE_ERR_INVALID_URL;
        strncpy(out_url->host, cursor, host_len);
        out_url->host[host_len] = '\0';
        cursor = host_end;

        // Custom Port
        if (*cursor == ':') {
            cursor++;
            const char* port_end = cursor;
            while (*port_end != '\0' && *port_end != '/' && *port_end != '?' && *port_end != '#') {
                port_end++;
            }
            uint16_t parsed_port = ParsePort(cursor, (size_t)(port_end - cursor));
            if (parsed_port > 0) {
                out_url->port = parsed_port;
                out_url->is_default_port = false;
            }
            cursor = port_end;
        }
    } else if (out_url->scheme == ABE_SCHEME_FILE) {
        strncpy(out_url->host, "localhost", sizeof(out_url->host) - 1);
    }

    // 3. Extract Path
    char raw_path[1024] = "/";
    if (*cursor == '/') {
        const char* path_end = cursor;
        while (*path_end != '\0' && *path_end != '?' && *path_end != '#') {
            path_end++;
        }
        size_t path_len = (size_t)(path_end - cursor);
        if (path_len >= sizeof(raw_path)) path_len = sizeof(raw_path) - 1;
        strncpy(raw_path, cursor, path_len);
        raw_path[path_len] = '\0';
        cursor = path_end;
    }
    ABE_NormalizePath(raw_path, out_url->path, sizeof(out_url->path));

    // 4. Extract Query String
    if (*cursor == '?') {
        cursor++;
        const char* query_end = cursor;
        while (*query_end != '\0' && *query_end != '#') {
            query_end++;
        }
        size_t query_len = (size_t)(query_end - cursor);
        if (query_len >= sizeof(out_url->query)) query_len = sizeof(out_url->query) - 1;
        strncpy(out_url->query, cursor, query_len);
        out_url->query[query_len] = '\0';
        cursor = query_end;
        ParseQueryParams(out_url);
    }

    // 5. Extract Fragment
    if (*cursor == '#') {
        cursor++;
        strncpy(out_url->fragment, cursor, sizeof(out_url->fragment) - 1);
    }

    out_url->is_valid = ABE_ValidateURL(out_url);
    if (out_url->is_valid) {
        ABE_Diag_RecordURLParsed();
        return ABE_SUCCESS;
    }
    return ABE_ERR_INVALID_URL;
}

bool ABE_ValidateURL(const ABE_URL* url) {
    if (!url) return false;
    if (url->scheme == ABE_SCHEME_UNKNOWN) return false;
    if (url->scheme == ABE_SCHEME_HTTP || url->scheme == ABE_SCHEME_HTTPS) {
        if (url->host[0] == '\0') return false;
    }
    return true;
}

ABE_Error ABE_ResolveRelativeURL(const char* base_url_str, const char* relative_url, char* out_resolved, size_t max_len) {
    if (!base_url_str || !relative_url || !out_resolved || max_len == 0) return ABE_ERR_INVALID_PARAM;

    ABE_URL base_url;
    ABE_Error err = ABE_ParseURL(base_url_str, &base_url);
    if (err != ABE_SUCCESS) return err;

    // Check if relative URL is actually an absolute URL
    if (strstr(relative_url, "://") != NULL || strncmp(relative_url, "about:", 6) == 0 || strncmp(relative_url, "data:", 5) == 0) {
        strncpy(out_resolved, relative_url, max_len - 1);
        out_resolved[max_len - 1] = '\0';
        return ABE_SUCCESS;
    }

    ABE_URL resolved = base_url;
    if (relative_url[0] == '/') {
        // Absolute path on same host
        strncpy(resolved.path, relative_url, sizeof(resolved.path) - 1);
    } else {
        // Relative path
        char combined[1024];
        strncpy(combined, base_url.path, sizeof(combined) - 1);
        char* last_slash = FindLastChar(combined, '/');
        if (last_slash) {
            *(last_slash + 1) = '\0';
            strcat(combined, relative_url);
        } else {
            strncpy(combined, relative_url, sizeof(combined) - 1);
        }
        ABE_NormalizePath(combined, resolved.path, sizeof(resolved.path));
    }

    return ABE_BuildURLString(&resolved, out_resolved, max_len);
}

ABE_Error ABE_BuildURLString(const ABE_URL* url, char* out_buf, size_t max_len) {
    if (!url || !out_buf || max_len == 0) return ABE_ERR_INVALID_PARAM;
    memset(out_buf, 0, max_len);

    if (url->scheme == ABE_SCHEME_ABOUT || url->scheme == ABE_SCHEME_DATA) {
        strncpy(out_buf, url->scheme_str, max_len - 1);
        strcat(out_buf, ":");
        strcat(out_buf, url->path);
        return ABE_SUCCESS;
    }

    strncpy(out_buf, url->scheme_str, max_len - 1);
    strcat(out_buf, "://");
    strcat(out_buf, url->host);

    if (!url->is_default_port) {
        strcat(out_buf, ":");
        // Append port string
        char port_str[8];
        uint16_t p = url->port;
        int idx = 0;
        char tmp[8];
        if (p == 0) tmp[idx++] = '0';
        while (p > 0) {
            tmp[idx++] = '0' + (p % 10);
            p /= 10;
        }
        int pos = 0;
        while (idx > 0) {
            port_str[pos++] = tmp[--idx];
        }
        port_str[pos] = '\0';
        strcat(out_buf, port_str);
    }

    strcat(out_buf, url->path);

    if (url->query[0] != '\0') {
        strcat(out_buf, "?");
        strcat(out_buf, url->query);
    }

    if (url->fragment[0] != '\0') {
        strcat(out_buf, "#");
        strcat(out_buf, url->fragment);
    }

    return ABE_SUCCESS;
}
