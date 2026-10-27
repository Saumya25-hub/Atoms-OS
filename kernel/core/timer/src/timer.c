#include "kernel/core/timer/include/timer.h"
#include "../../../debug/phase7_reliability.h"
#include "drivers/timer/pit/pit.h"
#include "kernel/core/interrupt/include/irq.h"
#include "kernel/core/scheduler/include/context.h"
#include "kernel/core/scheduler/include/scheduler.h"
#include "kernel/drivers/display/display.h"
#include "arch/x86_64/smp/smp.h"
#include "kernel/debug/lan_debug/lan_debug.h"
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

  extern void BCM_NotifyTimerTick(uint64_t);
  BCM_NotifyTimerTick(system_ticks);

  extern void BRE_Signal(uint32_t);
  // 0 = BRE_SERVICE_AUDIO, 1 = BRE_SERVICE_INPUT
  BRE_Signal(0);
  BRE_Signal(1);

  /* 1000Hz Hardware IRQ Cursor Animation Tick (AppStarting / Wait Spinner) */
  extern void bos_cursor_tick(void);
  bos_cursor_tick();

  /* Drain USB xHCI Event Ring at IRQ0 cadence to service keyboard & LEDs */
  extern void xhci_poll(void);
  xhci_poll();

  /* Hang-Proof Per-CPU Interrupted RIP Slot (CPU 0) */
  extern ATOMS_PerCPU *atoms_cpu_local(void);
  ATOMS_PerCPU *bsp_cpu = atoms_cpu_local();
  if (bsp_cpu) {
    bsp_cpu->last_interrupted_rip = regs ? regs->rip : 0;
  }

  // Context Manager saves the state
  Task *current = scheduler_current_task();
  if (current) {
    context_save_state(current, (uint64_t)regs);
  }

  scheduler_on_tick();
  atoms_p7_tick(system_ticks, scheduler_get_context_switch_count());

  /* 1 Hz Hang-Proof Diagnostic Telemetry (STEP 8: Safe, Bounded, No Allocation) */
  if ((system_ticks % 1000) == 0) {
    extern void emit_1hz_alive_telemetry(uint64_t tick);
    emit_1hz_alive_telemetry(system_ticks);
  }

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

/* --------------------------------------------------------------------------
 * STEP 8: Hang-Proof 1 Hz Telemetry Implementation
 * Emits per-CPU tick, context switches, last interrupted RIP, and lock owner
 * to serial COM1 without memory allocation or rendering.
 * -------------------------------------------------------------------------- */
void emit_1hz_alive_telemetry(uint64_t tick) {
  extern const ATOMS_CPUTopology *atoms_smp_topology(void);
  extern ATOMS_PerCPU *atoms_cpu_by_id(uint32_t id);
  extern void com1_puts(const char *s);
  extern void scheduler_get_lock_stats(uint64_t *acq, uint64_t *cont, uint64_t *spins, uint32_t *owner);

  const ATOMS_CPUTopology *topo = atoms_smp_topology();
  uint32_t count = topo ? topo->discovered_count : 1;
  if (count > ATOMS_MAX_CPUS) count = ATOMS_MAX_CPUS;

  uint64_t lacq = 0, lcont = 0, lspins = 0;
  uint32_t lock_owner = 0;
  scheduler_get_lock_stats(&lacq, &lcont, &lspins, &lock_owner);

  const char hex_chars[] = "0123456789ABCDEF";

  for (uint32_t i = 0; i < count; i++) {
    ATOMS_PerCPU *cpu = atoms_cpu_by_id(i);
    if (!cpu || cpu->state != ATOMS_CPU_ONLINE) continue;

    char buf[128];
    int pos = 0;
    const char prefix[] = "[ALIVE] CPU=";
    for (int k = 0; prefix[k]; k++) buf[pos++] = prefix[k];
    if (i >= 10) {
      buf[pos++] = (char)('0' + (i / 10));
    }
    buf[pos++] = (char)('0' + (i % 10));

    const char s_tick[] = " T=";
    for (int k = 0; s_tick[k]; k++) buf[pos++] = s_tick[k];
    char t_str[24]; int tp = 22; t_str[23] = '\0'; uint64_t tv = tick;
    if (tv == 0) t_str[tp--] = '0';
    else { while (tv > 0 && tp >= 0) { t_str[tp--] = (char)('0' + (tv % 10)); tv /= 10; } }
    for (int k = tp + 1; t_str[k]; k++) buf[pos++] = t_str[k];

    const char s_sw[] = " SW=";
    for (int k = 0; s_sw[k]; k++) buf[pos++] = s_sw[k];
    char sw_str[24]; int sp = 22; sw_str[23] = '\0'; uint64_t sv = cpu->context_switches;
    if (sv == 0) sw_str[sp--] = '0';
    else { while (sv > 0 && sp >= 0) { sw_str[sp--] = (char)('0' + (sv % 10)); sv /= 10; } }
    for (int k = sp + 1; sw_str[k]; k++) buf[pos++] = sw_str[k];

    const char s_rip[] = " RIP=0x";
    for (int k = 0; s_rip[k]; k++) buf[pos++] = s_rip[k];
    uint64_t rv = cpu->last_interrupted_rip;
    for (int s = 60; s >= 0; s -= 4) buf[pos++] = hex_chars[(rv >> s) & 0xF];

    const char s_lo[] = " LCK=";
    for (int k = 0; s_lo[k]; k++) buf[pos++] = s_lo[k];
    buf[pos++] = (char)('0' + (lock_owner % 10));

    buf[pos++] = '\r'; buf[pos++] = '\n'; buf[pos] = '\0';

    com1_puts(buf);
  }
}
