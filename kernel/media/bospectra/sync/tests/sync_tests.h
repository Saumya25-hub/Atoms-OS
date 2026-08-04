#ifndef SYNC_TESTS_H
#define SYNC_TESTS_H

#include "../../include/bospectra_types.h"

bospectra_error_t bospectra_sync_tests_run_all(void);
bospectra_error_t bospectra_sync_test_master_clock(void);
bospectra_error_t bospectra_sync_test_frame_scheduler(void);
bospectra_error_t bospectra_sync_test_drift_detector(void);
bospectra_error_t bospectra_sync_test_1k_sync_stress(void);

#endif // SYNC_TESTS_H
