#include "../include/dre_api.h"
#include "kernel/botree/include/botree.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/core/memory/heap/include/heap.h"

#define MAX_RUNTIMES 32

static BDeRuntime s_runtime_pool[MAX_RUNTIMES];
static uint32_t   s_next_runtime_id = 1;
static uint32_t   s_cache_hits = 0;
static uint32_t   s_cache_misses = 0;

BDeRuntime* BDeRuntime_Create(uint32_t owner_pid, BDeViewMode initial_view_mode) {
    for (int i = 0; i < MAX_RUNTIMES; i++) {
        if (!s_runtime_pool[i].active) {
            memset(&s_runtime_pool[i], 0, sizeof(BDeRuntime));
            s_runtime_pool[i].active = true;
            s_runtime_pool[i].runtime_id = s_next_runtime_id++;
            s_runtime_pool[i].owner_pid = owner_pid;
            s_runtime_pool[i].view_mode = initial_view_mode;
            s_runtime_pool[i].focused_index = -1;
            s_runtime_pool[i].hovered_index = -1;
            s_runtime_pool[i].active_index = -1;
            s_runtime_pool[i].nav_session = BDe_NavCreateSession(owner_pid);

            // Open Root Directory by Default
            BDeRuntime_Open(&s_runtime_pool[i], "/");
            return &s_runtime_pool[i];
        }
    }
    return NULL;
}

void BDeRuntime_Destroy(BDeRuntime* rt) {
    if (!rt || !rt->active) return;
    if (rt->nav_session != 0) {
        BDe_NavDestroySession(rt->nav_session);
    }
    if (rt->watch_handle != 0) {
        BDe_WatchUnsubscribe(rt->watch_handle);
    }
    rt->active = false;
}

const char* BDeRuntime_GetCurrentDirectory(BDeRuntime* rt) {
    if (!rt || !rt->active) return "/";
    return rt->current_dir;
}

void BDeRuntime_GetDiagnostics(BDeRuntimeDiagnostics* out_diag) {
    if (!out_diag) return;
    memset(out_diag, 0, sizeof(BDeRuntimeDiagnostics));

    for (int i = 0; i < MAX_RUNTIMES; i++) {
        if (s_runtime_pool[i].active) {
            out_diag->active_runtimes++;
            out_diag->total_open_sessions++;
        }
    }
    out_diag->cache_hits = s_cache_hits;
    out_diag->cache_misses = s_cache_misses;
    out_diag->memory_used_bytes = sizeof(s_runtime_pool);
}
