#ifndef PLAYBACK_TESTS_H
#define PLAYBACK_TESTS_H

#include "../../include/bospectra_types.h"

bospectra_error_t bospectra_playback_tests_run_all(void);
bospectra_error_t bospectra_playback_test_state_machine(void);
bospectra_error_t bospectra_playback_test_timeline(void);
bospectra_error_t bospectra_playback_test_rapid_controls(void);
bospectra_error_t bospectra_playback_test_1k_session_stress(void);

#endif // PLAYBACK_TESTS_H
