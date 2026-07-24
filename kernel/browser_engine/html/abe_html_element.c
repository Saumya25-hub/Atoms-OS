#include "abe_html_element.h"
#include "../diagnostics/abe_diagnostics.h"
#include "kernel/core/lib/include/string.h"

static bool g_element_lib_initialized = false;

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

ABE_Error ABE_HTMLElement_Init(void) {
    g_element_lib_initialized = true;
    ABE_Log(ABE_LOG_INFO, "ELEMENT", "ABE HTML Element Library & Attribute Engine initialized");
    return ABE_SUCCESS;
}

ABE_Error ABE_HTMLElement_Shutdown(void) {
    g_element_lib_initialized = false;
    return ABE_SUCCESS;
}

ABE_HTMLTagId ABE_HTMLElement_GetTagId(const char* tag_name) {
    if (!tag_name) return HTML_TAG_UNKNOWN;

    if (StrCaseCmp(tag_name, "html") == 0) return HTML_TAG_HTML;
    if (StrCaseCmp(tag_name, "head") == 0) return HTML_TAG_HEAD;
    if (StrCaseCmp(tag_name, "body") == 0) return HTML_TAG_BODY;
    if (StrCaseCmp(tag_name, "title") == 0) return HTML_TAG_TITLE;
    if (StrCaseCmp(tag_name, "meta") == 0) return HTML_TAG_META;
    if (StrCaseCmp(tag_name, "link") == 0) return HTML_TAG_LINK;
    if (StrCaseCmp(tag_name, "style") == 0) return HTML_TAG_STYLE;
    if (StrCaseCmp(tag_name, "script") == 0) return HTML_TAG_SCRIPT;
    if (StrCaseCmp(tag_name, "div") == 0) return HTML_TAG_DIV;
    if (StrCaseCmp(tag_name, "span") == 0) return HTML_TAG_SPAN;
    if (StrCaseCmp(tag_name, "p") == 0) return HTML_TAG_P;
    if (StrCaseCmp(tag_name, "h1") == 0) return HTML_TAG_H1;
    if (StrCaseCmp(tag_name, "h2") == 0) return HTML_TAG_H2;
    if (StrCaseCmp(tag_name, "h3") == 0) return HTML_TAG_H3;
    if (StrCaseCmp(tag_name, "h4") == 0) return HTML_TAG_H4;
    if (StrCaseCmp(tag_name, "h5") == 0) return HTML_TAG_H5;
    if (StrCaseCmp(tag_name, "h6") == 0) return HTML_TAG_H6;
    if (StrCaseCmp(tag_name, "img") == 0) return HTML_TAG_IMG;
    if (StrCaseCmp(tag_name, "a") == 0) return HTML_TAG_A;
    if (StrCaseCmp(tag_name, "form") == 0) return HTML_TAG_FORM;
    if (StrCaseCmp(tag_name, "input") == 0) return HTML_TAG_INPUT;
    if (StrCaseCmp(tag_name, "button") == 0) return HTML_TAG_BUTTON;
    if (StrCaseCmp(tag_name, "table") == 0) return HTML_TAG_TABLE;
    if (StrCaseCmp(tag_name, "tr") == 0) return HTML_TAG_TR;
    if (StrCaseCmp(tag_name, "td") == 0) return HTML_TAG_TD;
    if (StrCaseCmp(tag_name, "ul") == 0) return HTML_TAG_UL;
    if (StrCaseCmp(tag_name, "ol") == 0) return HTML_TAG_OL;
    if (StrCaseCmp(tag_name, "li") == 0) return HTML_TAG_LI;
    if (StrCaseCmp(tag_name, "canvas") == 0) return HTML_TAG_CANVAS;
    if (StrCaseCmp(tag_name, "video") == 0) return HTML_TAG_VIDEO;
    if (StrCaseCmp(tag_name, "audio") == 0) return HTML_TAG_AUDIO;

    return HTML_TAG_UNKNOWN;
}

bool ABE_HTMLElement_IsVoidElement(const char* tag_name) {
    if (!tag_name) return false;
    return (StrCaseCmp(tag_name, "area") == 0 ||
            StrCaseCmp(tag_name, "base") == 0 ||
            StrCaseCmp(tag_name, "br") == 0 ||
            StrCaseCmp(tag_name, "col") == 0 ||
            StrCaseCmp(tag_name, "embed") == 0 ||
            StrCaseCmp(tag_name, "hr") == 0 ||
            StrCaseCmp(tag_name, "img") == 0 ||
            StrCaseCmp(tag_name, "input") == 0 ||
            StrCaseCmp(tag_name, "link") == 0 ||
            StrCaseCmp(tag_name, "meta") == 0 ||
            StrCaseCmp(tag_name, "param") == 0 ||
            StrCaseCmp(tag_name, "source") == 0 ||
            StrCaseCmp(tag_name, "track") == 0 ||
            StrCaseCmp(tag_name, "wbr") == 0);
}

bool ABE_HTMLElement_IsFormattingElement(const char* tag_name) {
    if (!tag_name) return false;
    return (StrCaseCmp(tag_name, "a") == 0 ||
            StrCaseCmp(tag_name, "b") == 0 ||
            StrCaseCmp(tag_name, "big") == 0 ||
            StrCaseCmp(tag_name, "code") == 0 ||
            StrCaseCmp(tag_name, "em") == 0 ||
            StrCaseCmp(tag_name, "font") == 0 ||
            StrCaseCmp(tag_name, "i") == 0 ||
            StrCaseCmp(tag_name, "s") == 0 ||
            StrCaseCmp(tag_name, "small") == 0 ||
            StrCaseCmp(tag_name, "strike") == 0 ||
            StrCaseCmp(tag_name, "strong") == 0 ||
            StrCaseCmp(tag_name, "tt") == 0 ||
            StrCaseCmp(tag_name, "u") == 0);
}

ABE_Error ABE_HTMLAttr_Set(ABE_DOMNode* node, const char* name, const char* value) {
    if (!node || !name) return ABE_ERR_INVALID_PARAM;

    // Check if attribute already exists -> Update value
    for (uint32_t i = 0; i < node->attribute_count; i++) {
        if (StrCaseCmp(node->attributes[i].name, name) == 0) {
            strncpy(node->attributes[i].value, value ? value : "", sizeof(node->attributes[i].value) - 1);
            return ABE_SUCCESS;
        }
    }

    if (node->attribute_count >= ABE_MAX_ATTRIBUTES) return ABE_ERR_RESOURCE_EXHAUSTED;

    ABE_DOMAttributeInfo* attr = &node->attributes[node->attribute_count++];
    strncpy(attr->name, name, sizeof(attr->name) - 1);
    strncpy(attr->value, value ? value : "", sizeof(attr->value) - 1);
    return ABE_SUCCESS;
}

const char* ABE_HTMLAttr_Get(const ABE_DOMNode* node, const char* name) {
    if (!node || !name) return NULL;
    for (uint32_t i = 0; i < node->attribute_count; i++) {
        if (StrCaseCmp(node->attributes[i].name, name) == 0) {
            return node->attributes[i].value;
        }
    }
    return NULL;
}

ABE_Error ABE_HTMLAttr_Remove(ABE_DOMNode* node, const char* name) {
    if (!node || !name) return ABE_ERR_INVALID_PARAM;
    for (uint32_t i = 0; i < node->attribute_count; i++) {
        if (StrCaseCmp(node->attributes[i].name, name) == 0) {
            for (uint32_t j = i; j < node->attribute_count - 1; j++) {
                node->attributes[j] = node->attributes[j + 1];
            }
            node->attribute_count--;
            return ABE_SUCCESS;
        }
    }
    return ABE_SUCCESS;
}

bool ABE_HTMLAttr_Has(const ABE_DOMNode* node, const char* name) {
    return (ABE_HTMLAttr_Get(node, name) != NULL);
}
