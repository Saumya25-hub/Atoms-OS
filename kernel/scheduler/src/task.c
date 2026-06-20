#include "kernel/scheduler/include/task.h"

// Task management utility functions (to be expanded)
static uint64_t next_task_id = 1;

uint64_t task_generate_id(void) {
    return next_task_id++;
}
