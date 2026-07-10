#ifndef AUDIO_SERVICE_H
#define AUDIO_SERVICE_H

#include <stdbool.h>

/**
 * Audio Service Interface
 * Coordinates the Real-Time Mixer and Session routing.
 */

void audio_service_init(void);
bool audio_service_start(void);
void audio_service_stop(void);

#endif // AUDIO_SERVICE_H
