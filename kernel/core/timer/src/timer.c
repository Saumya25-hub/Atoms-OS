#include "kernel/core/timer/include/timer.h"
#include "../../../debug/phase7_reliability.h"
#include "drivers/timer/pit/pit.h"
#include "kernel/core/interrupt/include/irq.h"
#include "kernel/core/scheduler/include/context.h"
#include "kernel/core/scheduler/include/scheduler.h"
#include "kernel/drivers/display/display.h"
#include <stddef.h>

volatile uint64_t g_irq0_ticks = 0;
volatile uint64_t g_scheduler_ticks = 0;
volatile uint64_t g_heartbeat_ticks = 0;
volatile uint64_t g_xhci_events = 0;
volatile uint64_t g_xhci_transfers = 0;
volatile uint64_t g_usb_hid_packets = 0;
volatile uint64_t g_hida_events = 0;
volatile uint64_t g_mouse_events = 0;
volatile uint64_t g_keyboard_events = 0;

static uint64_t system_ticks = 0;
static uint32_t current_frequency = 0;
static TimerDriver *active_driver = NULL;

static uint64_t timer_tick_handler(registers_t *regs) {
  uint64_t start = 0;
  __asm__ volatile("rdtsc"
                   : "=a"(*(uint32_t *)&start),
                     "=d"(*((uint32_t *)&start + 1)));
  system_ticks++;
  g_irq0_ticks++;

  extern void BRE_Signal(uint32_t);
  // 0 = BRE_SERVICE_AUDIO
  BRE_Signal(0);

  /* 1000Hz Hardware IRQ Cursor Animation Tick (AppStarting / Wait Spinner) */
  extern void bos_cursor_tick(void);
  bos_cursor_tick();

  // Context Manager saves the state
  Task *current = scheduler_current_task();
  if (current) {
    context_save_state(current, (uint64_t)regs);
  }

  scheduler_on_tick();
  atoms_p7_tick(system_ticks, scheduler_get_context_switch_count());

  // After tick, process any pending BOS Reflex Engine bounded work
  extern void BRE_DispatchPending(void);
  BRE_DispatchPending();

  // For Sprint 1: Scheduler chooses SAME task
  current = scheduler_current_task();
  uint64_t result = current ? context_restore_state(current) : 0;
  uint64_t end = 0;
  __asm__ volatile("rdtsc"
                   : "=a"(*(uint32_t *)&end), "=d"(*((uint32_t *)&end + 1)));
  atoms_p7_note_irq(end - start, regs ? regs->rip : 0);
  return result;
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

uint64_t timer_get_ticks(void) { return system_ticks; }
