#ifndef DRIFT_DETECTOR_H
#define DRIFT_DETECTOR_H

#include <stdint.h>
#include <stdbool.h>

void    drift_detector_init(void);
void    drift_detector_shutdown(void);
void    drift_detector_update(uint64_t audio_pts_us, uint64_t video_pts_us);
int64_t drift_detector_get_current_drift_us(void);
int64_t drift_detector_get_avg_drift_us(void);
int64_t drift_detector_get_max_drift_us(void);
void    drift_detector_reset(void);

#endif // DRIFT_DETECTOR_H
