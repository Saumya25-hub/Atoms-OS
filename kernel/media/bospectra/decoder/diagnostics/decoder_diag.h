#ifndef DECODER_DIAG_H
#define DECODER_DIAG_H

#include "../include/bospectra_decoder.h"

void bospectra_decoder_diag_init(void);
void bospectra_decoder_diag_shutdown(void);
void bospectra_decoder_diag_record_frame(uint64_t decode_time_us, const char* codec_name, bool success);
void bospectra_decoder_collect_stats(BOSPECTRA_DecoderStats* out_stats);
void bospectra_decoder_dump_telemetry(void);

#endif // DECODER_DIAG_H
