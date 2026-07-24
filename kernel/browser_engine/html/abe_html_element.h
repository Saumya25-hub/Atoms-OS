#ifndef ABE_HTML_ELEMENT_H
#define ABE_HTML_ELEMENT_H

#include "../../../sdk/include/abe/abe.h"
#include "abe_dom_node.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    HTML_TAG_UNKNOWN = 0,
    HTML_TAG_HTML,
    HTML_TAG_HEAD,
    HTML_TAG_BODY,
    HTML_TAG_TITLE,
    HTML_TAG_META,
    HTML_TAG_LINK,
    HTML_TAG_STYLE,
    HTML_TAG_SCRIPT,
    HTML_TAG_DIV,
    HTML_TAG_SPAN,
    HTML_TAG_P,
    HTML_TAG_H1, HTML_TAG_H2, HTML_TAG_H3, HTML_TAG_H4, HTML_TAG_H5, HTML_TAG_H6,
    HTML_TAG_IMG,
    HTML_TAG_A,
    HTML_TAG_FORM,
    HTML_TAG_INPUT,
    HTML_TAG_BUTTON,
    HTML_TAG_TABLE,
    HTML_TAG_TR,
    HTML_TAG_TD,
    HTML_TAG_UL,
    HTML_TAG_OL,
    HTML_TAG_LI,
    HTML_TAG_CANVAS,
    HTML_TAG_VIDEO,
    HTML_TAG_AUDIO
} ABE_HTMLTagId;

ABE_Error     ABE_HTMLElement_Init(void);
ABE_Error     ABE_HTMLElement_Shutdown(void);

ABE_HTMLTagId ABE_HTMLElement_GetTagId(const char* tag_name);
bool          ABE_HTMLElement_IsVoidElement(const char* tag_name);
bool          ABE_HTMLElement_IsFormattingElement(const char* tag_name);

ABE_Error     ABE_HTMLAttr_Set(ABE_DOMNode* node, const char* name, const char* value);
const char*   ABE_HTMLAttr_Get(const ABE_DOMNode* node, const char* name);
ABE_Error     ABE_HTMLAttr_Remove(ABE_DOMNode* node, const char* name);
bool          ABE_HTMLAttr_Has(const ABE_DOMNode* node, const char* name);

#ifdef __cplusplus
}
#endif

#endif // ABE_HTML_ELEMENT_H
