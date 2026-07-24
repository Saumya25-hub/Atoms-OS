#ifndef ABE_LAYOUT_DIAG_H
#define ABE_LAYOUT_DIAG_H

#include "../../../sdk/include/abe/abe.h"
#include "abe_render_tree.h"

#ifdef __cplusplus
extern "C" {
#endif

ABE_Error ABE_LayoutDiag_Init(void);
ABE_Error ABE_LayoutDiag_Shutdown(void);

void ABE_LayoutDiag_DumpTree(ABE_RenderTreeHandle tree_handle);

#ifdef __cplusplus
}
#endif

#endif // ABE_LAYOUT_DIAG_H
