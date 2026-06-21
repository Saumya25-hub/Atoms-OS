#include "kernel/syscall/include/syscall.h"
#include "kernel/interrupt/include/isr.h"
#include "kernel/scheduler/include/scheduler.h"
#include "kernel/timer/include/timer.h"
#include "kernel/display/display.h"

// The Syscall Dispatcher
static uint64_t syscall_dispatcher(registers_t* regs) {
    uint64_t syscall_id = regs->rax;

    // RULE 94: Kernel services must validate syscall IDs before execution.
    // RULE 95: Invalid syscalls must safely return an error.
    if (syscall_id >= MAX_SYSCALL) {
        regs->rax = (uint64_t)-1; // -1 represents an invalid syscall error in the user's RAX
        return 0; // 0 means no stack switch
    }

    // Process the syscall
    switch (syscall_id) {
        case SYS_YIELD:
            // Delegate to scheduler's explicitly authorized API
            scheduler_yield();
            break;

        case SYS_SLEEP:
            // Arg 1 is in RDI
            scheduler_sleep(regs->rdi);
            break;

        case SYS_UPTIME:
            // Return value goes into RAX.
            // The isr_common_handler does NOT natively update the pushed RAX in the stack 
            // unless we modify the struct directly.
            regs->rax = timer_get_ticks();
            break;

        case SYS_GETPID:
            if (scheduler_current_task()) {
                regs->rax = scheduler_current_task()->id;
            } else {
                regs->rax = 0;
            }
            break;
            
        default:
            regs->rax = (uint64_t)-1;
            break;
    }

    return 0; // Return 0 means no stack switch requested via RAX to the assembly stub
}

void syscall_init(void) {
    // Register the dispatcher to INT 0x80 (128)
    isr_register_handler(128, syscall_dispatcher);
}
