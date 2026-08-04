/*
 * BOSPECTRA V3 — Queue Dump Implementation
 * kernel/media/bospectra/diagnostics/queue_dump.c
 */

#include "queue_dump.h"
#include "../pipeline/queue_metrics.h"

void bospectra_dump_queues(const PlaybackSessionCtx* sess) {
    if (sess) {
        bospectra_queue_metrics_dump(&sess->pipeline_ctx);
    }
}
