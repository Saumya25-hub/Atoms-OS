#include "abe_html.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/wm/bwe/include/bwe.h"

extern void bwe_log(const char* level, const char* msg);

void ABE_HTML_Init(void) {
    bwe_log("INFO", "ABE HTML5 Tokenizer & Recovery Parser Subsystem Initialized");
}

ABE_TagType ABE_HTML_ParseTag(const char* tag_str) {
    if (!tag_str) return ABE_TAG_UNKNOWN;

    if (strcmp(tag_str, "html") == 0) return ABE_TAG_HTML;
    if (strcmp(tag_str, "head") == 0) return ABE_TAG_HEAD;
    if (strcmp(tag_str, "body") == 0) return ABE_TAG_BODY;
    if (strcmp(tag_str, "title") == 0) return ABE_TAG_TITLE;
    if (strcmp(tag_str, "div") == 0) return ABE_TAG_DIV;
    if (strcmp(tag_str, "span") == 0) return ABE_TAG_SPAN;
    if (strcmp(tag_str, "p") == 0) return ABE_TAG_P;
    if (strcmp(tag_str, "a") == 0) return ABE_TAG_A;
    if (strcmp(tag_str, "img") == 0) return ABE_TAG_IMG;
    if (strcmp(tag_str, "input") == 0) return ABE_TAG_INPUT;
    if (strcmp(tag_str, "button") == 0) return ABE_TAG_BUTTON;
    if (strcmp(tag_str, "form") == 0) return ABE_TAG_FORM;
    if (strcmp(tag_str, "canvas") == 0) return ABE_TAG_CANVAS;
    return ABE_TAG_UNKNOWN;
}

bool ABE_HTML_DecodeEntity(const char* entity, char* out_char) {
    if (!entity || !out_char) return false;
    if (strcmp(entity, "&amp;") == 0) { *out_char = '&'; return true; }
    if (strcmp(entity, "&lt;") == 0) { *out_char = '<'; return true; }
    if (strcmp(entity, "&gt;") == 0) { *out_char = '>'; return true; }
    if (strcmp(entity, "&quot;") == 0) { *out_char = '"'; return true; }
    return false;
}
