#ifndef STUDIO_DESIGNER_H
#define STUDIO_DESIGNER_H

#include "framework/include/bos_ui.h"

typedef struct {
    uint32_t       control_id;
    char           name[64];
    char           type_name[64];
    BOS_Rect       bounds;
    bool           selected;
} BOS_StudioDesignerNode;

typedef struct {
    BOS_StudioDesignerNode nodes[256];
    uint32_t               node_count;
    int32_t                selected_index;
    char                   form_title[128];
    uint32_t               form_width;
    uint32_t               form_height;
} BOS_StudioForm;

void       BOS_Studio_DesignerInit(BOS_StudioForm* form);
BOS_Result BOS_Studio_AddControl(BOS_StudioForm* form, const char* type_name, int32_t x, int32_t y, uint32_t w, uint32_t h, uint32_t* out_id);
BOS_Result BOS_Studio_SelectControl(BOS_StudioForm* form, int32_t x, int32_t y);
BOS_Result BOS_Studio_DeleteSelectedControl(BOS_StudioForm* form);

#endif /* STUDIO_DESIGNER_H */
