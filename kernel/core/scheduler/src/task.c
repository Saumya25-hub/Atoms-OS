#include "kernel/core/scheduler/include/task.h"
#include "kernel/core/scheduler/include/scheduler.h"
#include "kernel/drivers/display/display.h"

static uint64_t next_task_id = 1;

uint64_t task_generate_id(void) { return next_task_id++; }

bool task_transition_is_valid(TaskState current_state,
                              TaskState requested_state) {
  if (current_state == TASK_INVALID || requested_state == TASK_INVALID ||
      current_state == requested_state)
    return false;

  switch (current_state) {
  case TASK_NEW:
    return requested_state == TASK_READY || requested_state == TASK_TERMINATED;
  case TASK_READY:
    return requested_state == TASK_RUNNING || requested_state == TASK_BLOCKED ||
           requested_state == TASK_WAITING ||
           requested_state == TASK_TERMINATED;
  case TASK_RUNNING:
    return requested_state == TASK_READY || requested_state == TASK_WAITING ||
           requested_state == TASK_SLEEPING ||
           requested_state == TASK_BLOCKED ||
           requested_state == TASK_TERMINATED;
  case TASK_WAITING:
  case TASK_SLEEPING:
  case TASK_BLOCKED:
    return requested_state == TASK_READY || requested_state == TASK_TERMINATED;
  case TASK_TERMINATED:
  case TASK_INVALID:
  default:
    return false;
  }
}

bool task_transition(Task *task, TaskState requested_state) {
  if (!task)
    return false;

  if (task == scheduler_get_idle_task() &&
      (requested_state == TASK_BLOCKED || requested_state == TASK_WAITING ||
       requested_state == TASK_SLEEPING ||
       requested_state == TASK_TERMINATED)) {
    display_print("\n[TASK] PANIC: attempted to block or terminate idle\n");
    __asm__ volatile("cli");
    while (1)
      __asm__ volatile("hlt");
  }

  if (!task_transition_is_valid(task->state, requested_state)) {
    display_print("\n[TASK] WARNING: invalid transition from ");
    display_print_dec(task->state);
    display_print(" to ");
    display_print_dec(requested_state);
    display_print(" task ");
    display_print_dec(task->id);
    display_print("\n");
    return false;
  }

  task->state = requested_state;
  ++task->state_transitions;
  return true;
}

TaskState task_get_state(const Task *task) {
  return task ? task->state : TASK_INVALID;
}

const char *task_state_name(TaskState state) {
  switch (state) {
  case TASK_NEW:
    return "NEW";
  case TASK_READY:
    return "READY";
  case TASK_RUNNING:
    return "RUNNING";
  case TASK_WAITING:
    return "WAITING";
  case TASK_SLEEPING:
    return "SLEEPING";
  case TASK_BLOCKED:
    return "BLOCKED";
  case TASK_TERMINATED:
    return "TERMINATED";
  case TASK_INVALID:
  default:
    return "INVALID";
  }
}
