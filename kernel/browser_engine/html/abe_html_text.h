#ifndef ABE_HTML_TEXT_H
#define ABE_HTML_TEXT_H

#include "../../../sdk/include/abe/abe.h"

#ifdef __cplusplus
extern "C" {
#endif

ABE_Error ABE_HTMLText_Init(void);
ABE_Error ABE_HTMLText_Shutdown(void);

bool      ABE_HTMLText_ValidateUTF8(const char* str, size_t len);
void      ABE_HTMLText_NormalizeWhitespace(const char* in_text, char* out_text, size_t max_len);
bool      ABE_HTMLText_IsWhitespaceOnly(const char* text);

#ifdef __cplusplus
}
#endif

#endif // ABE_HTML_TEXT_H
