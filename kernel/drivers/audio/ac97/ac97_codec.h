#ifndef AC97_CODEC_H
#define AC97_CODEC_H

#include <stdint.h>
#include <stdbool.h>
#include "ac97_registers.h"

void ac97_codec_init_base(uint16_t nam_bar, uint16_t nabm_bar);

void ac97_codec_write(uint8_t reg, uint16_t value);
uint16_t ac97_codec_read(uint8_t reg);

bool ac97_codec_wait_ready(void);
bool ac97_codec_cold_reset(void);
bool ac97_codec_warm_reset(void);
bool ac97_codec_verify_and_configure(void);

#endif // AC97_CODEC_H
