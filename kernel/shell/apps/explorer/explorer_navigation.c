#include "explorer_navigation.h"
#include "kernel/core/lib/include/string.h"

void explorer_nav_init(ExplorerNavHistory* nav) {
    if (!nav) return;
    memset(nav, 0, sizeof(ExplorerNavHistory));
    nav->current_index = -1;
    nav->count = 0;
}

bool explorer_nav_push(ExplorerNavHistory* nav, const char* path) {
    if (!nav || !path || strlen(path) == 0) return false;
    
    // Do not push duplicate of current path
    if (nav->current_index >= 0 && strcmp(nav->items[nav->current_index], path) == 0) {
        return true;
    }
    
    if (nav->current_index + 1 < BSEC_NAV_HISTORY_MAX) {
        nav->current_index++;
        strcpy(nav->items[nav->current_index], path);
        nav->count = nav->current_index + 1;
    } else {
        // Shift left
        for (int i = 0; i < BSEC_NAV_HISTORY_MAX - 1; i++) {
            strcpy(nav->items[i], nav->items[i + 1]);
        }
        strcpy(nav->items[BSEC_NAV_HISTORY_MAX - 1], path);
        nav->current_index = BSEC_NAV_HISTORY_MAX - 1;
        nav->count = BSEC_NAV_HISTORY_MAX;
    }
    return true;
}

const char* explorer_nav_back(ExplorerNavHistory* nav) {
    if (!nav || nav->current_index <= 0) return NULL;
    nav->current_index--;
    return nav->items[nav->current_index];
}

const char* explorer_nav_forward(ExplorerNavHistory* nav) {
    if (!nav || nav->current_index + 1 >= nav->count) return NULL;
    nav->current_index++;
    return nav->items[nav->current_index];
}

bool explorer_nav_can_back(const ExplorerNavHistory* nav) {
    return nav && nav->current_index > 0;
}

bool explorer_nav_can_forward(const ExplorerNavHistory* nav) {
    return nav && nav->current_index + 1 < nav->count;
}

void explorer_nav_get_parent(const char* current_path, char* out_parent) {
    if (!current_path || !out_parent) return;
    strcpy(out_parent, current_path);
    if (strcmp(out_parent, "/") == 0) return;
    
    int len = strlen(out_parent);
    int last_slash = -1;
    for (int i = 0; i < len; i++) {
        if (out_parent[i] == '/') last_slash = i;
    }
    if (last_slash > 0) {
        out_parent[last_slash] = '\0';
    } else if (last_slash == 0) {
        out_parent[1] = '\0';
    }
}
