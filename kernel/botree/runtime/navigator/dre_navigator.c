#include "../include/dre_api.h"
#include "kernel/botree/include/botree.h"
#include "kernel/core/lib/include/string.h"

static void update_breadcrumbs(BDeRuntime* rt) {
    if (!rt) return;
    rt->breadcrumb_count = 0;

    // Root segment
    strcpy(rt->breadcrumbs[0].name, "This PC");
    strcpy(rt->breadcrumbs[0].target_path, "/");
    rt->breadcrumb_count = 1;

    if (strcmp(rt->current_dir, "/") == 0) return;

    // Split components
    char temp[BDE_PATH_MAX];
    strcpy(temp, rt->current_dir);

    char path_builder[BDE_PATH_MAX] = "";
    char* ptr = temp;
    if (*ptr == '/') ptr++;

    while (*ptr != '\0' && rt->breadcrumb_count < DRE_MAX_BREADCRUMBS) {
        char segment[BDE_NAME_MAX];
        int idx = 0;
        while (*ptr != '\0' && *ptr != '/' && idx < BDE_NAME_MAX - 1) {
            segment[idx++] = *ptr++;
        }
        segment[idx] = '\0';
        if (*ptr == '/') ptr++;

        if (idx > 0) {
            strcat(path_builder, "/");
            strcat(path_builder, segment);
            strcpy(rt->breadcrumbs[rt->breadcrumb_count].name, segment);
            strcpy(rt->breadcrumbs[rt->breadcrumb_count].target_path, path_builder);
            rt->breadcrumb_count++;
        }
    }
}

int32_t BDeRuntime_Open(BDeRuntime* rt, const char* path) {
    if (!rt || !rt->active || !path) return -1;

    if (BDe_NavOpen(rt->nav_session, path) != 0) return -1;

    const char* curr = BDe_NavGetCurrentDir(rt->nav_session);
    strcpy(rt->current_dir, curr);

    // Refresh enumerated item cache
    BDeDirEntry* entries = NULL;
    uint32_t count = 0;
    if (BDe_ReadDirectory(rt->current_dir, &entries, &count) == 0) {
        rt->item_count = (count > DRE_MAX_ITEMS) ? DRE_MAX_ITEMS : count;
        if (entries && rt->item_count > 0) {
            memcpy(rt->entries, entries, rt->item_count * sizeof(BDeDirEntry));
            BDe_FreeDirectoryListing(entries);
        }
    } else {
        rt->item_count = 0;
    }

    // Reset selection mask
    memset(rt->selected_mask, 0, sizeof(rt->selected_mask));
    rt->focused_index = -1;

    // Rebuild breadcrumb bar
    update_breadcrumbs(rt);
    return 0;
}

int32_t BDeRuntime_Back(BDeRuntime* rt) {
    if (!rt || !rt->active) return -1;
    char path[BDE_PATH_MAX];
    if (BDe_NavBack(rt->nav_session, path, BDE_PATH_MAX) == 0) {
        return BDeRuntime_Open(rt, path);
    }
    return -1;
}

int32_t BDeRuntime_Forward(BDeRuntime* rt) {
    if (!rt || !rt->active) return -1;
    char path[BDE_PATH_MAX];
    if (BDe_NavForward(rt->nav_session, path, BDE_PATH_MAX) == 0) {
        return BDeRuntime_Open(rt, path);
    }
    return -1;
}

int32_t BDeRuntime_Up(BDeRuntime* rt) {
    if (!rt || !rt->active) return -1;
    char path[BDE_PATH_MAX];
    if (BDe_NavUp(rt->nav_session, path, BDE_PATH_MAX) == 0) {
        return BDeRuntime_Open(rt, path);
    }
    return -1;
}

int32_t BDeRuntime_Refresh(BDeRuntime* rt) {
    if (!rt || !rt->active) return -1;
    BDe_InvalidateCache(rt->current_dir);
    return BDeRuntime_Open(rt, rt->current_dir);
}

int32_t BDeRuntime_GetBreadcrumb(BDeRuntime* rt, BDeBreadcrumbSegment* out_segments, uint32_t* out_count) {
    if (!rt || !rt->active || !out_segments || !out_count) return -1;
    memcpy(out_segments, rt->breadcrumbs, rt->breadcrumb_count * sizeof(BDeBreadcrumbSegment));
    *out_count = rt->breadcrumb_count;
    return 0;
}
