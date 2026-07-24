#ifndef ABE_HTML_H
#define ABE_HTML_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// HTML5 Tokenizer & Recovery Parser Subsystem
typedef enum {
    ABE_TAG_UNKNOWN,
    ABE_TAG_HTML,
    ABE_TAG_HEAD,
    ABE_TAG_BODY,
    ABE_TAG_TITLE,
    ABE_TAG_META,
    ABE_TAG_LINK,
    ABE_TAG_SCRIPT,
    ABE_TAG_STYLE,
    ABE_TAG_DIV,
    ABE_TAG_SPAN,
    ABE_TAG_P,
    ABE_TAG_A,
    ABE_TAG_IMG,
    ABE_TAG_TABLE,
    ABE_TAG_FORM,
    ABE_TAG_INPUT,
    ABE_TAG_BUTTON,
    ABE_TAG_CANVAS,
    ABE_TAG_VIDEO,
    ABE_TAG_AUDIO,
    ABE_TAG_SVG
} ABE_TagType;

typedef struct {
    ABE_TagType type;
    char        name[32];
    bool        is_self_closing;
} ABE_HTMLToken;

void ABE_HTML_Init(void);
ABE_TagType ABE_HTML_ParseTag(const char* tag_str);
bool ABE_HTML_DecodeEntity(const char* entity, char* out_char);

#ifdef __cplusplus
}
#endif

#endif // ABE_HTML_H
