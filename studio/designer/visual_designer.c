#include "studio/include/studio_designer.h"
#include "kernel/core/lib/include/string.h"

void BOS_Studio_DesignerInit(BOS_StudioForm* form) {
    if (!form) return;
    memset(form, 0, sizeof(BOS_StudioForm));
    strncpy(form->form_title, "MainWindow", 127);
    form->form_width = 640;
    form->form_height = 480;
    form->selected_index = -1;
}

BOS_Result BOS_Studio_AddControl(BOS_StudioForm* form, const char* type_name, int32_t x, int32_t y, uint32_t w, uint32_t h, uint32_t* out_id) {
    if (!form || !type_name || !out_id || form->node_count >= 256) {
        return BOS_ERROR_INVALID_ARGUMENT;
    }

    uint32_t idx = form->node_count++;
    BOS_StudioDesignerNode* node = &form->nodes[idx];
    node->control_id = idx + 1;
    strncpy(node->type_name, type_name, 63);
    node->type_name[63] = '\0';
    
    /* Generate default instance name (e.g. btn1, lbl2) */
    strncpy(node->name, type_name, 60);
    node->name[63] = '\0';

    node->bounds.x = x;
    node->bounds.y = y;
    node->bounds.width = w;
    node->bounds.height = h;
    node->selected = false;

    *out_id = node->control_id;
    return BOS_SUCCESS;
}

BOS_Result BOS_Studio_SelectControl(BOS_StudioForm* form, int32_t x, int32_t y) {
    if (!form) return BOS_ERROR_INVALID_ARGUMENT;

    form->selected_index = -1;
    for (int32_t i = (int32_t)form->node_count - 1; i >= 0; i--) {
        BOS_StudioDesignerNode* node = &form->nodes[i];
        node->selected = false;

        if (x >= node->bounds.x && x < node->bounds.x + (int32_t)node->bounds.width &&
            y >= node->bounds.y && y < node->bounds.y + (int32_t)node->bounds.height) {
            
            node->selected = true;
            form->selected_index = i;
            break;
        }
    }

    return (form->selected_index != -1) ? BOS_SUCCESS : BOS_ERROR_NOT_FOUND;
}

BOS_Result BOS_Studio_DeleteSelectedControl(BOS_StudioForm* form) {
    if (!form || form->selected_index < 0 || form->selected_index >= (int32_t)form->node_count) {
        return BOS_ERROR_NOT_FOUND;
    }

    uint32_t sel = (uint32_t)form->selected_index;
    for (uint32_t i = sel; i < form->node_count - 1; i++) {
        form->nodes[i] = form->nodes[i + 1];
    }
    form->node_count--;
    form->selected_index = -1;
    return BOS_SUCCESS;
}
