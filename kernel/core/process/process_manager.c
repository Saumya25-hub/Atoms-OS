#include "process_manager.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/drivers/display/display.h"
#include "kernel/wm/bwe/include/bwe.h"
#include "kernel/core/scheduler/include/scheduler.h"

extern void bwe_log(const char* level, const char* msg);
extern bwe_error_t BOS_CloseSurfacesByPID(uint32_t pid);
extern void scheduler_terminate_tasks_by_pid(uint32_t pid);

static ATOMS_PCB g_pcb_table[ATOMS_MAX_PROCESSES];
static uint32_t  g_next_pid = 200;

static inline uint64_t disable_interrupts(void) {
    uint64_t rflags;
    __asm__ volatile("pushfq; pop %0; cli" : "=r"(rflags));
    return rflags;
}

static inline void restore_interrupts(uint64_t rflags) {
    __asm__ volatile("push %0; popfq" :: "r"(rflags));
}

void ATOMS_ProcessManager_Init(void) {
    uint64_t rflags = disable_interrupts();
    for (uint32_t i = 0; i < ATOMS_MAX_PROCESSES; i++) {
        g_pcb_table[i].pid = 0;
        g_pcb_table[i].state = ATOMS_PROC_STATE_CLOSED;
        g_pcb_table[i].parent_pid = 0;
        g_pcb_table[i].thread_count = 0;
        g_pcb_table[i].window_count = 0;
    }
    g_next_pid = 200;
    restore_interrupts(rflags);
    bwe_log("INFO", "ATOMS Enterprise Process Manager Initialized");
}

static void str_copy_limit(char* dest, const char* src, uint32_t limit) {
    if (!dest || !src || limit == 0) return;
    uint32_t i = 0;
    while (src[i] && i < limit - 1) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

uint32_t ATOMS_PID_Alloc(void) {
    uint64_t rflags = disable_interrupts();
    uint32_t attempts = 0;
    while (attempts < 1000) {
        uint32_t candidate = g_next_pid++;
        if (g_next_pid >= 10000) g_next_pid = 200; // wrap around
        
        // Ensure not in use
        bool in_use = false;
        for (uint32_t i = 0; i < ATOMS_MAX_PROCESSES; i++) {
            if (g_pcb_table[i].pid == candidate && g_pcb_table[i].state != ATOMS_PROC_STATE_CLOSED) {
                in_use = true;
                break;
            }
        }
        if (!in_use) {
            restore_interrupts(rflags);
            return candidate;
        }
        attempts++;
    }
    restore_interrupts(rflags);
    return 0; // Out of PIDs
}

void ATOMS_PID_Free(uint32_t pid) {
    // PID reuse is handled by the allocator scanning active PCBs
    (void)pid;
}

ATOMS_PCB* ATOMS_Process_Create(const char* name, const char* filepath, uint32_t parent_pid, uint32_t capabilities) {
    uint64_t rflags = disable_interrupts();
    for (uint32_t i = 0; i < ATOMS_MAX_PROCESSES; i++) {
        if (g_pcb_table[i].state == ATOMS_PROC_STATE_CLOSED) {
            ATOMS_PCB* pcb = &g_pcb_table[i];
            pcb->pid = ATOMS_PID_Alloc();
            if (pcb->pid == 0) {
                restore_interrupts(rflags);
                return 0;
            }
            pcb->parent_pid = parent_pid;
            str_copy_limit(pcb->name, name ? name : "App", sizeof(pcb->name));
            str_copy_limit(pcb->filepath, filepath ? filepath : "", sizeof(pcb->filepath));
            str_copy_limit(pcb->working_dir, "/", sizeof(pcb->working_dir));
            
            pcb->state = ATOMS_PROC_STATE_CREATING;
            pcb->pml4_phys = 0;
            pcb->heap_base = 0x60000000;
            pcb->heap_size = 1024 * 1024;
            pcb->stack_base = 0x7FFFF000;
            pcb->stack_size = 64 * 1024;
            
            pcb->thread_count = 0;
            for (uint32_t t = 0; t < ATOMS_MAX_THREADS_PER_PROC; t++) {
                pcb->thread_ids[t] = 0;
            }
            pcb->window_count = 0;
            for (uint32_t w = 0; w < ATOMS_MAX_WINDOWS_PER_PROC; w++) {
                pcb->window_ids[w] = 0;
            }
            pcb->capabilities_mask = capabilities;
            pcb->cpu_time_ms = 0;
            pcb->memory_used_bytes = pcb->heap_size + pcb->stack_size;
            pcb->exit_code = 0;
            pcb->ref_count = 1;
            
            pcb->state = ATOMS_PROC_STATE_READY;
            restore_interrupts(rflags);
            return pcb;
        }
    }
    restore_interrupts(rflags);
    return 0; // Table full
}

bool ATOMS_Process_Terminate(uint32_t pid, int32_t exit_code) {
    uint64_t rflags = disable_interrupts();
    ATOMS_PCB* pcb = ATOMS_Process_GetByPID(pid);
    if (!pcb) {
        restore_interrupts(rflags);
        return false;
    }

    pcb->state = ATOMS_PROC_STATE_TERMINATED;
    pcb->exit_code = exit_code;

    // 1. Terminate all tasks belonging to this process in the scheduler
    scheduler_terminate_tasks_by_pid(pid);

    // 2. Clean up owned window surfaces
    BOS_CloseSurfacesByPID(pid);

    // 3. Orphan re-parenting:
    // Any child of this process should be re-parented to PID 1 (init/system task)
    // If the child is already a ZOMBIE, we reap it immediately.
    for (uint32_t i = 0; i < ATOMS_MAX_PROCESSES; i++) {
        if (g_pcb_table[i].parent_pid == pid && g_pcb_table[i].state != ATOMS_PROC_STATE_CLOSED) {
            g_pcb_table[i].parent_pid = 1; // Adopted by init/system
            if (g_pcb_table[i].state == ATOMS_PROC_STATE_ZOMBIE) {
                // Reap zombie orphan
                g_pcb_table[i].pid = 0;
                g_pcb_table[i].state = ATOMS_PROC_STATE_CLOSED;
            }
        }
    }

    // 4. Zombie / Reap decision
    // If the parent is PID 1 or 0 (system processes), we reap the exiting process immediately
    if (pcb->parent_pid <= 1) {
        pcb->pid = 0;
        pcb->state = ATOMS_PROC_STATE_CLOSED;
    } else {
        // Parent is alive and might wait/reap it, transition to ZOMBIE
        pcb->state = ATOMS_PROC_STATE_ZOMBIE;
    }

    restore_interrupts(rflags);
    return true;
}

ATOMS_PCB* ATOMS_Process_GetByPID(uint32_t pid) {
    if (pid == 0) return 0;
    for (uint32_t i = 0; i < ATOMS_MAX_PROCESSES; i++) {
        if (g_pcb_table[i].pid == pid && g_pcb_table[i].state != ATOMS_PROC_STATE_CLOSED) {
            return &g_pcb_table[i];
        }
    }
    return 0;
}

ATOMS_PCB* ATOMS_Process_GetByIndex(uint32_t index) {
    if (index >= ATOMS_MAX_PROCESSES) return 0;
    if (g_pcb_table[index].state != ATOMS_PROC_STATE_CLOSED) {
        return &g_pcb_table[index];
    }
    return 0;
}

uint32_t ATOMS_Process_GetCount(void) {
    uint32_t count = 0;
    for (uint32_t i = 0; i < ATOMS_MAX_PROCESSES; i++) {
        if (g_pcb_table[i].state != ATOMS_PROC_STATE_CLOSED && g_pcb_table[i].state != ATOMS_PROC_STATE_ZOMBIE) {
            count++;
        }
    }
    return count;
}

int32_t ATOMS_Process_Wait(uint32_t pid, int32_t* out_exit_code) {
    ATOMS_PCB* child = ATOMS_Process_GetByPID(pid);
    if (!child) return -1;
    
    Task* cur = scheduler_current_task();
    uint32_t caller_pid = cur ? (uint32_t)cur->id : 0;
    
    // Validate caller is parent or system
    if (child->parent_pid != caller_pid && caller_pid != 1 && caller_pid != 0) {
        return -1;
    }
    
    // Yield in a loop if the child is still active
    if (caller_pid != 0) {
        while (child->state != ATOMS_PROC_STATE_ZOMBIE && child->state != ATOMS_PROC_STATE_CLOSED) {
            scheduler_yield();
        }
    }
    
    // Reap if it is a zombie
    if (child->state == ATOMS_PROC_STATE_ZOMBIE) {
        uint64_t rflags = disable_interrupts();
        if (out_exit_code) *out_exit_code = child->exit_code;
        child->pid = 0;
        child->state = ATOMS_PROC_STATE_CLOSED;
        restore_interrupts(rflags);
        return (int32_t)pid;
    }
    
    return 0;
}

void ATOMS_Process_DumpTelemetry(void) {
    display_print("\n========================================================\n");
    display_print("         ATOMS Enterprise Process Telemetry             \n");
    display_print("========================================================\n");
    display_print("  PID   Process Name         State       Mem (KB)   CPU \n");
    display_print("--------------------------------------------------------\n");

    for (uint32_t i = 0; i < ATOMS_MAX_PROCESSES; i++) {
        if (g_pcb_table[i].state != ATOMS_PROC_STATE_CLOSED) {
            display_print("  ");
            display_print_dec(g_pcb_table[i].pid);
            display_print("   ");
            display_print(g_pcb_table[i].name);
            display_print("            ");
            switch (g_pcb_table[i].state) {
                case ATOMS_PROC_STATE_CREATING: display_print("INIT        "); break;
                case ATOMS_PROC_STATE_READY:    display_print("READY       "); break;
                case ATOMS_PROC_STATE_RUNNING:  display_print("RUNNING     "); break;
                case ATOMS_PROC_STATE_SUSPENDED:display_print("SUSPENDED   "); break;
                case ATOMS_PROC_STATE_ZOMBIE:   display_print("ZOMBIE      "); break;
                default:                        display_print("TERMINATED  "); break;
            }
            display_print_dec(g_pcb_table[i].memory_used_bytes / 1024);
            display_print("    0ms\n");
        }
    }
    display_print("========================================================\n\n");
}
