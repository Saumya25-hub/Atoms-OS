#ifndef FRAME_SCHEDULER_H
#define FRAME_SCHEDULER_H

#include "../include/bospectra_sync_types.h"
#include "../../include/bospectra_errors.h"

#define BOSPECTRA_SYNC_LATE_THRESHOLD_US  40000LL // 40ms late threshold -> DROP
#define BOSPECTRA_SYNC_EARLY_THRESHOLD_US 10000LL // 10ms early threshold -> PRESENT
#define BOSPECTRA_SYNC_FUTURE_BOUND_US   500000LL // 500ms max future window -> WAIT

void                      frame_scheduler_init(void);
void                      frame_scheduler_shutdown(void);
bospectra_sync_decision_t frame_scheduler_evaluate(uint64_t frame_pts_us, uint64_t master_clock_us);

#endif // FRAME_SCHEDULER_H
