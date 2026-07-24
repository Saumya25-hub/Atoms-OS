#include "thread_manager.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/wm/bwe/include/bwe.h"

extern void bwe_log(const char* level, const char* msg);

static ATOMS_TCB g_tcb_table[ATOMS_MAX_THREADS];
static uint32_t  g_next_tid = 1000;

void ATOMS_ThreadManager_Init(void) {
    for (uint32_t i = 0; i < ATOMS_MAX_THREADS; i++) {
        g_tcb_table[i].tid = 0;
        g_tcb_table[i].state = ATOMS_THREAD_STATE_FREE;
    }
    bwe_log("INFO", "ATOMS Kernel Thread Manager Initialized");
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

ATOMS_TCB* ATOMS_Thread_Create(uint32_t pid, const char* name, uint64_t entry_point, uint32_t priority) {
    for (uint32_t i = 0; i < ATOMS_MAX_THREADS; i++) {
        if (g_tcb_table[i].state == ATOMS_THREAD_STATE_FREE) {
            ATOMS_TCB* tcb = &g_tcb_table[i];
            tcb->tid = g_next_tid++;
            tcb->pid = pid;
            str_copy_limit(tcb->name, name ? name : "MainThread", sizeof(tcb->name));
            tcb->state = ATOMS_THREAD_STATE_READY;
            tcb->priority = priority;
            tcb->entry_point = entry_point;
            tcb->stack_pointer = 0x7FFFF000;
            tcb->tls_base = 0;
            tcb->sleep_until_ms = 0;
            tcb->msg_count = 0;
            return tcb;
        }
    }
    return 0;
}

bool ATOMS_Thread_Suspend(uint32_t tid) {
    ATOMS_TCB* tcb = ATOMS_Thread_GetByTID(tid);
    if (!tcb || tcb->state == ATOMS_THREAD_STATE_FREE) return false;
    tcb->state = ATOMS_THREAD_STATE_SUSPENDED;
    return true;
}

bool ATOMS_Thread_Resume(uint32_t tid) {
    ATOMS_TCB* tcb = ATOMS_Thread_GetByTID(tid);
    if (!tcb || tcb->state != ATOMS_THREAD_STATE_SUSPENDED) return false;
    tcb->state = ATOMS_THREAD_STATE_READY;
    return true;
}

bool ATOMS_Thread_Terminate(uint32_t tid) {
    ATOMS_TCB* tcb = ATOMS_Thread_GetByTID(tid);
    if (!tcb) return false;
    tcb->state = ATOMS_THREAD_STATE_TERMINATED;
    tcb->tid = 0;
    tcb->state = ATOMS_THREAD_STATE_FREE;
    return true;
}

bool ATOMS_Thread_Sleep(uint32_t tid, uint32_t ms) {
    ATOMS_TCB* tcb = ATOMS_Thread_GetByTID(tid);
    if (!tcb) return false;
    tcb->state = ATOMS_THREAD_STATE_SLEEPING;
    tcb->sleep_until_ms = ms;
    return true;
}

ATOMS_TCB* ATOMS_Thread_GetByTID(uint32_t tid) {
    if (tid == 0) return 0;
    for (uint32_t i = 0; i < ATOMS_MAX_THREADS; i++) {
        if (g_tcb_table[i].tid == tid && g_tcb_table[i].state != ATOMS_THREAD_STATE_FREE) {
            return &g_tcb_table[i];
        }
    }
    return 0;
}

uint32_t ATOMS_Thread_GetCount(void) {
    uint32_t count = 0;
    for (uint32_t i = 0; i < ATOMS_MAX_THREADS; i++) {
        if (g_tcb_table[i].state != ATOMS_THREAD_STATE_FREE) {
            count++;
        }
    }
    return count;
}
