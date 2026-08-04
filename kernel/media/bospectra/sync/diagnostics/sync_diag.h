#ifndef SYNC_DIAG_H
#define SYNC_DIAG_H

#include "../include/bospectra_sync.h"

void bospectra_sync_diag_init(void);
void bospectra_sync_diag_shutdown(void);
void bospectra_sync_diag_record_decision(bospectra_sync_decision_t decision);
void bospectra_sync_collect_stats(BOSPECTRA_SyncStats* out_stats);
void bospectra_sync_dump_telemetry(void);

#endif // SYNC_DIAG_H
