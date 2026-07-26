#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "task.h"
#include <stdbool.h>
#include <stdint.h>

#define SCHEDULER_PRIORITY_LEVELS 32U
#define SCHEDULER_PRIORITY_MIN 0U
#define SCHEDULER_PRIORITY_MAX (SCHEDULER_PRIORITY_LEVELS - 1U)
#define SCHEDULER_AGING_INTERVAL_TICKS 250U

typedef enum {
  SCHEDULER_POLICY_ROUND_ROBIN = 0,
  SCHEDULER_POLICY_PRIORITY_AGING = 1
} SchedulerPolicy;

typedef enum {
  SCHEDULER_SWITCH_NONE = 0,
  SCHEDULER_SWITCH_QUANTUM,
  SCHEDULER_SWITCH_YIELD,
  SCHEDULER_SWITCH_BLOCK,
  SCHEDULER_SWITCH_SLEEP,
  SCHEDULER_SWITCH_WAIT,
  SCHEDULER_SWITCH_EXIT,
  SCHEDULER_SWITCH_WAKE_PREEMPT
} SchedulerSwitchReason;

typedef struct {
  uint64_t tick_count;
  uint64_t context_switches;
  uint64_t voluntary_switches;
  uint64_t involuntary_switches;
  uint64_t idle_ticks;
  uint64_t busy_ticks;
  uint64_t total_wait_ticks;
  uint64_t dispatched_tasks;
  uint64_t wakeups;
  uint64_t starvation_boosts;
  uint64_t illegal_transitions;
  uint64_t duplicate_rejections;
  uint64_t queue_corruptions;
  uint64_t dead_task_detections;
  uint64_t longest_runtime_ticks;
  uint64_t longest_runtime_task_id;
  uint32_t ready_count;
  uint32_t waiting_count;
  uint32_t sleeping_count;
  uint32_t blocked_count;
  uint32_t terminated_count;
  uint32_t cpu_utilization_x100;
  uint32_t average_wait_ticks;
  uint32_t current_quantum;
  uint32_t current_quantum_used;
  uint64_t current_task_id;
  uint32_t current_process_id;
  bool running;
  bool queues_consistent;
  SchedulerPolicy policy;
} SchedulerDiagnostics;

void scheduler_init(void);
void scheduler_tick(void);
void scheduler_on_tick(void);
void scheduler_start(void);
Task *scheduler_current_task(void);
Task *scheduler_create_kernel_task(const char *name, void (*entry)(void));
Task *scheduler_create_user_task(const char *name, void (*entry)(void));
void scheduler_add_task(Task *task);
bool scheduler_submit_task(Task *task);
bool scheduler_wait_task(Task *task);
bool scheduler_block_task(Task *task);
bool scheduler_resume_task(Task *task);
bool scheduler_suspend_task(Task *task);
void scheduler_terminate_task(Task *task);
uint32_t scheduler_get_task_count(void);
uint64_t scheduler_get_tick_count(void);
uint64_t scheduler_get_context_switch_count(void);
bool scheduler_set_task_priority(Task *task, uint8_t priority);
uint8_t scheduler_get_task_priority(const Task *task);
bool scheduler_set_policy(SchedulerPolicy policy);
SchedulerPolicy scheduler_get_policy(void);
void scheduler_wake_task(Task *task);
bool scheduler_set_task_affinity(Task *task, uint64_t affinity_mask);
bool scheduler_request_migration(Task *task, uint32_t target_cpu);
Task *scheduler_get_idle_task(void);
Task *scheduler_create_idle_task_cpu(uint32_t cpu_id);
void scheduler_sleep(uint64_t ticks);
void scheduler_yield(void);
void scheduler_register_boot_task(void);
void scheduler_dump_tasks(void);
void scheduler_dump_task_info(uint64_t pid);
void scheduler_terminate_tasks_by_pid(uint32_t pid);
void scheduler_dump_runtime_diagnostics(void);
void scheduler_dump_queues(void);
void scheduler_get_diagnostics(SchedulerDiagnostics *out);
bool scheduler_validate_consistency(void);
bool scheduler_is_running(void);

// Current Task Tracking
extern Task *current_task;

#endif
