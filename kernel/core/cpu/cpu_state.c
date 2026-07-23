#include "kernel/core/cpu/cpu_state.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/lib/include/string.h"

void cpu_extended_state_init_task(Task* task) {
    if (!task) return;
    if (!task->extended_state) {
        task->extended_state = kmalloc_aligned(CPU_EXTENDED_STATE_SIZE, CPU_EXTENDED_STATE_ALIGNMENT);
    }
    if (task->extended_state) {
        memset(task->extended_state, 0, CPU_EXTENDED_STATE_SIZE);

        __asm__ volatile ("fninit");
        uint32_t mxcsr = 0x1F80;
        __asm__ volatile ("ldmxcsr %0" :: "m"(mxcsr));

        uint8_t* state_buf = (uint8_t*)task->extended_state;
        __asm__ volatile ("fxsave64 %0" : "=m"(*state_buf) :: "memory");
    }
}

void cpu_extended_state_save(Task* task) {
    if (task && task->extended_state) {
        uint8_t* state_buf = (uint8_t*)task->extended_state;
        __asm__ volatile ("fxsave64 %0" : "=m"(*state_buf) :: "memory");
    }
}

void cpu_extended_state_restore(Task* task) {
    if (task && task->extended_state) {
        uint8_t* state_buf = (uint8_t*)task->extended_state;
        __asm__ volatile ("fxrstor64 %0" :: "m"(*state_buf) : "memory");
    }
}

void cpu_extended_state_free_task(Task* task) {
    if (task && task->extended_state) {
        kfree_aligned(task->extended_state);
        task->extended_state = NULL;
    }
}
