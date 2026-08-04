#include "playlist_manager.h"
#include "kernel/core/lib/include/string.h"

void playlist_init(BOS_PlaylistManager* pm) {
    if (!pm) return;
    memset(pm, 0, sizeof(BOS_PlaylistManager));
}

bool playlist_add_item(BOS_PlaylistManager* pm, const char* title, const char* uri, uint64_t duration_us) {
    if (!pm || !uri || pm->count >= BOS_PLAYLIST_MAX_ITEMS) return false;

    BOS_PlaylistItem* item = &pm->items[pm->count];
    strncpy(item->title, (title ? title : uri), sizeof(item->title) - 1);
    strncpy(item->uri, uri, sizeof(item->uri) - 1);
    item->duration_us = duration_us;

    pm->count++;
    return true;
}

const char* playlist_get_current_uri(const BOS_PlaylistManager* pm) {
    if (!pm || pm->count == 0 || pm->current_index >= pm->count) return NULL;
    return pm->items[pm->current_index].uri;
}

const char* playlist_next(BOS_PlaylistManager* pm) {
    if (!pm || pm->count == 0) return NULL;
    pm->current_index = (pm->current_index + 1) % pm->count;
    return pm->items[pm->current_index].uri;
}

const char* playlist_prev(BOS_PlaylistManager* pm) {
    if (!pm || pm->count == 0) return NULL;
    pm->current_index = (pm->current_index == 0) ? (pm->count - 1) : (pm->current_index - 1);
    return pm->items[pm->current_index].uri;
}
