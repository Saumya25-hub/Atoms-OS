#ifndef KERNEL_DEBUG_MOUSE_TELEMETRY_H
#define KERNEL_DEBUG_MOUSE_TELEMETRY_H

#include <stdint.h>
#include <stdbool.h>

#define MOUSE_STAGE_RAW_HW       1
#define MOUSE_STAGE_HIDA_PUSH    2
#define MOUSE_STAGE_POINTER      3
#define MOUSE_STAGE_DISPATCH     4
#define MOUSE_STAGE_BWE_QUEUE    5
#define MOUSE_STAGE_SYS22_EXIT   6
#define MOUSE_STAGE_RING3_RCVD   7

void mouse_telemetry_log(uint32_t stage, int32_t x, int32_t y, uint32_t buttons, const char* tag);
uint64_t mouse_telemetry_next_seq(void);
uint64_t mouse_telemetry_current_seq(void);

#endif
