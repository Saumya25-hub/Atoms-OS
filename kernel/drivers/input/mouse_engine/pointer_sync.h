#ifndef POINTER_SYNC_H
#define POINTER_SYNC_H

#include <stdint.h>
#include <stdbool.h>

void pointer_sync_init(void);
bool pointer_sync_validate_packet(int32_t dx, int32_t dy, bool overflow_x, bool overflow_y);

#endif
