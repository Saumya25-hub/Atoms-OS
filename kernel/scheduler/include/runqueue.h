#ifndef RUNQUEUE_H
#define RUNQUEUE_H

#include "kernel/lib/include/list.h"
#include "kernel/scheduler/include/task.h"
#include <stdint.h>
#include <stdbool.h>

#define RUNQUEUE_MAGIC 0x88AA4422

typedef struct {
    uint32_t magic;
    list_t ready_list;
} RunQueue;

void runqueue_init(RunQueue* rq);
void runqueue_push(RunQueue* rq, Task* task);
Task* runqueue_pop(RunQueue* rq);
Task* runqueue_peek(RunQueue* rq);
void runqueue_remove(RunQueue* rq, Task* task);
bool runqueue_contains(RunQueue* rq, Task* task);
bool runqueue_is_empty(RunQueue* rq);
uint32_t runqueue_get_size(RunQueue* rq);

#endif // RUNQUEUE_H
