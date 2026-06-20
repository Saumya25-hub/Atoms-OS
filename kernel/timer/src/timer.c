#include "kernel/timer/include/timer.h"
#include "kernel/interrupt/include/irq.h"
#include "kernel/display/display.h"
#include <stddef.h>

static uint64_t system_ticks = 0;
static uint32_t current_frequency = 0;
static TimerBackend* active_backend = NULL;

static void timer_tick_handler(registers_t* regs) {
    (void)regs;
    system_ticks++;
    
    // Optional: Print a dot every 100 ticks to verify it's working visually
    // if (system_ticks % 100 == 0) {
    //     display_print(".");
    // }
}

void timer_set_backend(TimerBackend* backend) {
    active_backend = backend;
}

void timer_init(uint32_t frequency) {
    current_frequency = frequency;
    
    if (active_backend && active_backend->init) {
        active_backend->init(frequency);
    }
    
    // Register the timer tick handler to IRQ 0 (Timer)
    irq_register_handler(0, timer_tick_handler);
}

uint64_t timer_get_ticks(void) {
    return system_ticks;
}
