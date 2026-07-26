#ifndef TASK_H
#define TASK_H

#include "../../lib/include/list.h"
#include <stdint.h>

#define KERNEL_TASK_STACK_SIZE (16 * 1024)

typedef enum {
  /* Values 0-4 are ABI-stable for legacy Task users. */
  TASK_READY = 0,
  TASK_RUNNING,
  TASK_BLOCKED,
  TASK_SLEEPING,
  TASK_TERMINATED,
  TASK_WAITING,
  TASK_NEW,
  TASK_INVALID
} TaskState;

typedef enum {
  TASK_QUEUE_NONE = 0,
  TASK_QUEUE_READY,
  TASK_QUEUE_WAITING,
  TASK_QUEUE_SLEEPING,
  TASK_QUEUE_BLOCKED,
  TASK_QUEUE_TERMINATED
} TaskQueueClass;

typedef struct Task {
  uint64_t id;
  const char *name;
  volatile TaskState state;
  void *stack;
  uint64_t rsp;
  uint64_t rip;
  int32_t quantum;
  int32_t default_quantum;
  uint64_t wake_tick;
  uint8_t is_user_task;
  void *user_stack;       // Base of the user stack
  void *pml4;             // Task's address space
  uint64_t last_run_tick; // For scheduler ping-pong prevention
  uint64_t ready_since_tick;
  uint64_t total_run_ticks;
  uint64_t total_wait_ticks;
  uint64_t dispatch_count;
  uint64_t voluntary_switches;
  uint64_t involuntary_switches;
  uint64_t state_transitions;
  uint64_t affinity_mask;
  uint8_t base_priority;
  uint8_t effective_priority;
  uint8_t starvation_boosts;
  uint8_t queue_class;
  void *extended_state;      // FPU / SSE extended CPU state buffer (512 bytes,
                             // 16-byte aligned)
  uint32_t owner_pid;        // Process ID owning this task
  void *current_bgl_context; // Task-local active BGL context
  /* Phase 6 placement metadata. Task remains the sole execution authority. */
  uint32_t assigned_cpu;
  uint32_t last_cpu;
  uint64_t migration_count;
  list_node_t queue_node;
} Task;

#include <stdbool.h>

bool task_transition(Task *task, TaskState requested_state);
bool task_transition_is_valid(TaskState current_state,
                              TaskState requested_state);
TaskState task_get_state(const Task *task);
const char *task_state_name(TaskState state);

#endif
