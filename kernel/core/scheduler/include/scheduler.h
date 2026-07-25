#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "kernel/core/scheduler/include/task.h"

void scheduler_init(void);
void scheduler_tick(void);
void scheduler_on_tick(void);
void scheduler_start(void);
Task* scheduler_current_task(void);
Task* scheduler_create_kernel_task(const char* name, void (*entry)(void));
Task* scheduler_create_user_task(const char* name, void (*entry)(void));
void scheduler_add_task(Task* task);
void scheduler_terminate_task(Task* task);
uint32_t scheduler_get_task_count(void);
Task* scheduler_get_idle_task(void);
void scheduler_sleep(uint64_t ticks);
void scheduler_yield(void);
void scheduler_register_boot_task(void);
void scheduler_dump_tasks(void);
void scheduler_dump_task_info(uint64_t pid);
void scheduler_terminate_tasks_by_pid(uint32_t pid);

// Current Task Tracking
extern Task* current_task;

#endif
