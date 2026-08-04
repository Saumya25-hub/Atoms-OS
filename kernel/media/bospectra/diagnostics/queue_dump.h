/*
 * BOSPECTRA V3 — Queue Dump Subsystem
 * kernel/media/bospectra/diagnostics/queue_dump.h
 */

#ifndef BOSPECTRA_V3_QUEUE_DUMP_H
#define BOSPECTRA_V3_QUEUE_DUMP_H

#include "../playback/session/playback_session.h"

void bospectra_dump_queues(const PlaybackSessionCtx* sess);

#endif /* BOSPECTRA_V3_QUEUE_DUMP_H */
