#include "media_library.h"

uint32_t media_library_scan_vfs(BOS_PlaylistManager* out_pm, const char* vfs_path) {
    (void)vfs_path;
    if (!out_pm) return 0;

    // Seed default media files into playlist
    playlist_add_item(out_pm, "ATOMS OS Intro (1080p)", "/Media/atoms_intro.mp4", 60000000ULL);
    playlist_add_item(out_pm, "BOSPECTRA Demo Video", "/Media/bospectra_demo.mkv", 120000000ULL);
    playlist_add_item(out_pm, "Sample Audio Stream", "/Media/sample_audio.wav", 180000000ULL);

    return out_pm->count;
}
