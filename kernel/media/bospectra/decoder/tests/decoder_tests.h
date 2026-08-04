#ifndef DECODER_TESTS_H
#define DECODER_TESTS_H

#include "../../include/bospectra_types.h"

bospectra_error_t bospectra_decoder_tests_run_all(void);
bospectra_error_t bospectra_decoder_test_bitstream_reader(void);
bospectra_error_t bospectra_decoder_test_8x8_idct(void);
bospectra_error_t bospectra_decoder_test_mjpeg(void);
bospectra_error_t bospectra_decoder_test_1k_decode_stress(void);

#endif // DECODER_TESTS_H
