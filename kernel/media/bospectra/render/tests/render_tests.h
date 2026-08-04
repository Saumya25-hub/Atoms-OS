#ifndef RENDER_TESTS_H
#define RENDER_TESTS_H

#include "../../include/bospectra_types.h"

bospectra_error_t bospectra_render_tests_run_all(void);
bospectra_error_t bospectra_render_test_session_lifecycle(void);
bospectra_error_t bospectra_render_test_texture_pool_recycling(void);
bospectra_error_t bospectra_render_test_boundary_guards(void);
bospectra_error_t bospectra_render_test_1k_render_stress(void);

#endif // RENDER_TESTS_H
