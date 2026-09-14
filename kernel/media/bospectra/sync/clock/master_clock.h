#ifndef MASTER_CLOCK_H
#define MASTER_CLOCK_H

#include "../include/bospectra_sync_types.h"
#include "../../include/bospectra_errors.h"

void                     master_clock_init(void);
void                     master_clock_shutdown(void);
void                     master_clock_set_source(bospectra_clock_source_t source);
bospectra_clock_source_t master_clock_get_source(void);
void                     master_clock_update_audio_pts(uint64_t audio_pts_us);
uint64_t                 master_clock_get_time_us(void);
void                     master_clock_set_speed(uint32_t speed_x100);
uint32_t                 master_clock_get_speed(void);
void                     master_clock_reset(uint64_t start_pts_us);
void                     master_clock_seek(uint64_t target_pts_us);

#endif // MASTER_CLOCK_H
