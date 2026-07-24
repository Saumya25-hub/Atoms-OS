#ifndef ABE_CSS_CASCADE_H
#define ABE_CSS_CASCADE_H

#include "../../../sdk/include/abe/abe.h"
#include "abe_css_parser.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    ABE_CSSDeclaration declaration;
    ABE_CSSOrigin origin;
    uint32_t specificity_a;
    uint32_t specificity_b;
    uint32_t specificity_c;
    uint32_t rule_index;
} ABE_CascadedProperty;

ABE_Error ABE_CSSCascade_Init(void);
ABE_Error ABE_CSSCascade_Shutdown(void);

ABE_StylesheetHandle ABE_CSSCascade_GetUserAgentStylesheet(void);
bool                 ABE_CSSCascade_ShouldOverride(const ABE_CascadedProperty* existing, const ABE_CascadedProperty* incoming);

#ifdef __cplusplus
}
#endif

#endif // ABE_CSS_CASCADE_H
