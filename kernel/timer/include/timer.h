#pragma once
#include <stdint.h>

// Interface for hardware timer backends
typedef struct {
    void (*init)(uint32_t frequency);
    void (*set_frequency)(uint32_t frequency);
} TimerBackend;

// Register the hardware timer backend
void timer_set_backend(TimerBackend* backend);

// Initialize the timer subsystem and request IRQ0
void timer_init(uint32_t frequency);

// Get the current system ticks since boot
uint64_t timer_get_ticks(void);
