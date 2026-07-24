#ifndef ABE_FLEX_LAYOUT_H
#define ABE_FLEX_LAYOUT_H

#include "../../../sdk/include/abe/abe.h"
#include "abe_render_tree.h"

#ifdef __cplusplus
extern "C" {
#endif

ABE_Error ABE_FlexLayout_Init(void);
ABE_Error ABE_FlexLayout_Shutdown(void);

void ABE_FlexLayout_Perform(ABE_RenderNode* flex_node, float containing_width, float containing_height);

#ifdef __cplusplus
}
#endif

#endif // ABE_FLEX_LAYOUT_H
