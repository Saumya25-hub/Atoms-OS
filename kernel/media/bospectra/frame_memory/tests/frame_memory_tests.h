#ifndef FRAME_MEMORY_TESTS_H
#define FRAME_MEMORY_TESTS_H

#include "../../include/bospectra_types.h"

bospectra_error_t bospectra_frame_memory_tests_run_all(void);
bospectra_error_t bospectra_frame_memory_test_frame_pool(void);
bospectra_error_t bospectra_frame_memory_test_packet_pool(void);
bospectra_error_t bospectra_frame_memory_test_circular_ring(void);
bospectra_error_t bospectra_frame_memory_test_media_queue(void);
bospectra_error_t bospectra_frame_memory_test_10k_stress(void);

#endif // FRAME_MEMORY_TESTS_H
