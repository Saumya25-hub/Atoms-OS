#ifndef AUDIO_TESTS_H
#define AUDIO_TESTS_H

#include "../../include/bospectra_types.h"

bospectra_error_t bospectra_audio_tests_run_all(void);
bospectra_error_t bospectra_audio_test_session_lifecycle(void);
bospectra_error_t bospectra_audio_test_queue_fifo(void);
bospectra_error_t bospectra_audio_test_bridge_transmission(void);
bospectra_error_t bospectra_audio_test_1k_pcm_stress(void);

#endif // AUDIO_TESTS_H
