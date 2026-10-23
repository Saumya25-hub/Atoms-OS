#include "phase7_reliability.h"
#include "../../arch/x86_64/smp/smp.h"
#include "../core/lib/include/crash_log.h"
#include "../core/memory/heap/include/heap.h"
#include "../core/scheduler/include/scheduler.h"
#include "../core/syscall/include/syscall.h"


extern void display_print(const char *str);
extern void display_print_dec(uint64_t value);
extern void display_print_hex(uint64_t value);
extern uint64_t timer_get_ticks(void);

static ATOMS_P7Config g_config;
static ATOMS_P7Diagnostics g_diag;
static ATOMS_P7CPUProfile g_cpu[ATOMS_P7_MAX_CPUS];
static ATOMS_P7SyscallProfile g_syscalls[ATOMS_P7_MAX_SYSCALLS];
static ATOMS_P7CrashEvent g_crashes[ATOMS_P7_CRASH_EVENTS];
static ATOMS_P7DriverHealth g_drivers[ATOMS_P7_MAX_DRIVERS];
static uint16_t g_lock_stack[ATOMS_P7_MAX_CPUS][ATOMS_P7_MAX_LOCK_DEPTH];
static uint32_t g_lock_depth[ATOMS_P7_MAX_CPUS];
static uint64_t g_last_watchdog_tick;
static bool g_initialized;

static void p7_zero(void *ptr, size_t size) {
  uint8_t *bytes = (uint8_t *)ptr;
  for (size_t i = 0; i < size; ++i)
    bytes[i] = 0;
}

static uint32_t p7_cpu(void) {
  uint32_t id = atoms_cpu_id();
  return id < ATOMS_P7_MAX_CPUS ? id : 0;
}

static uint32_t p7_bucket(uint64_t value) {
  uint32_t bucket = 0;
  while (value > 1 && bucket + 1 < ATOMS_P7_LATENCY_BUCKETS) {
    value >>= 1;
    ++bucket;
  }
  return bucket;
}

static void p7_copy_name(char *out, const char *in) {
  uint32_t i = 0;
  if (!in) {
    out[0] = 0;
    return;
  }
  while (i + 1 < ATOMS_P7_NAME_MAX && in[i]) {
    out[i] = in[i];
    ++i;
  }
  out[i] = 0;
}

void atoms_p7_init(const ATOMS_P7Config *config) {
  p7_zero(&g_diag, sizeof(g_diag));
  p7_zero(g_cpu, sizeof(g_cpu));
  p7_zero(g_syscalls, sizeof(g_syscalls));
  p7_zero(g_crashes, sizeof(g_crashes));
  p7_zero(g_drivers, sizeof(g_drivers));
  p7_zero(g_lock_stack, sizeof(g_lock_stack));
  p7_zero(g_lock_depth, sizeof(g_lock_depth));
  g_config.runtime_flags = ATOMS_P7_RUNTIME_DEFAULT;
  g_config.watchdog_period_ticks = 1000;
  g_config.watchdog_stall_ticks = 5000;
  g_config.watchdog_report_limit = 3;
  g_config.lock_spin_limit = 1000000;
  g_config.stress_iteration_limit = ATOMS_P7_STRESS_MAX_ITERATIONS;
  if (config)
    g_config = *config;
  if (!g_config.watchdog_period_ticks)
    g_config.watchdog_period_ticks = 1000;
  if (!g_config.watchdog_stall_ticks)
    g_config.watchdog_stall_ticks = 5000;
  if (g_config.watchdog_report_limit > 16)
    g_config.watchdog_report_limit = 16;
  if (!g_config.lock_spin_limit)
    g_config.lock_spin_limit = 1000000;
  if (!g_config.stress_iteration_limit ||
      g_config.stress_iteration_limit > ATOMS_P7_STRESS_MAX_ITERATIONS)
    g_config.stress_iteration_limit = ATOMS_P7_STRESS_MAX_ITERATIONS;
  g_cpu[0].online = true;
  g_initialized = true;
  g_diag.initialized = true;
  crash_log_add("[PHASE7] Reliability layer initialized");
}

const ATOMS_P7Config *atoms_p7_config(void) { return &g_config; }

void atoms_p7_tick_cpu(uint32_t cpu_id, uint64_t tick, uint64_t scheduler_progress) {
  if (!g_initialized || cpu_id >= ATOMS_P7_MAX_CPUS)
    return;
  ATOMS_P7CPUProfile *profile = &g_cpu[cpu_id];
  profile->online = true;
  if (!profile->synthetic_stall) {
    profile->last_tick = tick;
    profile->scheduler_progress = scheduler_progress;
    ++profile->heartbeat;
  }
  if (cpu_id == 0) {
    ++profile->timer_entries;
    if (tick - g_last_watchdog_tick >= g_config.watchdog_period_ticks) {
      g_last_watchdog_tick = tick;
      atoms_p7_watchdog_check(tick);
    }
  }
}

void atoms_p7_tick(uint64_t tick, uint64_t scheduler_progress) {
  atoms_p7_tick_cpu(p7_cpu(), tick, scheduler_progress);
}

void atoms_p7_heartbeat(void) {
  if (g_initialized) {
    uint32_t c = p7_cpu();
    if (c < ATOMS_P7_MAX_CPUS && !g_cpu[c].synthetic_stall)
      ++g_cpu[c].heartbeat;
  }
}

void atoms_p7_inject_stall(uint32_t cpu_id, bool stall) {
  if (cpu_id < ATOMS_P7_MAX_CPUS) {
    g_cpu[cpu_id].synthetic_stall = stall;
    extern void com1_puts(const char *s);
    com1_puts(stall ? "[DANGER_MONITOR] Synthetic Stall Injected on CPU " : "[DANGER_MONITOR] Synthetic Stall Lifted on CPU ");
    char cid[4] = { (char)('0' + cpu_id), '\r', '\n', '\0' };
    com1_puts(cid);
  }
}

ATOMS_CPUDangerLevel atoms_p7_get_cpu_danger(uint32_t cpu_id) {
  if (cpu_id >= ATOMS_P7_MAX_CPUS)
    return CPU_DANGER_OFFLINE;
  return (ATOMS_CPUDangerLevel)g_cpu[cpu_id].danger_level;
}

const char *atoms_p7_danger_str(ATOMS_CPUDangerLevel level) {
  switch (level) {
    case CPU_DANGER_HEALTHY: return "HEALTHY";
    case CPU_DANGER_SUSPECTED_STALL: return "SUSPECTED_STALL";
    case CPU_DANGER_CONFIRMED_STALL: return "CONFIRMED_STALL";
    case CPU_DANGER_INITIALIZING: return "INITIALIZING";
    case CPU_DANGER_SAFE_HALT: return "SAFE_HALT";
    case CPU_DANGER_PANIC: return "PANIC";
    default: return "OFFLINE";
  }
}

void atoms_p7_note_scheduler(uint64_t cycles, uint64_t sampled_rip) {
  ATOMS_P7CPUProfile *p = &g_cpu[p7_cpu()];
  ++p->samples;
  p->sampled_rip = sampled_rip;
  ++p->context_switches;
  ++p->scheduler_progress;
  ++p->scheduler_latency[p7_bucket(cycles)];
}

void atoms_p7_note_irq(uint64_t cycles, uint64_t sampled_rip) {
  ATOMS_P7CPUProfile *p = &g_cpu[p7_cpu()];
  ++p->irq_entries;
  p->sampled_rip = sampled_rip;
  ++p->irq_latency[p7_bucket(cycles)];
}

void atoms_p7_note_syscall(uint32_t id, uint64_t cycles, bool error,
                           uint64_t sampled_rip) {
  ATOMS_P7CPUProfile *p = &g_cpu[p7_cpu()];
  if (id >= ATOMS_P7_MAX_SYSCALLS)
    return;
  ATOMS_P7SyscallProfile *s = &g_syscalls[id];
  ++p->syscall_entries;
  ++s->calls;
  s->total_cycles += cycles;
  if (error) {
    ++p->syscall_errors;
    ++s->errors;
  }
  if (cycles > s->max_cycles)
    s->max_cycles = cycles;
  ++s->latency[p7_bucket(cycles)];
  p->sampled_rip = sampled_rip;
}

void atoms_p7_watchdog_check(uint64_t tick) {
  if (!g_initialized || !(g_config.runtime_flags & ATOMS_P7_RUNTIME_WATCHDOG))
    return;
  ++g_diag.watchdog_checks;
  uint32_t warn_limit = 3000;  // 3.0 seconds Warning (SUSPECTED_STALL)
  uint32_t panic_limit = 6000; // 6.0 seconds Hard Fault (CONFIRMED_STALL)

  for (uint32_t i = 0; i < ATOMS_P7_MAX_CPUS; ++i) {
    ATOMS_P7CPUProfile *p = &g_cpu[i];
    ATOMS_PerCPU *hw = atoms_cpu_by_id(i);

    if (!hw || hw->state != ATOMS_CPU_ONLINE) {
      p->danger_level = (hw && hw->state == ATOMS_CPU_STARTING) ? CPU_DANGER_INITIALIZING : CPU_DANGER_OFFLINE;
      p->stalled = false;
      continue;
    }

    p->online = true;
    if (p->last_tick == 0) {
      p->danger_level = CPU_DANGER_INITIALIZING;
      continue;
    }

    uint64_t delta = (tick >= p->last_tick) ? (tick - p->last_tick) : 0;
    if (delta > panic_limit) {
      p->danger_level = CPU_DANGER_CONFIRMED_STALL;
      p->stalled = true;
      ++p->consecutive_stalls;
      ++g_diag.watchdog_stalls;
      if (p->consecutive_stalls <= g_config.watchdog_report_limit) {
        atoms_p7_capture_event(ATOMS_P7_EVENT_WATCHDOG_STALL,
                               (uint32_t)delta, p->sampled_rip, 0, 0, 0,
                               0, "BOS DANGER: CPU CONFIRMED STALL");
        extern void com1_puts(const char *s);
        com1_puts("[DANGER ALERT] CPU ");
        char cid[4] = { (char)('0' + i), '\0' };
        com1_puts(cid);
        com1_puts(" CONFIRMED STALL! Delta > 6000ms\r\n");
      }
    } else if (delta > warn_limit) {
      p->danger_level = CPU_DANGER_SUSPECTED_STALL;
      p->stalled = true;
    } else {
      if (p->danger_level == CPU_DANGER_CONFIRMED_STALL || p->danger_level == CPU_DANGER_SUSPECTED_STALL) {
        extern void com1_puts(const char *s);
        com1_puts("[DANGER MONITOR] CPU ");
        char cid[4] = { (char)('0' + i), '\0' };
        com1_puts(cid);
        com1_puts(" Recovered -> HEALTHY\r\n");
      }
      p->danger_level = CPU_DANGER_HEALTHY;
      p->stalled = false;
      p->consecutive_stalls = 0;
    }
  }
}

void atoms_p7_capture_event(ATOMS_P7EventType type, uint32_t code, uint64_t rip,
                            uint64_t rsp, const char *file, uint32_t line,
                            const char *function, const char *reason) {
  uint32_t seq = __atomic_fetch_add(&g_diag.crash_head, 1, __ATOMIC_SEQ_CST);
  uint32_t index = seq % ATOMS_P7_CRASH_EVENTS;
  ATOMS_P7CrashEvent *event = &g_crashes[index];
  event->sequence = seq + 1;
  event->tick = timer_get_ticks();
  event->rip = rip;
  event->rsp = rsp;
  Task *cur_task = scheduler_current_task();
  event->task_id = cur_task ? cur_task->id : 0;
  event->process_id =
      cur_task ? cur_task->owner_pid : 0;
  event->cpu_id = p7_cpu();
  event->code = code;
  event->type = (uint16_t)type;
  event->line = line;
  event->file = file;
  event->function = function;
  event->reason = reason;
  if (g_diag.crash_count < ATOMS_P7_CRASH_EVENTS)
    __atomic_fetch_add(&g_diag.crash_count, 1, __ATOMIC_SEQ_CST);
  if (type == ATOMS_P7_EVENT_ASSERT)
    __atomic_fetch_add(&g_diag.asserts, 1, __ATOMIC_SEQ_CST);
  if (type == ATOMS_P7_EVENT_PANIC)
    __atomic_fetch_add(&g_diag.panics, 1, __ATOMIC_SEQ_CST);
  if (type == ATOMS_P7_EVENT_INVARIANT)
    __atomic_fetch_add(&g_diag.invariant_failures, 1, __ATOMIC_SEQ_CST);
}

void atoms_p7_capture_assert(const char *file, uint32_t line,
                             const char *function) {
  atoms_p7_capture_event(ATOMS_P7_EVENT_ASSERT, 0, 0, 0, file, line, function,
                         "assertion failed");
}

void atoms_p7_capture_panic(uint32_t code, uint64_t rip, uint64_t rsp,
                            const char *reason) {
  atoms_p7_capture_event(ATOMS_P7_EVENT_PANIC, code, rip, rsp, 0, 0, 0, reason);
}

void atoms_p7_dump_snapshot(void) {
  ++g_diag.snapshots;
  display_print("[PHASE7] SNAPSHOT events=");
  display_print_dec(g_diag.crash_count);
  display_print(" watchdog_stalls=");
  display_print_dec(g_diag.watchdog_stalls);
  display_print(" locks=");
  display_print_dec(g_diag.lock_contentions);
  display_print(" drivers=");
  display_print_dec(g_diag.driver_count);
  display_print("\n");
  crash_log_dump();
}

bool atoms_p7_lock_can_acquire(uint16_t rank) {
  uint32_t cpu = p7_cpu();
  if (g_lock_depth[cpu] && rank < g_lock_stack[cpu][g_lock_depth[cpu] - 1]) {
    ++g_diag.lock_order_violations;
    atoms_p7_capture_event(ATOMS_P7_EVENT_LOCK_ORDER, rank, 0, 0, 0, 0, 0,
                           "lock rank inversion");
    return false;
  }
  return true;
}

void atoms_p7_lock_acquired(uint16_t rank, bool contended, uint64_t spins) {
  if (contended)
    __atomic_fetch_add(&g_diag.lock_contentions, 1, __ATOMIC_RELAXED);
  (void)spins;


  uint32_t cpu = p7_cpu();
  if (g_lock_depth[cpu] < ATOMS_P7_MAX_LOCK_DEPTH)
    g_lock_stack[cpu][g_lock_depth[cpu]++] = rank;
}
void atoms_p7_lock_released(uint16_t rank) {
  (void)rank;
  uint32_t cpu = p7_cpu();
  if (g_lock_depth[cpu])
    --g_lock_depth[cpu];
}
void atoms_p7_lock_recursion(uint16_t rank) {
  ++g_diag.lock_recursions;
  atoms_p7_capture_event(ATOMS_P7_EVENT_LOCK_RECURSION, rank, 0, 0, 0, 0, 0,
                         "recursive lock");
}
void atoms_p7_lock_timeout(uint16_t rank, uint64_t spins) {
  __atomic_fetch_add(&g_diag.lock_timeouts, 1, __ATOMIC_RELAXED);
  atoms_p7_capture_event(ATOMS_P7_EVENT_LOCK_TIMEOUT, rank, spins, 0, 0, 0, 0,
                         "lock spin bound exceeded");
}

int atoms_p7_driver_register(uint32_t id, const char *name, bool required) {
  for (uint32_t i = 0; i < ATOMS_P7_MAX_DRIVERS; ++i)
    if (g_drivers[i].occupied && g_drivers[i].id == id)
      return -1;
  for (uint32_t i = 0; i < ATOMS_P7_MAX_DRIVERS; ++i)
    if (!g_drivers[i].occupied) {
      ATOMS_P7DriverHealth *d = &g_drivers[i];
      d->occupied = true;
      d->id = id;
      d->required = required;
      d->state = ATOMS_P7_DRIVER_INITIALIZING;
      p7_copy_name(d->name, name);
      ++g_diag.driver_count;
      ++g_diag.driver_registrations;
      return 0;
    }
  return -1;
}
void atoms_p7_driver_state(uint32_t id, ATOMS_P7DriverState state) {
  for (uint32_t i = 0; i < ATOMS_P7_MAX_DRIVERS; ++i)
    if (g_drivers[i].occupied && g_drivers[i].id == id) {
      g_drivers[i].state = state;
      if (state == ATOMS_P7_DRIVER_FAILED)
        ++g_diag.driver_failures;
      return;
    }
}
void atoms_p7_driver_heartbeat(uint32_t id, uint64_t tick) {
  for (uint32_t i = 0; i < ATOMS_P7_MAX_DRIVERS; ++i)
    if (g_drivers[i].occupied && g_drivers[i].id == id) {
      g_drivers[i].heartbeat++;
      g_drivers[i].last_tick = tick;
      return;
    }
}
void atoms_p7_driver_event(uint32_t id, bool error) {
  for (uint32_t i = 0; i < ATOMS_P7_MAX_DRIVERS; ++i)
    if (g_drivers[i].occupied && g_drivers[i].id == id) {
      ++g_drivers[i].events;
      if (error)
        ++g_drivers[i].errors;
      return;
    }
}

bool atoms_p7_check_heap(void) {
  ++g_diag.heap_checks;
  heap_validate();
  return true;
}
bool atoms_p7_check_current_stack(void) {
  ++g_diag.stack_checks;
  return scheduler_current_task() != 0;
}

bool atoms_p7_run_bounded_self_test(uint32_t iterations,
                                    ATOMS_P7StressReport *report) {
  if (!report)
    return false;
  p7_zero(report, sizeof(*report));
  report->status = ATOMS_P7_STRESS_NOT_VERIFIED;
  if (iterations == 0)
    iterations = 1;
  if (iterations > g_config.stress_iteration_limit)
    iterations = g_config.stress_iteration_limit;
  report->requested_iterations = iterations;
  for (uint32_t i = 0; i < iterations; ++i) {
    report->deterministic_digest = report->deterministic_digest * 33U + i + 1U;
    ++report->process_checks;
    ++report->thread_checks;
    ++report->scheduler_checks;
    ++report->synchronization_checks;
    ++report->syscall_validation_checks;
    if (!scheduler_validate_consistency() || !syscall_phase5_self_test()) {
      ++report->failures;
      break;
    }
    ++report->completed_iterations;
  }
  if (report->failures)
    report->status = ATOMS_P7_STRESS_FAIL;
  else if (report->completed_iterations == report->requested_iterations)
    report->status = ATOMS_P7_STRESS_PASS;
  ++g_diag.stress_runs;
  if (report->status == ATOMS_P7_STRESS_FAIL)
    ++g_diag.stress_failures;
  return report->status == ATOMS_P7_STRESS_PASS;
}

void atoms_p7_get_diagnostics(ATOMS_P7Diagnostics *out) {
  if (out)
    *out = g_diag;
}
void atoms_p7_get_cpu_profile(uint32_t cpu_id, ATOMS_P7CPUProfile *out) {
  if (out && cpu_id < ATOMS_P7_MAX_CPUS)
    *out = g_cpu[cpu_id];
}
void atoms_p7_get_syscall_profile(uint32_t id, ATOMS_P7SyscallProfile *out) {
  if (out && id < ATOMS_P7_MAX_SYSCALLS)
    *out = g_syscalls[id];
}
