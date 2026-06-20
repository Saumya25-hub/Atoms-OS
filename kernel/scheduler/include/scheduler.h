#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "kernel/scheduler/include/task.h"

void scheduler_init(void);
void scheduler_tick(void);
void scheduler_start(void);
Task* scheduler_current_task(void);
Task* scheduler_create_kernel_task(const char* name, void (*entry)(void));
void scheduler_add_task(Task* task);
uint32_t scheduler_get_task_count(void);

#endif
