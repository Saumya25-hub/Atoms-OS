#ifndef ABE_BOX_MODEL_H
#define ABE_BOX_MODEL_H

#include "../../../sdk/include/abe/abe.h"
#include "abe_render_tree.h"

#ifdef __cplusplus
extern "C" {
#endif

ABE_Error ABE_BoxModel_Init(void);
ABE_Error ABE_BoxModel_Shutdown(void);

void ABE_BoxModel_Compute(ABE_RenderNode* node, float containing_width, float containing_height);
float ABE_BoxModel_CollapseMargins(float margin_a, float margin_b);

#ifdef __cplusplus
}
#endif

#endif // ABE_BOX_MODEL_H
