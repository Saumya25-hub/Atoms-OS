/*
 * BOSPECTRA V3 — Decoder Manager
 * kernel/media/bospectra/manager/decoder_manager.h
 *
 * Automatic decoder registration, codec routing, and stream matching.
 */

#ifndef BOSPECTRA_DECODER_MANAGER_H
#define BOSPECTRA_DECODER_MANAGER_H

#include "../include/bospectra_types.h"
#include "../include/bospectra_errors.h"
#include "../decoder/registry/decoder_registry.h"
#include <stdint.h>
#include <stdbool.h>

void bospectra_decoder_manager_init(void);
void bospectra_decoder_manager_shutdown(void);

const BOSPECTRA_DecoderDriver* bospectra_decoder_resolve(const BOSPECTRA_StreamDescriptor* stream_desc);

#endif /* BOSPECTRA_DECODER_MANAGER_H */
