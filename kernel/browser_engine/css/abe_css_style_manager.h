#ifndef ABE_CSS_STYLE_MANAGER_H
#define ABE_CSS_STYLE_MANAGER_H

#include "../../../sdk/include/abe/abe.h"

#ifdef __cplusplus
extern "C" {
#endif

ABE_Error ABE_StyleManager_Init(void);
ABE_Error ABE_StyleManager_Shutdown(void);

ABE_Error ABE_StyleManager_LoadDocumentStyles(ABE_DocumentHandle doc_handle);

#ifdef __cplusplus
}
#endif

#endif // ABE_CSS_STYLE_MANAGER_H
