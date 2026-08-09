#include "thread_manager.h"
#include "../execution/include/execution_contract.h"
#include "../process/process_manager.h"
#include "../scheduler/include/scheduler.h"
#include "../timer/include/timer.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/drivers/display/display.h"
#include <stddef.h>

#define THREAD_MODULE_ID 2U

static ATOMS_TCB g_tcb_table[ATOMS_MAX_THREADS];
static ATOMS_ThreadManagerDiagnostics g_diag;
static uint32_t g_next_tid = 1000U;
static uint32_t g_generation = 1U;
static bool g_initialized;

static uint64_t irq_save(void) {
  uint64_t flags;
  __asm__ volatile("pushfq; pop %0; cli" : "=r"(flags)::"memory");
  return flags;
}

static void irq_restore(uint64_t flags) {
  __asm__ volatile("push %0; popfq" ::"r"(flags) : "memory", "cc");
}

static void zero_bytes(void *memory, uint64_t size) {
  uint8_t *bytes = (uint8_t *)memory;
  for (uint64_t i = 0; i < size; ++i)
    bytes[i] = 0;
}

static void copy_name(char *destination, const char *source) {
  if (!source)
    source = "thread";
  uint32_t i = 0;
  while (i + 1U < ATOMS_THREAD_NAME_LENGTH && source[i]) {
    destination[i] = source[i];
    ++i;
  }
  destination[i] = '\0';
}

static ATOMS_TCB *find_tid_locked(uint32_t tid) {
  if (!tid)
    return 0;
  for (uint32_t i = 0; i < ATOMS_MAX_THREADS; ++i) {
    if (g_tcb_table[i].tid == tid &&
        g_tcb_table[i].state != ATOMS_THREAD_STATE_FREE)
      return &g_tcb_table[i];
  }
  return 0;
}

static uint32_t allocate_tid_locked(void) {
  for (uint32_t attempts = 0; attempts < ATOMS_MAX_THREADS * 2U; ++attempts) {
    uint32_t candidate = g_next_tid++;
    if (g_next_tid < 1000U)
      g_next_tid = 1000U;
    if (!find_tid_locked(candidate))
      return candidate;
  }
  return 0;
}

void ATOMS_ThreadManager_Init(void) {
  uint64_t flags = irq_save();
  zero_bytes(g_tcb_table, sizeof(g_tcb_table));
  zero_bytes(&g_diag, sizeof(g_diag));
  g_diag.capacity = ATOMS_MAX_THREADS;
  g_next_tid = 1000U;
  g_generation = 1U;
  g_initialized = true;
  irq_restore(flags);
}

ATOMS_TCB *ATOMS_Thread_Create(uint32_t pid, const char *name,
                               uint64_t entry_point, uint32_t priority) {
  if (!g_initialized || !pid || priority >= ATOMS_THREAD_PRIORITY_LEVELS)
    return 0;
  uint64_t flags = irq_save();
  ATOMS_TCB *tcb = 0;
  for (uint32_t i = 0; i < ATOMS_MAX_THREADS; ++i) {
    if (g_tcb_table[i].state == ATOMS_THREAD_STATE_FREE) {
      tcb = &g_tcb_table[i];
      break;
    }
  }
  if (!tcb) {
    irq_restore(flags);
    return 0;
  }
  uint32_t tid = allocate_tid_locked();
  if (!tid) {
    irq_restore(flags);
    return 0;
  }
  zero_bytes(tcb, sizeof(*tcb));
  tcb->tid = tid;
  tcb->pid = pid;
  tcb->generation = g_generation++;
  copy_name(tcb->name, name);
  tcb->entry_point = entry_point;
  tcb->priority = priority;
  tcb->state = ATOMS_THREAD_STATE_CREATED;
  tcb->cpu_affinity = ATOMS_CPU_AFFINITY_ALL;
  tcb->scheduling.base_priority = (uint8_t)priority;
  tcb->scheduling.effective_priority = (uint8_t)priority;
  tcb->scheduling.queue_priority = (uint8_t)priority;
  tcb->scheduling.quantum_ticks = 1U + priority / 4U;
  tcb->scheduling.remaining_ticks = tcb->scheduling.quantum_ticks;
  tcb->statistics.created_tick = scheduler_get_tick_count();
  tcb->state = ATOMS_THREAD_STATE_READY;
  ++tcb->diagnostics.state_transitions;
  ++g_diag.active;
  ++g_diag.creations;
  if (g_diag.active > g_diag.peak_active)
    g_diag.peak_active = g_diag.active;
  irq_restore(flags);

  if (ATOMS_Process_RegisterThread(pid, tid) != ATOMS_EXEC_OK) {
    flags = irq_save();
    zero_bytes(tcb, sizeof(*tcb));
    irq_restore(flags);
    return 0;
  }
  ATOMS_Execution_Trace(THREAD_MODULE_ID, ATOMS_TRACE_THREAD_CREATE, tid, pid,
                        ATOMS_THREAD_STATE_READY, tcb->generation);
  return tcb;
}

static ATOMS_TCB *create_scheduled_thread(uint32_t pid, const char *name,
                                          void (*entry)(void),
                                          uint32_t priority, bool user) {
  if (!entry)
    return 0;
  ATOMS_TCB *tcb = ATOMS_Thread_Create(pid, name, (uint64_t)entry, priority);
  if (!tcb)
    return 0;
  Task *task = user ? scheduler_create_user_task(name, entry)
                    : scheduler_create_kernel_task(name, entry, (uint8_t)priority);
  if (!task) {
    ATOMS_Thread_Terminate(tcb->tid);
    return 0;
  }
  task->owner_pid = pid;
  if (!scheduler_set_task_priority(task, (uint8_t)priority) ||
      ATOMS_Thread_BindTask(tcb->tid, task) != ATOMS_EXEC_OK) {
    scheduler_terminate_task(task);
    ATOMS_Thread_Terminate(tcb->tid);
    return 0;
  }
  tcb->kernel_stack.base = task->stack;
  tcb->kernel_stack.size = KERNEL_TASK_STACK_SIZE;
  tcb->user_stack.base = task->user_stack;
  tcb->user_stack.size = task->user_stack ? KERNEL_TASK_STACK_SIZE : 0;
  tcb->stack_pointer = task->rsp;
  return tcb;
}

ATOMS_TCB *ATOMS_Thread_CreateKernel(uint32_t pid, const char *name,
                                     void (*entry)(void), uint32_t priority) {
  return create_scheduled_thread(pid, name, entry, priority, false);
}

ATOMS_TCB *ATOMS_Thread_CreateUser(uint32_t pid, const char *name,
                                   void (*entry)(void), uint32_t priority) {
  return create_scheduled_thread(pid, name, entry, priority, true);
}

ATOMS_ExecutionErrorCode ATOMS_Thread_BindTask(uint32_t tid,
                                               struct Task *task) {
  if (!task)
    return ATOMS_EXEC_ERR_INVALID_ARGUMENT;
  uint64_t flags = irq_save();
  ATOMS_TCB *tcb = find_tid_locked(tid);
  if (!tcb) {
    irq_restore(flags);
    return ATOMS_EXEC_ERR_NOT_FOUND;
  }
  if (tcb->task && tcb->task != task) {
    irq_restore(flags);
    return ATOMS_EXEC_ERR_BUSY;
  }
  tcb->task = task;
  tcb->stack_pointer = task->rsp;
  irq_restore(flags);
  return ATOMS_EXEC_OK;
}

ATOMS_ExecutionErrorCode ATOMS_Thread_Transition(ATOMS_TCB *tcb,
                                                 ATOMS_ThreadState state) {
  if (!tcb)
    return ATOMS_EXEC_ERR_INVALID_ARGUMENT;
  uint64_t flags = irq_save();
  if (!ATOMS_ThreadStateTransitionValid((uint32_t)tcb->state,
                                        (uint32_t)state)) {
    ++tcb->diagnostics.invalid_transitions;
    ++g_diag.invalid_transitions;
    tcb->diagnostics.last_error = ATOMS_EXEC_ERR_INVALID_STATE;
    irq_restore(flags);
    ATOMS_ExecutionError error = {
        .module = THREAD_MODULE_ID,
        .code = ATOMS_EXEC_ERR_INVALID_STATE,
        .severity = ATOMS_EXEC_SEVERITY_ERROR,
        .object_id = tcb->tid,
        .owner_id = tcb->pid,
        .current_state = tcb->state,
        .requested_state = state,
        .cause = "invalid thread state transition",
        .recovery = "inspect TCB and scheduler Task ownership"};
    ATOMS_Execution_RecordError(&error);
    return ATOMS_EXEC_ERR_INVALID_STATE;
  }
  ATOMS_ThreadState old = tcb->state;
  tcb->state = state;
  ++tcb->diagnostics.state_transitions;
  irq_restore(flags);
  ATOMS_Execution_Trace(THREAD_MODULE_ID, ATOMS_TRACE_THREAD_STATE, tcb->tid,
                        (uint32_t)old, state, 0);
  return ATOMS_EXEC_OK;
}

ATOMS_TCB *ATOMS_Thread_GetByTID(uint32_t tid) {
  uint64_t flags = irq_save();
  ATOMS_TCB *tcb = find_tid_locked(tid);
  irq_restore(flags);
  return tcb;
}

ATOMS_TCB *ATOMS_Thread_GetByTask(const struct Task *task) {
  if (!task)
    return 0;
  uint64_t flags = irq_save();
  for (uint32_t i = 0; i < ATOMS_MAX_THREADS; ++i) {
    if (g_tcb_table[i].state != ATOMS_THREAD_STATE_FREE &&
        g_tcb_table[i].task == task) {
      ATOMS_TCB *result = &g_tcb_table[i];
      irq_restore(flags);
      return result;
    }
  }
  irq_restore(flags);
  return 0;
}

bool ATOMS_Thread_Suspend(uint32_t tid) {
  ATOMS_TCB *tcb = ATOMS_Thread_GetByTID(tid);
  if (!tcb)
    return false;
  if (tcb->task && !scheduler_suspend_task(tcb->task))
    return false;
  return ATOMS_Thread_Transition(tcb, ATOMS_THREAD_STATE_SUSPENDED) ==
         ATOMS_EXEC_OK;
}

bool ATOMS_Thread_Resume(uint32_t tid) {
  ATOMS_TCB *tcb = ATOMS_Thread_GetByTID(tid);
  if (!tcb)
    return false;
  if (tcb->task && !scheduler_resume_task(tcb->task))
    return false;
  return ATOMS_Thread_Transition(tcb, ATOMS_THREAD_STATE_READY) ==
         ATOMS_EXEC_OK;
}

bool ATOMS_Thread_Kill(uint32_t tid, int32_t exit_code) {
  ATOMS_TCB *tcb = ATOMS_Thread_GetByTID(tid);
  if (!tcb)
    return false;
  tcb->kill_pending = true;
  tcb->exit_code = exit_code;
  if (tcb->task)
    scheduler_terminate_task(tcb->task);
  return ATOMS_Thread_Terminate(tid);
}

bool ATOMS_Thread_Terminate(uint32_t tid) {
  uint64_t flags = irq_save();
  ATOMS_TCB *tcb = find_tid_locked(tid);
  if (!tcb) {
    irq_restore(flags);
    return false;
  }
  if (tcb->state != ATOMS_THREAD_STATE_DEAD)
    tcb->state = ATOMS_THREAD_STATE_DEAD;
  tcb->statistics.exit_tick = scheduler_get_tick_count();
  ++g_diag.exits;
  if (g_diag.active)
    --g_diag.active;
  ++g_diag.dead;
  irq_restore(flags);
  ATOMS_Process_UnregisterThread(tcb->pid, tid);
  ATOMS_Execution_Trace(THREAD_MODULE_ID, ATOMS_TRACE_THREAD_EXIT, tid,
                        tcb->pid, ATOMS_THREAD_STATE_DEAD,
                        (uint32_t)tcb->exit_code);
  if (tcb->detached) {
    flags = irq_save();
    zero_bytes(tcb, sizeof(*tcb));
    irq_restore(flags);
  }
  return true;
}

bool ATOMS_Thread_Sleep(uint32_t tid, uint32_t ms) {
  ATOMS_TCB *tcb = ATOMS_Thread_GetByTID(tid);
  if (!tcb || tcb->task != scheduler_current_task())
    return false;
  tcb->sleep_until_ms = timer_get_ticks() + ms;
  if (ATOMS_Thread_Transition(tcb, ATOMS_THREAD_STATE_SLEEPING) !=
      ATOMS_EXEC_OK)
    return false;
  scheduler_sleep(ms);
  return true;
}

bool ATOMS_Thread_Yield(uint32_t tid) {
  ATOMS_TCB *tcb = ATOMS_Thread_GetByTID(tid);
  if (!tcb || tcb->task != scheduler_current_task())
    return false;
  scheduler_yield();
  return true;
}

ATOMS_ExecutionErrorCode ATOMS_Thread_Join(uint32_t tid,
                                           int32_t *out_exit_code) {
  ATOMS_TCB *tcb = ATOMS_Thread_GetByTID(tid);
  if (!tcb)
    return ATOMS_EXEC_ERR_NOT_FOUND;
  if (tcb->detached)
    return ATOMS_EXEC_ERR_BUSY;
  if (tcb->state != ATOMS_THREAD_STATE_DEAD) {
    tcb->joiner_tid =
        scheduler_current_task() ? (uint32_t)scheduler_current_task()->id : 0;
    while (tcb->state != ATOMS_THREAD_STATE_DEAD)
      scheduler_yield();
  }
  if (out_exit_code)
    *out_exit_code = tcb->exit_code;
  uint64_t flags = irq_save();
  zero_bytes(tcb, sizeof(*tcb));
  irq_restore(flags);
  return ATOMS_EXEC_OK;
}

ATOMS_ExecutionErrorCode ATOMS_Thread_Detach(uint32_t tid) {
  uint64_t flags = irq_save();
  ATOMS_TCB *tcb = find_tid_locked(tid);
  if (!tcb) {
    irq_restore(flags);
    return ATOMS_EXEC_ERR_NOT_FOUND;
  }
  tcb->detached = true;
  bool dead = tcb->state == ATOMS_THREAD_STATE_DEAD;
  if (dead)
    zero_bytes(tcb, sizeof(*tcb));
  irq_restore(flags);
  return ATOMS_EXEC_OK;
}

uint32_t ATOMS_Thread_GetCount(void) { return g_diag.active; }

void ATOMS_Thread_AccountCPU(uint32_t tid, bool kernel_mode, uint64_t ticks) {
  ATOMS_TCB *tcb = ATOMS_Thread_GetByTID(tid);
  if (!tcb)
    return;
  if (kernel_mode)
    tcb->statistics.kernel_ticks += ticks;
  else
    tcb->statistics.user_ticks += ticks;
  tcb->statistics.run_count += ticks;
}

void ATOMS_Thread_GetManagerDiagnostics(ATOMS_ThreadManagerDiagnostics *out) {
  if (!out)
    return;
  uint64_t flags = irq_save();
  *out = g_diag;
  irq_restore(flags);
}

void ATOMS_Thread_DumpTelemetry(void) {
  display_print("\n--- ATOMS Thread Diagnostics ---\n");
  display_print("TID PID STATE PRIORITY NAME RUN\n");
  for (uint32_t i = 0; i < ATOMS_MAX_THREADS; ++i) {
    ATOMS_TCB *tcb = &g_tcb_table[i];
    if (tcb->state == ATOMS_THREAD_STATE_FREE)
      continue;
    display_print_dec(tcb->tid);
    display_print(" ");
    display_print_dec(tcb->pid);
    display_print(" ");
    display_print_dec(tcb->state);
    display_print(" ");
    display_print_dec(tcb->scheduling.effective_priority);
    display_print(" ");
    display_print(tcb->name);
    display_print(" ");
    display_print_dec(tcb->statistics.run_count);
    display_print("\n");
  }
  display_print("Active: ");
  display_print_dec(g_diag.active);
  display_print(" Dead: ");
  display_print_dec(g_diag.dead);
  display_print(" Invalid transitions: ");
  display_print_dec(g_diag.invalid_transitions);
  display_print("\n--------------------------------\n");
}
