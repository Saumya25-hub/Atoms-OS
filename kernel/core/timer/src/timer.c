#include "kernel/core/timer/include/timer.h"
#include "kernel/core/interrupt/include/irq.h"
#include "kernel/drivers/display/display.h"
#include "kernel/core/scheduler/include/scheduler.h"
#include "drivers/timer/pit/pit.h"
#include "kernel/core/scheduler/include/context.h"
#include <stddef.h>

static uint64_t system_ticks = 0;
static uint32_t current_frequency = 0;
static TimerDriver* active_driver = NULL;

static uint64_t timer_tick_handler(registers_t* regs) {
    system_ticks++;
    
    // Context Manager saves the state
    Task* current = scheduler_current_task();
    if (current) {
        context_save_state(current, (uint64_t)regs);
    }
    
    scheduler_on_tick();

    
    // For Sprint 1: Scheduler chooses SAME task
    current = scheduler_current_task();
    if (current) {
        return context_restore_state(current);
    }
    
    return 0;
}

void timer_init(uint32_t frequency) {
    current_frequency = frequency;
    
    // Fallback to PIT Driver internally (Isolates Kernel)
    active_driver = &pit_timer_driver;
    
    if (active_driver && active_driver->init) {
        active_driver->init(frequency);
    }
    
    // Register the timer tick handler to IRQ 0 (Timer)
    irq_register_handler(0, timer_tick_handler);
}

uint64_t timer_get_ticks(void) {
    return system_ticks;
}
