#include "../include/brt_api.h"

int32_t BRT_SaveRuntime(BRTRuntime* rt) {
    if (!rt || !rt->active) return -1;
    return 0;
}

int32_t BRT_RestoreRuntime(BRTRuntime* rt) {
    if (!rt || !rt->active) return -1;
    return 0;
}
