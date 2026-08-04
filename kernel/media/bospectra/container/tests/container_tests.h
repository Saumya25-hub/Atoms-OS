#ifndef CONTAINER_TESTS_H
#define CONTAINER_TESTS_H

#include "../../include/bospectra_types.h"

bospectra_error_t bospectra_container_tests_run_all(void);
bospectra_error_t bospectra_container_test_probing(void);
bospectra_error_t bospectra_container_test_mp4(void);
bospectra_error_t bospectra_container_test_avi(void);
bospectra_error_t bospectra_container_test_bounds_security(void);

#endif // CONTAINER_TESTS_H
