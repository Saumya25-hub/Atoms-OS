#ifndef COLOR_TESTS_H
#define COLOR_TESTS_H

#include "../../include/bospectra_types.h"

bospectra_error_t bospectra_color_tests_run_all(void);
bospectra_error_t bospectra_color_test_red_reference(void);
bospectra_error_t bospectra_color_test_green_reference(void);
bospectra_error_t bospectra_color_test_blue_reference(void);
bospectra_error_t bospectra_color_test_black_white_reference(void);
bospectra_error_t bospectra_color_test_1k_conversion_stress(void);

#endif // COLOR_TESTS_H
