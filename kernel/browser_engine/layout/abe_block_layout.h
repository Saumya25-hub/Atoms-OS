#ifndef ABE_BLOCK_LAYOUT_H
#define ABE_BLOCK_LAYOUT_H

#include "../../../sdk/include/abe/abe.h"
#include "abe_render_tree.h"

#ifdef __cplusplus
extern "C" {
#endif

ABE_Error ABE_BlockLayout_Init(void);
ABE_Error ABE_BlockLayout_Shutdown(void);

void ABE_BlockLayout_Perform(ABE_RenderNode* block_node, float containing_width, float containing_height);

#ifdef __cplusplus
}
#endif

#endif // ABE_BLOCK_LAYOUT_H
