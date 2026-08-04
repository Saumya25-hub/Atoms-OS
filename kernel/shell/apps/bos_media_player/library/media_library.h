#ifndef MEDIA_LIBRARY_H
#define MEDIA_LIBRARY_H

#include "../playlist/playlist_manager.h"

uint32_t media_library_scan_vfs(BOS_PlaylistManager* out_pm, const char* vfs_path);

#endif // MEDIA_LIBRARY_H
