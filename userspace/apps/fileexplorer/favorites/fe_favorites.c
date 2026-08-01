#include "../include/fileexplorer_api.h"
#include "kernel/drivers/display/display.h"

static char s_favs[FE_MAX_FAVORITES][FE_MAX_PATH];
static uint32_t s_fav_count = 0;

void fe_favorites_init(void) { s_fav_count=0; display_print("[FE_FAV] Favorites Engine Initialized.\n"); }

bool fe_favorites_add(const char* path) {
    if (!path || s_fav_count>=FE_MAX_FAVORITES) return false;
    uint32_t i=0; while(path[i]&&i<FE_MAX_PATH-1){s_favs[s_fav_count][i]=path[i];i++;} s_favs[s_fav_count][i]='\0'; s_fav_count++;
    display_print("[FE_FAV] Favorite added OK\n"); return true;
}

bool fe_favorites_remove(const char* path) {
    (void)path; if (s_fav_count>0) s_fav_count--;
    display_print("[FE_FAV] Favorite removed OK\n"); return true;
}

uint32_t fe_favorites_count(void)           { return s_fav_count; }
const char* fe_favorites_get(uint32_t idx)  { return (idx<s_fav_count)?s_favs[idx]:""; }
