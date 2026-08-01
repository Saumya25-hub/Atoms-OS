#include "../include/bdr_api.h"
#include "kernel/core/lib/include/string.h"

static uint32_t s_next_icon_id = 100;

int32_t BDR_AddIcon(BDrSession* session, const char* name, const char* path, BDrIconType type, int32_t grid_x, int32_t grid_y) {
    if (!session || !session->active || !name || !path) return -1;
    if (session->icon_count >= BDR_MAX_ICONS) return -1;

    BDrIconNode* node = &session->icons[session->icon_count++];
    memset(node, 0, sizeof(BDrIconNode));
    node->icon_id = s_next_icon_id++;
    strcpy(node->name, name);
    strcpy(node->target_path, path);
    node->type = type;
    node->grid_x = grid_x;
    node->grid_y = grid_y;
    node->pixel_x = grid_x * BDR_GRID_CELL_WIDTH + 20;
    node->pixel_y = grid_y * BDR_GRID_CELL_HEIGHT + 20;
    return (int32_t)node->icon_id;
}

int32_t BDR_RemoveIcon(BDrSession* session, uint32_t icon_id) {
    if (!session || !session->active) return -1;

    for (uint32_t i = 0; i < session->icon_count; i++) {
        if (session->icons[i].icon_id == icon_id) {
            for (uint32_t j = i; j < session->icon_count - 1; j++) {
                session->icons[j] = session->icons[j + 1];
            }
            session->icon_count--;
            return 0;
        }
    }
    return -1;
}

int32_t BDR_AutoArrangeIcons(BDrSession* session) {
    if (!session || !session->active) return -1;

    int32_t curr_x = 0;
    int32_t curr_y = 0;
    int32_t max_rows = 6;

    for (uint32_t i = 0; i < session->icon_count; i++) {
        session->icons[i].grid_x = curr_x;
        session->icons[i].grid_y = curr_y;
        session->icons[i].pixel_x = curr_x * BDR_GRID_CELL_WIDTH + 20;
        session->icons[i].pixel_y = curr_y * BDR_GRID_CELL_HEIGHT + 20;

        curr_y++;
        if (curr_y >= max_rows) {
            curr_y = 0;
            curr_x++;
        }
    }
    return 0;
}
