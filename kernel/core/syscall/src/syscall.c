#include "kernel/core/syscall/include/syscall.h"
#include "kernel/core/interrupt/include/isr.h"
#include "kernel/core/scheduler/include/scheduler.h"
#include "kernel/core/timer/include/timer.h"
#include "kernel/drivers/display/display.h"
#include "kernel/core/memory/pmm/include/pmm.h"
#include "kernel/core/memory/vmm/include/vmm.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/core/process/include/process.h"
#include "kernel/core/process/process_manager.h"
#include "kernel/vfs/vfs_legacy/include/vfs.h"
#include "kernel/drivers/keyboard/include/keyboard.h"
#include "kernel/core/lib/include/crash_log.h"
#include "kernel/shell/conhost/conhost.h"
#include "kernel/wm/surface/surface.h"
#include "kernel/ui/events/gui_events.h"

volatile uint64_t g_sys_get_input_event_calls = 0;
volatile uint64_t g_sys_get_input_event_empty = 0;
volatile uint32_t g_sys_get_input_event_last_pid = 0;
volatile uint32_t g_doom_checkpoint = 0;

// The C Syscall Handler called from syscall_entry.asm (Ring 3 SYSCALL)
uint64_t syscall_handler(uint64_t id, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5) {
    if (id >= MAX_SYSCALL) {
        return SYSCALL_INVALID;
    }

    switch (id) {
        case 300:
            g_doom_checkpoint = (uint32_t)arg1;
            return 0;

        case SYS_YIELD:
            scheduler_yield();
            return 0;

        case SYS_WRITE:
            // arg1 = const char* str
            if ((void*)arg1 != 0) {
                Task* curr = scheduler_current_task();
                if (curr && conhost_write_pid(curr->id, (const char*)arg1)) {
                    return 0; // Handled by ConHost
                }
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
                extern bool ATOMS_Process_Terminate(uint32_t pid, int32_t exit_code);
                ATOMS_Process_Terminate((uint32_t)current->id, (int32_t)arg1);
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
            extern ATOMS_PCB* ATOMS_Process_Create(const char* name, const char* filepath, uint32_t parent_pid, uint32_t capabilities);
            extern bool ATOMS_Process_Terminate(uint32_t pid, int32_t exit_code);

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

            // Create unified process PCB first
            Task* curr = scheduler_current_task();
            uint32_t ppid = curr ? (uint32_t)curr->id : 0;
            ATOMS_PCB* pcb = ATOMS_Process_Create(path, path, ppid, 0);
            if (!pcb) {
                return (uint64_t)-1;
            }
            pcb->pml4_phys = (uint64_t)new_pml4;
            new_image->pid = pcb->pid; // Set PID in process image so process_spawn knows it

            // The name can just be the path for now
            Task* new_task = process_spawn(new_image, path);
            if (!new_task) {
                ATOMS_Process_Terminate(pcb->pid, -1);
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

        case SYS_GET_KEY_EVENT: {
            Task* curr = scheduler_current_task();
            if (curr && conhost_get_session_by_pid(curr->id)) {
                if (conhost_pop_key_pid(curr->id, (KeyboardEvent*)arg1)) {
                    return 1;
                }
                return 0;
            }
            if (keyboard_poll_event((KeyboardEvent*)arg1)) {
                return 1;
            }
            return 0;
        }

        case SYS_GET_INPUT_EVENT: {
            g_sys_get_input_event_calls++;
            Task* curr = scheduler_current_task();
            if (curr) {
                g_sys_get_input_event_last_pid = (uint32_t)curr->id;
                extern bool bwe_process_queue_pop(uint32_t owner_pid, void* out_event);
                if (bwe_process_queue_pop(curr->id, (void*)arg1)) {
                    return 1;
                }
            }
            g_sys_get_input_event_empty++;
            return 0;
        }

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

        case SYS_WRITE_FILE:
            // arg1 = int fd, arg2 = void* buffer, arg3 = size_t size
            return vfs_write((int)arg1, (void*)arg2, (size_t)arg3);

        case SYS_MKDIR:
            // arg1 = const char* path
            return vfs_mkdir((const char*)arg1);

        case SYS_CREATE:
            // arg1 = const char* path
            return vfs_create((const char*)arg1);

        case SYS_RENAME:
            // arg1 = const char* old_path, arg2 = const char* new_name
            return vfs_rename((const char*)arg1, (const char*)arg2);

        case SYS_DELETE:
            // arg1 = const char* path
            return vfs_delete((const char*)arg1);

        case SYS_CLEAR_SCREEN: {
            Task* curr = scheduler_current_task();
            if (curr && conhost_clear_pid(curr->id)) {
                return 0;
            }
            display_clear();
            return 0;
        }

        case SYS_SET_CURSOR: {
            Task* curr = scheduler_current_task();
            if (curr && conhost_set_cursor_pid(curr->id, (uint16_t)arg1, (uint16_t)arg2)) {
                return 0;
            }
            display_set_cursor((uint16_t)arg1, (uint16_t)arg2);
            return 0;
        }

        case SYS_GUI_CREATE_WINDOW: {
            extern uint32_t g_current_creating_pid;
            Task* curr = scheduler_current_task();
            g_current_creating_pid = curr ? curr->id : 0;
            uint32_t win_id = 0;
            bwe_error_t err = BOS_CreateWindow((int32_t)arg1, (int32_t)arg2, (int32_t)arg3, (int32_t)arg4, (const char*)arg5, &win_id);
            g_current_creating_pid = 0;
            return (err == BWE_SUCCESS) ? win_id : SYSCALL_FAIL;
        }

        case SYS_GUI_CREATE_BUTTON: {
            uint64_t* ext = (uint64_t*)arg5;
            uint32_t btn_id = 0;
            bwe_error_t err = BOS_CreateButton((uint32_t)arg1, (uint32_t)arg2, (uint32_t)arg3, (uint32_t)arg4, (uint32_t)ext[0], (const char*)ext[1], 0, &btn_id);
            if (err == BWE_SUCCESS) {
                BWE_Surface* s = BWE_GetSurface(btn_id);
                if (s) {
                    s->owner_pid = scheduler_current_task()->id;
                    s->control_data.button.user_callback = ext[2];
                }
            }
            return (err == BWE_SUCCESS) ? btn_id : SYSCALL_FAIL;
        }

        case SYS_GUI_CREATE_LABEL: {
            uint32_t lbl_id = 0;
            bwe_error_t err = BOS_CreateLabel((uint32_t)arg1, (uint32_t)arg2, (uint32_t)arg3, (const char*)arg4, (uint32_t)arg5, &lbl_id);
            if (err == BWE_SUCCESS) {
                BWE_Surface* s = BWE_GetSurface(lbl_id);
                if (s) s->owner_pid = scheduler_current_task()->id;
            }
            return (err == BWE_SUCCESS) ? lbl_id : SYSCALL_FAIL;
        }

        case SYS_GUI_CREATE_PANEL: {
            uint64_t* ext = (uint64_t*)arg5;
            uint32_t pnl_id = 0;
            bwe_error_t err = BOS_CreatePanel((uint32_t)arg1, (uint32_t)arg2, (uint32_t)arg3, (uint32_t)arg4, (uint32_t)ext[0], (uint32_t)ext[1], &pnl_id);
            if (err == BWE_SUCCESS) {
                BWE_Surface* s = BWE_GetSurface(pnl_id);
                if (s) s->owner_pid = scheduler_current_task()->id;
            }
            return (err == BWE_SUCCESS) ? pnl_id : SYSCALL_FAIL;
        }

        case SYS_GUI_SHOW_WINDOW: {
            uint32_t window_id = (uint32_t)arg1;
            if (BOS_Show(window_id) != BWE_SUCCESS) {
                return SYSCALL_FAIL;
            }
            return BOS_SetFocus(window_id) == BWE_SUCCESS ? SYSCALL_OK : SYSCALL_FAIL;
        }

        case SYS_GUI_SET_TEXT: {
            return BOS_SetText((uint32_t)arg1, (const char*)arg2) == BWE_SUCCESS ? SYSCALL_OK : SYSCALL_FAIL;
        }

        case SYS_GUI_GET_EVENT: {
            Task* curr = scheduler_current_task();
            if (!curr) return 0;
            return bos_gui_event_pop(curr->id, (BOS_GUIEvent*)arg1);
        }

        case SYS_SEEK:
            // arg1 = int fd, arg2 = uint64_t offset, arg3 = int whence
            return vfs_seek((int)arg1, (uint64_t)arg2, (int)arg3);

        case SYS_SURFACE_PRESENT: {
            // arg1 = window_id, arg2 = const uint32_t* pixels, arg3 = w, arg4 = h
            extern bwe_error_t BOS_SurfacePresent(uint32_t window_id, const uint32_t* pixels, uint32_t w, uint32_t h);
            return BOS_SurfacePresent((uint32_t)arg1, (const uint32_t*)arg2, (uint32_t)arg3, (uint32_t)arg4) == BWE_SUCCESS ? SYSCALL_OK : SYSCALL_FAIL;
        }

        default:
            return SYSCALL_NOT_IMPLEMENTED;
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
