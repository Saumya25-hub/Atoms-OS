#ifndef ABE_REFLOW_H
#define ABE_REFLOW_H

#include "../../../sdk/include/abe/abe.h"
#include "abe_render_tree.h"

#ifdef __cplusplus
extern "C" {
#endif

ABE_Error ABE_Reflow_Init(void);
ABE_Error ABE_Reflow_Shutdown(void);

ABE_Error ABE_Reflow_Perform(ABE_RenderTreeHandle tree_handle);
ABE_Error ABE_Reflow_MarkNodeDirty(ABE_RenderTreeHandle tree_handle, ABE_NodeHandle dom_handle);

#ifdef __cplusplus
}
#endif

#endif // ABE_REFLOW_H
