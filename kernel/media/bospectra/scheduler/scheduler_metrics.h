/*
 * BOSPECTRA V3 — Scheduler Metrics Subsystem
 * kernel/media/bospectra/scheduler/scheduler_metrics.h
 *
 * Frame scheduler diagnostics engine.
 */

#ifndef BOSPECTRA_V3_SCHEDULER_METRICS_H
#define BOSPECTRA_V3_SCHEDULER_METRICS_H

#include "frame_scheduler.h"

void bospectra_scheduler_metrics_dump(const BOSPECTRA_FrameSchedulerContext* ctx);

#endif /* BOSPECTRA_V3_SCHEDULER_METRICS_H */
