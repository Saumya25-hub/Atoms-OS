#ifndef ABE_INLINE_LAYOUT_H
#define ABE_INLINE_LAYOUT_H

#include "../../../sdk/include/abe/abe.h"
#include "abe_render_tree.h"

#ifdef __cplusplus
extern "C" {
#endif

ABE_Error ABE_InlineLayout_Init(void);
ABE_Error ABE_InlineLayout_Shutdown(void);

void ABE_InlineLayout_Perform(ABE_RenderNode* inline_node, float start_x, float start_y, float max_width);

#ifdef __cplusplus
}
#endif

#endif // ABE_INLINE_LAYOUT_H
