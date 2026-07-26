#ifndef ATOMS_PHASE7_RELIABILITY_H
#define ATOMS_PHASE7_RELIABILITY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define ATOMS_P7_MAX_CPUS 8U
#define ATOMS_P7_MAX_SYSCALLS 43U
#define ATOMS_P7_LATENCY_BUCKETS 8U
#define ATOMS_P7_MAX_DRIVERS 24U
#define ATOMS_P7_MAX_LOCK_DEPTH 8U
#define ATOMS_P7_CRASH_EVENTS 32U
#define ATOMS_P7_STRESS_MAX_ITERATIONS 256U
#define ATOMS_P7_NAME_MAX 24U

#define ATOMS_P7_RUNTIME_WATCHDOG (1U << 0)
#define ATOMS_P7_RUNTIME_SELF_TEST (1U << 1)
#define ATOMS_P7_RUNTIME_VERBOSE (1U << 2)
#define ATOMS_P7_RUNTIME_DEFAULT ATOMS_P7_RUNTIME_WATCHDOG

typedef enum {
  ATOMS_P7_EVENT_BOOT = 0,
  ATOMS_P7_EVENT_ASSERT,
  ATOMS_P7_EVENT_PANIC,
  ATOMS_P7_EVENT_WATCHDOG_STALL,
  ATOMS_P7_EVENT_LOCK_RECURSION,
  ATOMS_P7_EVENT_LOCK_ORDER,
  ATOMS_P7_EVENT_LOCK_TIMEOUT,
  ATOMS_P7_EVENT_DRIVER_STATE,
  ATOMS_P7_EVENT_INVARIANT
} ATOMS_P7EventType;

typedef enum {
  ATOMS_P7_DRIVER_UNKNOWN = 0,
  ATOMS_P7_DRIVER_INITIALIZING,
  ATOMS_P7_DRIVER_HEALTHY,
  ATOMS_P7_DRIVER_DEGRADED,
  ATOMS_P7_DRIVER_FAILED,
  ATOMS_P7_DRIVER_OFFLINE
} ATOMS_P7DriverState;

typedef enum {
  ATOMS_P7_STRESS_NOT_RUN = 0,
  ATOMS_P7_STRESS_PASS,
  ATOMS_P7_STRESS_FAIL,
  ATOMS_P7_STRESS_NOT_VERIFIED
} ATOMS_P7StressStatus;

typedef struct {
  uint64_t sequence;
  uint64_t tick;
  uint64_t rip;
  uint64_t rsp;
  uint64_t cr3;
  uint64_t task_id;
  uint32_t cpu_id;
  uint32_t process_id;
  uint32_t code;
  uint16_t type;
  uint16_t line;
  const char *file;
  const char *function;
  const char *reason;
} ATOMS_P7CrashEvent;

typedef struct {
  uint64_t heartbeat;
  uint64_t last_tick;
  uint64_t scheduler_progress;
  uint64_t last_scheduler_progress;
  uint64_t timer_entries;
  uint64_t irq_entries;
  uint64_t syscall_entries;
  uint64_t syscall_errors;
  uint64_t context_switches;
  uint64_t samples;
  uint64_t sampled_rip;
  uint64_t scheduler_latency[ATOMS_P7_LATENCY_BUCKETS];
  uint64_t irq_latency[ATOMS_P7_LATENCY_BUCKETS];
  uint64_t syscall_latency[ATOMS_P7_LATENCY_BUCKETS];
  uint32_t consecutive_stalls;
  bool online;
  bool stalled;
} ATOMS_P7CPUProfile;

typedef struct {
  uint64_t calls;
  uint64_t errors;
  uint64_t total_cycles;
  uint64_t max_cycles;
  uint64_t latency[ATOMS_P7_LATENCY_BUCKETS];
} ATOMS_P7SyscallProfile;

typedef struct {
  char name[ATOMS_P7_NAME_MAX];
  uint32_t id;
  ATOMS_P7DriverState state;
  uint64_t heartbeat;
  uint64_t last_tick;
  uint64_t events;
  uint64_t errors;
  uint32_t stale_count;
  bool required;
  bool occupied;
} ATOMS_P7DriverHealth;

typedef struct {
  uint32_t runtime_flags;
  uint32_t watchdog_period_ticks;
  uint32_t watchdog_stall_ticks;
  uint32_t watchdog_report_limit;
  uint32_t lock_spin_limit;
  uint32_t stress_iteration_limit;
} ATOMS_P7Config;

typedef struct {
  ATOMS_P7StressStatus status;
  uint32_t requested_iterations;
  uint32_t completed_iterations;
  uint32_t process_checks;
  uint32_t thread_checks;
  uint32_t scheduler_checks;
  uint32_t synchronization_checks;
  uint32_t syscall_validation_checks;
  uint32_t failures;
  uint32_t not_verified;
  uint32_t deterministic_digest;
} ATOMS_P7StressReport;

typedef struct {
  uint64_t snapshots;
  uint64_t asserts;
  uint64_t panics;
  uint64_t invariant_failures;
  uint64_t watchdog_checks;
  uint64_t watchdog_stalls;
  uint64_t lock_contentions;
  uint64_t lock_recursions;
  uint64_t lock_order_violations;
  uint64_t lock_timeouts;
  uint64_t heap_checks;
  uint64_t heap_check_failures;
  uint64_t stack_checks;
  uint64_t stack_check_failures;
  uint64_t driver_registrations;
  uint64_t driver_failures;
  uint64_t stress_runs;
  uint64_t stress_failures;
  uint32_t crash_head;
  uint32_t crash_count;
  uint32_t driver_count;
  bool initialized;
} ATOMS_P7Diagnostics;

void atoms_p7_init(const ATOMS_P7Config *config);
const ATOMS_P7Config *atoms_p7_config(void);
void atoms_p7_tick(uint64_t tick, uint64_t scheduler_progress);
void atoms_p7_note_scheduler(uint64_t cycles, uint64_t sampled_rip);
void atoms_p7_note_irq(uint64_t cycles, uint64_t sampled_rip);
void atoms_p7_note_syscall(uint32_t id, uint64_t cycles, bool error,
                           uint64_t sampled_rip);
void atoms_p7_heartbeat(void);
void atoms_p7_watchdog_check(uint64_t tick);

void atoms_p7_capture_event(ATOMS_P7EventType type, uint32_t code, uint64_t rip,
                            uint64_t rsp, const char *file, uint32_t line,
                            const char *function, const char *reason);
void atoms_p7_capture_assert(const char *file, uint32_t line,
                             const char *function);
void atoms_p7_capture_panic(uint32_t code, uint64_t rip, uint64_t rsp,
                            const char *reason);
void atoms_p7_dump_snapshot(void);

void atoms_p7_lock_acquired(uint16_t rank, bool contended, uint64_t spins);
void atoms_p7_lock_released(uint16_t rank);
void atoms_p7_lock_recursion(uint16_t rank);
void atoms_p7_lock_timeout(uint16_t rank, uint64_t spins);
bool atoms_p7_lock_can_acquire(uint16_t rank);

int atoms_p7_driver_register(uint32_t id, const char *name, bool required);
void atoms_p7_driver_state(uint32_t id, ATOMS_P7DriverState state);
void atoms_p7_driver_heartbeat(uint32_t id, uint64_t tick);
void atoms_p7_driver_event(uint32_t id, bool error);

bool atoms_p7_check_heap(void);
bool atoms_p7_check_current_stack(void);
bool atoms_p7_run_bounded_self_test(uint32_t iterations,
                                    ATOMS_P7StressReport *report);
void atoms_p7_get_diagnostics(ATOMS_P7Diagnostics *out);
void atoms_p7_get_cpu_profile(uint32_t cpu_id, ATOMS_P7CPUProfile *out);
void atoms_p7_get_syscall_profile(uint32_t id, ATOMS_P7SyscallProfile *out);

#endif
