#ifndef PLAYER_TESTS_H
#define PLAYER_TESTS_H

#include "kernel/wm/bwe/include/bwe.h"

bwe_error_t bos_media_player_tests_run_all(void);
bwe_error_t bos_media_player_test_hittest(void);
bwe_error_t bos_media_player_test_playlist_queue(void);
bwe_error_t bos_media_player_test_core_lifecycle(void);
bwe_error_t bos_media_player_test_1k_playback_stress(void);

#endif // PLAYER_TESTS_H
