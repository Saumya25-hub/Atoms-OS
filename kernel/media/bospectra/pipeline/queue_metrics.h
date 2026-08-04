/*
 * BOSPECTRA V3 — Queue Metrics Diagnostics
 * kernel/media/bospectra/pipeline/queue_metrics.h
 *
 * Pipeline diagnostics engine printing queue usage %, latency, and dropped frames.
 */

#ifndef BOSPECTRA_V3_QUEUE_METRICS_H
#define BOSPECTRA_V3_QUEUE_METRICS_H

#include "pipeline_engine.h"

void bospectra_queue_metrics_dump(const BOSPECTRA_PipelineContext* ctx);

#endif /* BOSPECTRA_V3_QUEUE_METRICS_H */
