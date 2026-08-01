#include "../include/brt_api.h"
#include "kernel/shell_runtime/include/bsr_api.h"
#include "kernel/core/lib/include/string.h"

int32_t BRT_Open(BRTRuntime* rt, const char* target_path) {
    if (!rt || !rt->active || !rt->shell_rt || !target_path) return -1;
    if (BSR_Open(rt->shell_rt, target_path) == 0) {
        strcpy(rt->current_path, rt->shell_rt->current_path);
        return 0;
    }
    return -1;
}

int32_t BRT_Back(BRTRuntime* rt) {
    if (!rt || !rt->active || !rt->shell_rt) return -1;
    if (BSR_Back(rt->shell_rt) == 0) {
        strcpy(rt->current_path, rt->shell_rt->current_path);
        return 0;
    }
    return -1;
}

int32_t BRT_Forward(BRTRuntime* rt) {
    if (!rt || !rt->active || !rt->shell_rt) return -1;
    if (BSR_Forward(rt->shell_rt) == 0) {
        strcpy(rt->current_path, rt->shell_rt->current_path);
        return 0;
    }
    return -1;
}

int32_t BRT_Up(BRTRuntime* rt) {
    if (!rt || !rt->active || !rt->shell_rt) return -1;
    if (BSR_Up(rt->shell_rt) == 0) {
        strcpy(rt->current_path, rt->shell_rt->current_path);
        return 0;
    }
    return -1;
}

int32_t BRT_Refresh(BRTRuntime* rt) {
    if (!rt || !rt->active || !rt->shell_rt) return -1;
    return BSR_Open(rt->shell_rt, rt->current_path);
}

int32_t BRT_Copy(BRTRuntime* rt) {
    if (!rt || !rt->active || !rt->shell_rt) return -1;
    return BSR_Copy(rt->shell_rt);
}

int32_t BRT_Move(BRTRuntime* rt, const char* dest_dir) {
    if (!rt || !rt->active || !dest_dir) return -1;
    return 0;
}

int32_t BRT_Delete(BRTRuntime* rt, bool send_to_recycle) {
    if (!rt || !rt->active || !rt->shell_rt) return -1;
    return BSR_Delete(rt->shell_rt, send_to_recycle);
}

int32_t BRT_Rename(BRTRuntime* rt, const char* old_name, const char* new_name) {
    if (!rt || !rt->active || !rt->shell_rt) return -1;
    return BSR_Rename(rt->shell_rt, old_name, new_name);
}

int32_t BRT_NewFolder(BRTRuntime* rt, const char* folder_name) {
    if (!rt || !rt->active || !rt->shell_rt) return -1;
    return BSR_NewFolder(rt->shell_rt, folder_name);
}

int32_t BRT_Launch(const char* file_path) {
    return BSR_Launch(file_path);
}
