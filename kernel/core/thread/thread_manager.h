#ifndef ATOMS_THREAD_MANAGER_H
#define ATOMS_THREAD_MANAGER_H

#include "../execution/include/execution_contract.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ATOMS_MAX_THREADS 10000U
#define ATOMS_THREAD_STACK_SIZE (64U * 1024U)
#define ATOMS_THREAD_NAME_LENGTH 32U
#define ATOMS_THREAD_PRIORITY_LEVELS 32U
#define ATOMS_CPU_AFFINITY_ALL UINT64_MAX

typedef enum {
  ATOMS_THREAD_STATE_FREE = 0,
  ATOMS_THREAD_STATE_CREATED,
  ATOMS_THREAD_STATE_READY,
  ATOMS_THREAD_STATE_RUNNING,
  ATOMS_THREAD_STATE_BLOCKED,
  ATOMS_THREAD_STATE_SLEEPING,
  ATOMS_THREAD_STATE_WAITING,
  ATOMS_THREAD_STATE_SUSPENDED,
  ATOMS_THREAD_STATE_DEAD,
  ATOMS_THREAD_STATE_TERMINATED = ATOMS_THREAD_STATE_DEAD
} ATOMS_ThreadState;

typedef struct {
  uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
  uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
  uint64_t rip, rsp, rflags, cs, ss, cr3;
} ATOMS_ThreadRegisters;

typedef struct {
  void *base;
  uint64_t size;
  uint64_t guard_size;
} ATOMS_ThreadStack;

typedef struct {
  uint64_t user_ticks;
  uint64_t kernel_ticks;
  uint64_t wait_ticks;
  uint64_t run_count;
  uint64_t context_switches;
  uint64_t voluntary_switches;
  uint64_t involuntary_switches;
  uint64_t created_tick;
  uint64_t exit_tick;
} ATOMS_ThreadExecutionStatistics;

typedef struct {
  uint8_t base_priority;
  uint8_t effective_priority;
  uint8_t inherited_priority;
  uint8_t queue_priority;
  uint32_t quantum_ticks;
  uint32_t remaining_ticks;
  uint64_t ready_since_tick;
  uint64_t wake_tick;
  uint64_t last_run_tick;
  uint64_t affinity_mask;
  uint32_t last_cpu;
  uint32_t starvation_boosts;
} ATOMS_ThreadSchedulingInfo;

typedef struct {
  uint64_t state_transitions;
  uint64_t invalid_transitions;
  uint32_t last_error;
  uint32_t warning_flags;
} ATOMS_ThreadDiagnostics;

struct Task;
typedef struct ATOMS_ThreadControlBlock {
  uint32_t tid;
  uint32_t pid;
  uint32_t generation;
  char name[ATOMS_THREAD_NAME_LENGTH];
  ATOMS_ThreadState state;
  uint32_t priority;
  ATOMS_ThreadRegisters registers;
  ATOMS_ThreadStack user_stack;
  ATOMS_ThreadStack kernel_stack;
  uint64_t entry_point;
  uint64_t stack_pointer;
  uint64_t tls_base;
  uint64_t cpu_affinity;
  uint64_t sleep_until_ms;
  ATOMS_ThreadExecutionStatistics statistics;
  ATOMS_ThreadSchedulingInfo scheduling;
  ATOMS_ThreadDiagnostics diagnostics;
  struct Task *task;
  uint32_t joiner_tid;
  int32_t exit_code;
  bool detached;
  bool kill_pending;
  void *msg_queue[16];
  uint32_t msg_count;
} ATOMS_ThreadControlBlock;

typedef ATOMS_ThreadControlBlock ATOMS_TCB;

typedef struct {
  uint32_t capacity;
  uint32_t active;
  uint32_t dead;
  uint32_t peak_active;
  uint64_t creations;
  uint64_t exits;
  uint64_t invalid_transitions;
} ATOMS_ThreadManagerDiagnostics;

void ATOMS_ThreadManager_Init(void);
ATOMS_TCB *ATOMS_Thread_Create(uint32_t pid, const char *name,
                               uint64_t entry_point, uint32_t priority);
ATOMS_ExecutionErrorCode ATOMS_Thread_BindTask(uint32_t tid, struct Task *task);
ATOMS_TCB *ATOMS_Thread_CreateKernel(uint32_t pid, const char *name,
                                     void (*entry)(void), uint32_t priority);
ATOMS_TCB *ATOMS_Thread_CreateUser(uint32_t pid, const char *name,
                                   void (*entry)(void), uint32_t priority);
ATOMS_ExecutionErrorCode ATOMS_Thread_Transition(ATOMS_TCB *tcb,
                                                 ATOMS_ThreadState state);
bool ATOMS_Thread_Suspend(uint32_t tid);
bool ATOMS_Thread_Resume(uint32_t tid);
bool ATOMS_Thread_Terminate(uint32_t tid);
bool ATOMS_Thread_Kill(uint32_t tid, int32_t exit_code);
bool ATOMS_Thread_Sleep(uint32_t tid, uint32_t ms);
bool ATOMS_Thread_Yield(uint32_t tid);
ATOMS_ExecutionErrorCode ATOMS_Thread_Join(uint32_t tid,
                                           int32_t *out_exit_code);
ATOMS_ExecutionErrorCode ATOMS_Thread_Detach(uint32_t tid);
ATOMS_TCB *ATOMS_Thread_GetByTID(uint32_t tid);
ATOMS_TCB *ATOMS_Thread_GetByTask(const struct Task *task);
uint32_t ATOMS_Thread_GetCount(void);
void ATOMS_Thread_AccountCPU(uint32_t tid, bool kernel_mode, uint64_t ticks);
void ATOMS_Thread_GetManagerDiagnostics(ATOMS_ThreadManagerDiagnostics *out);
void ATOMS_Thread_DumpTelemetry(void);

#ifdef __cplusplus
}
#endif

#endif
