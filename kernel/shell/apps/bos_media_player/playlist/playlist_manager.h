#ifndef PLAYLIST_MANAGER_H
#define PLAYLIST_MANAGER_H

#include <stdint.h>
#include <stdbool.h>

#define BOS_PLAYLIST_MAX_ITEMS 64U

typedef struct {
    char title[64];
    char uri[128];
    uint64_t duration_us;
} BOS_PlaylistItem;

typedef struct {
    BOS_PlaylistItem items[BOS_PLAYLIST_MAX_ITEMS];
    uint32_t         count;
    uint32_t         current_index;
} BOS_PlaylistManager;

void        playlist_init(BOS_PlaylistManager* pm);
bool        playlist_add_item(BOS_PlaylistManager* pm, const char* title, const char* uri, uint64_t duration_us);
const char* playlist_get_current_uri(const BOS_PlaylistManager* pm);
const char* playlist_next(BOS_PlaylistManager* pm);
const char* playlist_prev(BOS_PlaylistManager* pm);

#endif // PLAYLIST_MANAGER_H
