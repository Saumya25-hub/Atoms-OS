#ifndef ATOMS_THREAD_MANAGER_H
#define ATOMS_THREAD_MANAGER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================
// ATOMS Kernel Thread Manager (Phase 10)
// ============================================================

#define ATOMS_MAX_THREADS       256
#define ATOMS_THREAD_STACK_SIZE 64 * 1024

typedef enum {
    ATOMS_THREAD_STATE_FREE = 0,
    ATOMS_THREAD_STATE_READY,
    ATOMS_THREAD_STATE_RUNNING,
    ATOMS_THREAD_STATE_SLEEPING,
    ATOMS_THREAD_STATE_WAITING,
    ATOMS_THREAD_STATE_SUSPENDED,
    ATOMS_THREAD_STATE_TERMINATED
} ATOMS_ThreadState;

typedef struct {
    uint32_t          tid;              // Thread ID
    uint32_t          pid;              // Parent Process ID
    char              name[32];         // Thread Name
    ATOMS_ThreadState state;            // Thread State
    uint32_t          priority;         // Priority (0 = Lowest, 31 = Realtime)
    
    uint64_t          entry_point;      // Instruction Pointer
    uint64_t          stack_pointer;    // Register RSP
    uint64_t          tls_base;         // Thread-Local Storage Pointer
    
    uint32_t          sleep_until_ms;   // Wakeup Timestamp
    void*             msg_queue[16];    // Message Queue
    uint32_t          msg_count;        // Count of Pending Messages
} ATOMS_ThreadControlBlock;

typedef ATOMS_ThreadControlBlock ATOMS_TCB;

void       ATOMS_ThreadManager_Init(void);
ATOMS_TCB* ATOMS_Thread_Create(uint32_t pid, const char* name, uint64_t entry_point, uint32_t priority);
bool       ATOMS_Thread_Suspend(uint32_t tid);
bool       ATOMS_Thread_Resume(uint32_t tid);
bool       ATOMS_Thread_Terminate(uint32_t tid);
bool       ATOMS_Thread_Sleep(uint32_t tid, uint32_t ms);
ATOMS_TCB* ATOMS_Thread_GetByTID(uint32_t tid);
uint32_t   ATOMS_Thread_GetCount(void);

#ifdef __cplusplus
}
#endif

#endif // ATOMS_THREAD_MANAGER_H
