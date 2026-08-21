#ifndef CPU_STATE_H
#define CPU_STATE_H

#include "../scheduler/include/task.h"
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define CPU_EXT_MAGIC 0x41544558 /* "ATEX" (ATOMS Extended State) */

typedef enum {
    CPU_EXT_BACKEND_NONE = 0,
    CPU_EXT_BACKEND_FXSAVE = 1,
    CPU_EXT_BACKEND_XSAVE = 2
} CpuExtBackend;

typedef struct TaskExtendedContext {
    uint32_t magic;          /* CPU_EXT_MAGIC */
    CpuExtBackend backend;   /* FXSAVE or XSAVE */
    uint32_t size;           /* Allocated buffer size */
    uint32_t alignment;      /* Guaranteed alignment (64 bytes) */
    bool is_initialized;     /* True if state buffer holds valid architectural state */
    uint8_t *buffer;         /* 64-byte aligned state memory */
} TaskExtendedContext;

/* Engine Lifecycle APIs */
void cpu_extended_state_engine_init(void);
CpuExtBackend cpu_extended_state_get_backend(void);
uint32_t cpu_extended_state_get_size(void);
uint32_t cpu_extended_state_get_alignment(void);

/* Task Extended State Lifecycle APIs */
bool cpu_extended_state_init_task(Task *task);
void cpu_extended_state_free_task(Task *task);

/* Context Switch & Execution APIs */
void cpu_extended_state_save(Task *task);
void cpu_extended_state_restore(Task *task);
void cpu_extended_state_context_switch(Task *old_task, Task *new_task);

#endif // CPU_STATE_H
