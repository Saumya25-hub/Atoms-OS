#ifndef TASK_H
#define TASK_H

#include <stdint.h>

#define KERNEL_TASK_STACK_SIZE (4 * 1024)

typedef enum {
    TASK_READY,
    TASK_RUNNING,
    TASK_BLOCKED,
    TASK_SLEEPING,
    TASK_TERMINATED
} TaskState;

typedef struct Task {
    uint64_t id;
    TaskState state;
    void* stack;
    uint64_t rsp;
    uint64_t rip;
    struct Task* next;
} Task;

#endif
