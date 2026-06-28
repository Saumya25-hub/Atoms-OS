#ifndef TASK_H
#define TASK_H

#include <stdint.h>
#include "kernel/lib/include/list.h"

#define KERNEL_TASK_STACK_SIZE (16 * 1024)

typedef enum
{
    TASK_READY = 0,
    TASK_RUNNING,
    TASK_BLOCKED,
    TASK_SLEEPING,
    TASK_TERMINATED
} TaskState;

typedef struct Task {
    uint64_t id;
    const char* name;
    volatile TaskState state;
    void* stack;
    uint64_t rsp;
    uint64_t rip;
    int32_t quantum;
    int32_t default_quantum;
    uint64_t wake_tick;
    uint8_t is_user_task;
    void* user_stack; // Base of the user stack
    void* pml4;       // Task's address space
    uint64_t last_run_tick; // For scheduler ping-pong prevention
    list_node_t queue_node;
} Task;

#include <stdbool.h>

bool task_transition(Task* task, TaskState requested_state);
TaskState task_get_state(const Task* task);

#endif
