#include "abe_html_text.h"
#include "../diagnostics/abe_diagnostics.h"
#include "kernel/core/lib/include/string.h"

static bool g_text_engine_initialized = false;

ABE_Error ABE_HTMLText_Init(void) {
    g_text_engine_initialized = true;
    ABE_Log(ABE_LOG_INFO, "TEXT", "ABE Production Text & Unicode Engine initialized");
    return ABE_SUCCESS;
}

ABE_Error ABE_HTMLText_Shutdown(void) {
    g_text_engine_initialized = false;
    return ABE_SUCCESS;
}

bool ABE_HTMLText_ValidateUTF8(const char* str, size_t len) {
    if (!str) return false;
    size_t i = 0;
    while (i < len && str[i] != '\0') {
        unsigned char c = (unsigned char)str[i];
        if (c <= 0x7F) {
            i += 1;
        } else if ((c & 0xE0) == 0xC0) {
            if (i + 1 >= len || (str[i + 1] & 0xC0) != 0x80) return false;
            i += 2;
        } else if ((c & 0xF0) == 0xE0) {
            if (i + 2 >= len || (str[i + 1] & 0xC0) != 0x80 || (str[i + 2] & 0xC0) != 0x80) return false;
            i += 3;
        } else if ((c & 0xF8) == 0xF0) {
            if (i + 3 >= len || (str[i + 1] & 0xC0) != 0x80 || (str[i + 2] & 0xC0) != 0x80 || (str[i + 3] & 0xC0) != 0x80) return false;
            i += 4;
        } else {
            return false;
        }
    }
    return true;
}

void ABE_HTMLText_NormalizeWhitespace(const char* in_text, char* out_text, size_t max_len) {
    if (!in_text || !out_text || max_len == 0) return;
    size_t in_i = 0;
    size_t out_i = 0;
    bool in_whitespace = false;

    while (in_text[in_i] != '\0' && out_i < max_len - 1) {
        char c = in_text[in_i++];
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f') {
            if (!in_whitespace) {
                out_text[out_i++] = ' ';
                in_whitespace = true;
            }
        } else {
            out_text[out_i++] = c;
            in_whitespace = false;
        }
    }
    out_text[out_i] = '\0';
}

bool ABE_HTMLText_IsWhitespaceOnly(const char* text) {
    if (!text) return true;
    for (size_t i = 0; text[i] != '\0'; i++) {
        char c = text[i];
        if (c != ' ' && c != '\t' && c != '\n' && c != '\r' && c != '\f') {
            return false;
        }
    }
    return true;
}
