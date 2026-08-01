#ifndef BSEC_EXPLORER_NAVIGATION_H
#define BSEC_EXPLORER_NAVIGATION_H

#include <stdint.h>
#include <stdbool.h>

#define BSEC_NAV_HISTORY_MAX 32

typedef struct {
    char items[BSEC_NAV_HISTORY_MAX][256];
    int32_t current_index;
    int32_t count;
} ExplorerNavHistory;

void explorer_nav_init(ExplorerNavHistory* nav);
bool explorer_nav_push(ExplorerNavHistory* nav, const char* path);
const char* explorer_nav_back(ExplorerNavHistory* nav);
const char* explorer_nav_forward(ExplorerNavHistory* nav);
bool explorer_nav_can_back(const ExplorerNavHistory* nav);
bool explorer_nav_can_forward(const ExplorerNavHistory* nav);
void explorer_nav_get_parent(const char* current_path, char* out_parent);

#endif // BSEC_EXPLORER_NAVIGATION_H
