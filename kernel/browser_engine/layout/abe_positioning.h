#ifndef ABE_POSITIONING_H
#define ABE_POSITIONING_H

#include "../../../sdk/include/abe/abe.h"
#include "abe_render_tree.h"

#ifdef __cplusplus
extern "C" {
#endif

ABE_Error ABE_Positioning_Init(void);
ABE_Error ABE_Positioning_Shutdown(void);

void ABE_Positioning_Apply(ABE_RenderNode* node, float containing_width, float containing_height);
void ABE_Overflow_Compute(ABE_RenderNode* node);

#ifdef __cplusplus
}
#endif

#endif // ABE_POSITIONING_H
