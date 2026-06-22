#include "kernel/syscall/include/syscall.h"
#include "kernel/interrupt/include/isr.h"
#include "kernel/scheduler/include/scheduler.h"
#include "kernel/timer/include/timer.h"
#include "kernel/display/display.h"
#include "kernel/memory/pmm/include/pmm.h"
#include "kernel/memory/vmm/include/vmm.h"
#include "kernel/memory/heap/include/heap.h"
#include "kernel/vfs/include/vfs.h"
#include "kernel/keyboard/include/keyboard.h"

// The C Syscall Handler called from syscall_entry.asm (Ring 3 SYSCALL)
uint64_t syscall_handler(uint64_t id, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5) {
    if (id >= MAX_SYSCALL) {
        return (uint64_t)-1;
    }

    switch (id) {
        case SYS_YIELD:
            scheduler_yield();
            return 0;

        case SYS_WRITE:
            // arg1 = const char* str
            if ((void*)arg1 != 0) {
                display_print((const char*)arg1);
                return 0; // Success
            }
            return (uint64_t)-1; // Error

        case SYS_SLEEP:
            scheduler_sleep(arg1);
            return 0;

        case SYS_UPTIME:
            return timer_get_ticks();

        case SYS_GETPID:
            if (scheduler_current_task()) {
                return scheduler_current_task()->id;
            }
            return 0;

        case SYS_EXIT: {
            Task* current = scheduler_current_task();
            if (current && current != scheduler_get_idle_task()) {
                scheduler_terminate_task(current);
                scheduler_yield(); // Will not return to this task
            } else {
                // We are running the userspace process through the kernel_main bypass (idle task).
                // We cannot yield idle_task_ptr, so we just halt and wait for the timer interrupt
                // to preempt us and switch to TaskA.
                __asm__ volatile("sti");
                while(1) { __asm__ volatile("hlt"); }
            }
            return 0;
        }

        case SYS_OPEN:
            // arg1 = const char* path
            return vfs_open((const char*)arg1);

        case SYS_GETC:
            return keyboard_getc();

        case SYS_READ:
            // arg1 = int fd, arg2 = void* buffer, arg3 = size_t size
            return vfs_read((int)arg1, (void*)arg2, (size_t)arg3);

        case SYS_CLOSE:
            // arg1 = int fd
            vfs_close((int)arg1);
            return 0;

        default:
            return (uint64_t)-1;
    }
}

// Legacy INT 0x80 Dispatcher for Kernel Tasks
static uint64_t syscall_dispatcher_legacy(registers_t* regs) {
    uint64_t syscall_id = regs->rax;
    
    // Map legacy SYS_SLEEP=1, SYS_UPTIME=2, SYS_GETPID=3 to new IDs
    // In old syscall.h: SYS_SLEEP was 1, now it is 2.
    // Let's just use the C syscall_handler directly by mapping it!
    
    // If it's old SYS_SLEEP (1)
    if (syscall_id == 1) syscall_id = SYS_SLEEP;
    else if (syscall_id == 2) syscall_id = SYS_UPTIME;
    else if (syscall_id == 3) syscall_id = SYS_GETPID;

    regs->rax = syscall_handler(syscall_id, regs->rdi, regs->rsi, regs->rdx, regs->rcx, regs->r8);
    return 0;
}

void syscall_init(void) {
    // 0. Allocate a dedicated kernel stack for syscalls
    // Stack grows downwards, so we add the size to the allocated pointer
    void* stack_ptr = kmalloc(4096);
    syscall_kernel_stack = (uint64_t)stack_ptr + 4096;

    // 1. Initialize MSRs for the syscall instruction
    syscall_init_asm();
    
    // 2. Keep the old INT 0x80 handler for legacy/kernel task compatibility
    isr_register_handler(128, syscall_dispatcher_legacy);
}
