#ifndef ABE_CSS_SELECTOR_H
#define ABE_CSS_SELECTOR_H

#include "../../../sdk/include/abe/abe.h"
#include "../html/abe_dom_node.h"

#ifdef __cplusplus
extern "C" {
#endif

ABE_Error ABE_CSSSelector_Init(void);
ABE_Error ABE_CSSSelector_Shutdown(void);

bool      ABE_CSSSelector_Match(const ABE_DOMNode* node, const char* selector_str);
int       ABE_CSSSelector_CompareSpecificity(uint32_t a1, uint32_t b1, uint32_t c1, uint32_t a2, uint32_t b2, uint32_t c2);

#ifdef __cplusplus
}
#endif

#endif // ABE_CSS_SELECTOR_H
