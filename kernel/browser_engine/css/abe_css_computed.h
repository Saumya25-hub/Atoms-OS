#ifndef ABE_CSS_COMPUTED_H
#define ABE_CSS_COMPUTED_H

#include "../../../sdk/include/abe/abe.h"
#include "../html/abe_dom_node.h"

#ifdef __cplusplus
extern "C" {
#endif

ABE_Error ABE_CSSComputed_Init(void);
ABE_Error ABE_CSSComputed_Shutdown(void);

ABE_Error ABE_CSSComputed_ComputeDocumentStyles(ABE_DocumentHandle doc_handle);
ABE_Error ABE_CSSComputed_GetNodeStyle(ABE_NodeHandle node_handle, ABE_ComputedStyle* out_style);

#ifdef __cplusplus
}
#endif

#endif // ABE_CSS_COMPUTED_H
