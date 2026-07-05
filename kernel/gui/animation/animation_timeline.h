#ifndef ANIMATION_TIMELINE_H
#define ANIMATION_TIMELINE_H

#include <stdint.h>
#include <stdbool.h>

// For BOFLOW V1, timeline logic is simplified and handled by the scheduler.
// This header is reserved for V2 complex multi-track timelines.

void animation_timeline_init(void);

#endif
