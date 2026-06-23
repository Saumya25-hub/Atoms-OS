#include "kernel/syscall/include/syscall.h"
#include "kernel/interrupt/include/isr.h"
#include "kernel/scheduler/include/scheduler.h"
#include "kernel/timer/include/timer.h"
#include "kernel/display/display.h"
#include "kernel/memory/pmm/include/pmm.h"
#include "kernel/memory/vmm/include/vmm.h"
#include "kernel/memory/heap/include/heap.h"
#include "kernel/lib/include/string.h"
#include "kernel/process/include/process.h"
#include "kernel/vfs/include/vfs.h"
#include "kernel/keyboard/include/keyboard.h"
#include "kernel/lib/include/crash_log.h"

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

        case SYS_SPAWN: {
            // arg1 = const char* path
            const char* path = (const char*)arg1;
            extern void* vmm_create_address_space(void);
            extern ProcessImage* elf_load_image(void* pml4, const char* path);
            extern bool process_build_user_stack(ProcessImage* image, void* pml4);

            void* new_pml4 = vmm_create_address_space();
            ProcessImage* new_image = elf_load_image(new_pml4, path);
            if (!new_image) {
                // Return -1 on failure
                // We should free new_pml4 but we don't have a vmm_destroy_address_space yet
                return (uint64_t)-1;
            }

            if (!process_build_user_stack(new_image, new_pml4)) {
                return (uint64_t)-1;
            }

            // The name can just be the path for now
            Task* new_task = process_spawn(new_image, path);
            if (!new_task) {
                return (uint64_t)-1;
            }

            return new_task->id; // Return PID
        }

        case SYS_READDIR:
            // arg1 = const char* path, arg2 = int index, arg3 = vfs_dirent_t* out_entry
            return vfs_readdir((const char*)arg1, (int)arg2, (vfs_dirent_t*)arg3);

        case SYS_GET_HEAP_STATS: {
            HeapStats stats;
            heap_get_stats(&stats);
            display_print("\n--- Kernel Heap Info ---\n");
            display_print("Total Size : "); display_print_dec(stats.total_size); display_print(" bytes\n");
            display_print("Used Size  : "); display_print_dec(stats.used_size); display_print(" bytes\n");
            display_print("Free Size  : "); display_print_dec(stats.free_size); display_print(" bytes\n");
            display_print("Blocks     : "); display_print_dec(stats.block_count); display_print("\n");
            display_print("Largest Free: "); display_print_dec(stats.largest_free); display_print(" bytes\n");
            display_print("------------------------\n");
            return 0;
        }

        case SYS_PS:
            scheduler_dump_tasks();
            return 0;

        case SYS_GET_KEY_EVENT:
            // arg1 = KeyboardEvent* out_event
            keyboard_get_event((KeyboardEvent*)arg1);
            return 0;

        case SYS_HEAP_DUMP:
            heap_dump_blocks();
            return 0;

        case SYS_MEMMAP:
            pmm_print_memmap();
            return 0;

        case SYS_DMESG:
            crash_log_dump();
            return 0;

        case SYS_TASK_INFO:
            scheduler_dump_task_info((uint64_t)arg1);
            return 0;

        case SYS_STRESS_HEAP:
            heap_stress_test();
            return 0;

        case SYS_HEAP_VALIDATE:
            heap_validate();
            return 0;

        case SYS_HEAP_WALK:
            heap_walk();
            return 0;

        case SYS_HEAP_TRACE_TOGGLE:
            heap_trace_toggle();
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
    // No longer using a global syscall kernel stack, we use TSS rsp0 directly!
    // void* stack_ptr = kmalloc(4096);

    // 1. Initialize MSRs for the syscall instruction
    syscall_init_asm();
    
    // 2. Keep the old INT 0x80 handler for legacy/kernel task compatibility
    isr_register_handler(128, syscall_dispatcher_legacy);
}
