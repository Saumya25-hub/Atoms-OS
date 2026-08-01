#include "../include/brt_api.h"

int32_t BRT_RegisterRuntime(BRTRuntime* rt, uint32_t window_id) {
    if (!rt || !rt->active) return -1;
    rt->window_id = window_id;
    return 0;
}

BRTRuntime* BRT_GetRuntimeByPID(uint32_t pid) {
    (void)pid;
    return NULL;
}
