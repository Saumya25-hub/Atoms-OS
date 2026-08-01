#include "../include/bdr_api.h"

int32_t BDR_SelectIcon(BDrSession* session, uint32_t icon_id, bool add_to_selection) {
    if (!session || !session->active) return -1;

    if (!add_to_selection) {
        BDR_DeselectAll(session);
    }

    for (uint32_t i = 0; i < session->icon_count; i++) {
        if (session->icons[i].icon_id == icon_id) {
            session->icons[i].selected = true;
            session->icons[i].focused = true;
            return 0;
        }
    }
    return -1;
}

int32_t BDR_SelectBox(BDrSession* session, int32_t x1, int32_t y1, int32_t x2, int32_t y2) {
    if (!session || !session->active) return -1;

    int32_t min_x = (x1 < x2) ? x1 : x2;
    int32_t max_x = (x1 > x2) ? x1 : x2;
    int32_t min_y = (y1 < y2) ? y1 : y2;
    int32_t max_y = (y1 > y2) ? y1 : y2;

    session->is_selecting_box = true;
    session->box_x1 = min_x;
    session->box_y1 = min_y;
    session->box_x2 = max_x;
    session->box_y2 = max_y;

    for (uint32_t i = 0; i < session->icon_count; i++) {
        int32_t ix = session->icons[i].pixel_x;
        int32_t iy = session->icons[i].pixel_y;
        if (ix >= min_x && ix <= max_x && iy >= min_y && iy <= max_y) {
            session->icons[i].selected = true;
        }
    }
    return 0;
}

int32_t BDR_DeselectAll(BDrSession* session) {
    if (!session || !session->active) return -1;

    session->is_selecting_box = false;
    for (uint32_t i = 0; i < session->icon_count; i++) {
        session->icons[i].selected = false;
        session->icons[i].focused = false;
    }
    return 0;
}
