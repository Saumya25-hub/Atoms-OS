#include "../include/brt_api.h"

int32_t BRT_BeginDrag(BRTRuntime* rt, int32_t start_x, int32_t start_y) {
    if (!rt || !rt->active) return -1;
    rt->is_dragging = true;
    rt->drag_start_x = start_x;
    rt->drag_start_y = start_y;
    rt->drag_cur_x = start_x;
    rt->drag_cur_y = start_y;
    return 0;
}

int32_t BRT_UpdateDrag(BRTRuntime* rt, int32_t cur_x, int32_t cur_y) {
    if (!rt || !rt->active || !rt->is_dragging) return -1;
    rt->drag_cur_x = cur_x;
    rt->drag_cur_y = cur_y;
    return 0;
}

int32_t BRT_EndDrag(BRTRuntime* rt, const char* drop_target) {
    if (!rt || !rt->active || !rt->is_dragging) return -1;
    (void)drop_target;
    rt->is_dragging = false;
    return 0;
}
