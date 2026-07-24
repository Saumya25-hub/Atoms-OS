#include "process_manager.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/drivers/display/display.h"
#include "kernel/wm/bwe/include/bwe.h"

extern void bwe_log(const char* level, const char* msg);

static ATOMS_PCB g_pcb_table[ATOMS_MAX_PROCESSES];
static uint32_t  g_next_pid = 200;

void ATOMS_ProcessManager_Init(void) {
    for (uint32_t i = 0; i < ATOMS_MAX_PROCESSES; i++) {
        g_pcb_table[i].pid = 0;
        g_pcb_table[i].state = ATOMS_PROC_STATE_CLOSED;
    }
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

ATOMS_PCB* ATOMS_Process_Create(const char* name, const char* filepath, uint32_t parent_pid, uint32_t capabilities) {
    for (uint32_t i = 0; i < ATOMS_MAX_PROCESSES; i++) {
        if (g_pcb_table[i].state == ATOMS_PROC_STATE_CLOSED) {
            ATOMS_PCB* pcb = &g_pcb_table[i];
            pcb->pid = g_next_pid++;
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
            pcb->window_count = 0;
            pcb->capabilities_mask = capabilities;
            pcb->cpu_time_ms = 0;
            pcb->memory_used_bytes = pcb->heap_size + pcb->stack_size;
            pcb->exit_code = 0;
            pcb->ref_count = 1;
            
            pcb->state = ATOMS_PROC_STATE_READY;
            return pcb;
        }
    }
    return 0; // Table full
}

bool ATOMS_Process_Terminate(uint32_t pid, int32_t exit_code) {
    ATOMS_PCB* pcb = ATOMS_Process_GetByPID(pid);
    if (!pcb) return false;

    pcb->state = ATOMS_PROC_STATE_TERMINATED;
    pcb->exit_code = exit_code;

    // Clean up
    pcb->pid = 0;
    pcb->state = ATOMS_PROC_STATE_CLOSED;
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

uint32_t ATOMS_Process_GetCount(void) {
    uint32_t count = 0;
    for (uint32_t i = 0; i < ATOMS_MAX_PROCESSES; i++) {
        if (g_pcb_table[i].state != ATOMS_PROC_STATE_CLOSED) {
            count++;
        }
    }
    return count;
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
            display_print("            READY       ");
            display_print_dec(g_pcb_table[i].memory_used_bytes / 1024);
            display_print("    0ms\n");
        }
    }
    display_print("========================================================\n\n");
}
