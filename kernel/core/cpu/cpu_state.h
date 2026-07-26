#ifndef CPU_STATE_H
#define CPU_STATE_H

#include "../scheduler/include/task.h"
#include <stdbool.h>
#include <stdint.h>

#define CPU_EXTENDED_STATE_SIZE 512
#define CPU_EXTENDED_STATE_ALIGNMENT 16

void cpu_extended_state_init_task(Task *task);
void cpu_extended_state_save(Task *task);
void cpu_extended_state_restore(Task *task);
void cpu_extended_state_free_task(Task *task);

#endif // CPU_STATE_H
