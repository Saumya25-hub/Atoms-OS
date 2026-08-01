// Engine 20: Render Scheduler
#include "../include/agp_api.h"

void AGP_ScheduleRenderWork(void (*work_cb)(void*), void* user_data) {
    if (work_cb) work_cb(user_data);
}
