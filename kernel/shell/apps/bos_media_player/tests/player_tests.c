#include "player_tests.h"
#include "../core/player_core.h"
#include "../controls/playback_controls.h"
#include "../playlist/playlist_manager.h"

extern void display_print(const char* str);

bwe_error_t bos_media_player_test_hittest(void) {
    BOS_Rect btn = { .x = 100, .y = 100, .w = 40, .h = 40 };

    if (!playback_controls_hittest(&btn, 110, 110)) {
        return 1;
    }
    if (playback_controls_hittest(&btn, 200, 200)) {
        return 1;
    }

    return BWE_SUCCESS;
}

bwe_error_t bos_media_player_test_playlist_queue(void) {
    BOS_PlaylistManager pm;
    playlist_init(&pm);

    playlist_add_item(&pm, "Video 1", "/Media/video1.mp4", 10000000ULL);
    playlist_add_item(&pm, "Video 2", "/Media/video2.mp4", 20000000ULL);

    if (pm.count != 2) return 1;
    if (playlist_get_current_uri(&pm) == NULL) return 1;

    playlist_next(&pm);
    if (pm.current_index != 1) return 1;

    return BWE_SUCCESS;
}

bwe_error_t bos_media_player_test_core_lifecycle(void) {
    PlayerCoreCtx ctx;
    if (player_core_init(&ctx) != BWE_SUCCESS) return 1;

    if (player_core_open_media(&ctx, "/Media/demo.mp4") != BWE_SUCCESS) {
        player_core_shutdown(&ctx);
        return 1;
    }

    player_core_toggle_play_pause(&ctx);
    player_core_stop(&ctx);
    player_core_shutdown(&ctx);

    return BWE_SUCCESS;
}

bwe_error_t bos_media_player_test_1k_playback_stress(void) {
    PlayerCoreCtx ctx;
    player_core_init(&ctx);
    player_core_open_media(&ctx, "/Media/stress.mp4");

    for (int i = 0; i < 1000; i++) {
        player_core_toggle_play_pause(&ctx);
        player_core_seek(&ctx, (uint64_t)(i * 10000));
    }

    player_core_shutdown(&ctx);
    return BWE_SUCCESS;
}

bwe_error_t bos_media_player_tests_run_all(void) {
    display_print("========= BOS MEDIA PLAYER CERTIFICATION TESTS =========\n");

    bwe_error_t res = bos_media_player_test_hittest();
    if (res != BWE_SUCCESS) return res;

    res = bos_media_player_test_playlist_queue();
    if (res != BWE_SUCCESS) return res;

    res = bos_media_player_test_core_lifecycle();
    if (res != BWE_SUCCESS) return res;

    res = bos_media_player_test_1k_playback_stress();
    if (res != BWE_SUCCESS) return res;

    display_print("[BOS:MEDIA_PLAYER_TEST] All Certification Self-Tests PASSED!\n");
    display_print("========================================================\n");

    return BWE_SUCCESS;
}
