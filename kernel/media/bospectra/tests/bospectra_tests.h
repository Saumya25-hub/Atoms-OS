#ifndef BOSPECTRA_TESTS_H
#define BOSPECTRA_TESTS_H

#include "../include/bospectra_types.h"

// Self-Test Runner Entry Points
bospectra_error_t bospectra_tests_run_all(void);
bospectra_error_t bospectra_tests_test_lifecycle(void);
bospectra_error_t bospectra_tests_test_memory(void);
bospectra_error_t bospectra_tests_test_packet_buffer(void);
bospectra_error_t bospectra_tests_test_stream_registry(void);

#endif // BOSPECTRA_TESTS_H
