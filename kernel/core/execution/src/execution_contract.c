#include "../include/execution_contract.h"
#include "../../process/process_manager.h"
#include "../../scheduler/include/scheduler.h"
#include "../../thread/thread_manager.h"
#include <stddef.h>


extern uint64_t timer_get_ticks(void);
extern uint64_t scheduler_get_tick_count(void);
extern uint64_t scheduler_get_context_switch_count(void);

static ATOMS_ExecutionTraceRecord g_trace[ATOMS_EXECUTION_TRACE_CAPACITY];
static volatile uint64_t g_trace_sequence;
static volatile uint32_t g_error_count;
static volatile uint32_t g_warning_count;

static uint64_t irq_save(void) {
  uint64_t flags;
  __asm__ volatile("pushfq; pop %0; cli" : "=r"(flags)::"memory");
  return flags;
}

static void irq_restore(uint64_t flags) {
  __asm__ volatile("push %0; popfq" ::"r"(flags) : "memory", "cc");
}

void ATOMS_Execution_Init(void) {
  uint64_t flags = irq_save();
  for (uint32_t i = 0; i < ATOMS_EXECUTION_TRACE_CAPACITY; ++i) {
    g_trace[i].sequence = 0;
    g_trace[i].timestamp = 0;
    g_trace[i].event = 0;
    g_trace[i].module = 0;
    g_trace[i].object_id = 0;
    g_trace[i].related_id = 0;
    g_trace[i].state = 0;
    g_trace[i].value = 0;
  }
  g_trace_sequence = 0;
  g_error_count = 0;
  g_warning_count = 0;
  irq_restore(flags);
}

void ATOMS_Execution_Trace(uint16_t module, ATOMS_ExecutionTraceEvent event,
                           uint32_t object_id, uint32_t related_id,
                           uint32_t state, uint32_t value) {
  uint64_t flags = irq_save();
  uint64_t sequence = ++g_trace_sequence;
  ATOMS_ExecutionTraceRecord *record =
      &g_trace[(sequence - 1U) % ATOMS_EXECUTION_TRACE_CAPACITY];
  record->timestamp = timer_get_ticks();
  record->event = (uint16_t)event;
  record->module = module;
  record->object_id = object_id;
  record->related_id = related_id;
  record->state = state;
  record->value = value;
  __asm__ volatile("" ::: "memory");
  record->sequence = sequence;
  irq_restore(flags);
}

uint32_t ATOMS_Execution_TraceCopy(ATOMS_ExecutionTraceRecord *out,
                                   uint32_t capacity) {
  if (!out || capacity == 0)
    return 0;
  uint64_t flags = irq_save();
  uint64_t latest = g_trace_sequence;
  uint32_t available = latest < ATOMS_EXECUTION_TRACE_CAPACITY
                           ? (uint32_t)latest
                           : ATOMS_EXECUTION_TRACE_CAPACITY;
  uint32_t count = capacity < available ? capacity : available;
  uint64_t first = latest - count + 1U;
  for (uint32_t i = 0; i < count; ++i) {
    out[i] = g_trace[(first + i - 1U) % ATOMS_EXECUTION_TRACE_CAPACITY];
  }
  irq_restore(flags);
  return count;
}

const ATOMS_ExecutionTraceRecord *ATOMS_Execution_TraceAt(uint32_t index) {
  if (index >= ATOMS_EXECUTION_TRACE_CAPACITY)
    return NULL;
  return &g_trace[index];
}

void ATOMS_Execution_RecordError(const ATOMS_ExecutionError *error) {
  if (!error)
    return;
  if (error->severity >= ATOMS_EXEC_SEVERITY_ERROR) {
    __atomic_add_fetch(&g_error_count, 1U, __ATOMIC_RELAXED);
  } else if (error->severity == ATOMS_EXEC_SEVERITY_WARNING) {
    __atomic_add_fetch(&g_warning_count, 1U, __ATOMIC_RELAXED);
  }
  ATOMS_Execution_Trace(error->module, ATOMS_TRACE_WARNING, error->object_id,
                        error->owner_id, error->current_state, error->code);
}

void ATOMS_Execution_GetDiagnostics(ATOMS_ExecutionDiagnostics *out) {
  if (!out)
    return;
  out->process_capacity = ATOMS_MAX_PROCESSES;
  out->process_active = ATOMS_Process_GetCount();
  out->thread_capacity = ATOMS_MAX_THREADS;
  out->thread_active = ATOMS_Thread_GetCount();
  out->context_switches = scheduler_get_context_switch_count();
  out->scheduler_ticks = scheduler_get_tick_count();
  out->trace_sequence = g_trace_sequence;
  out->error_count = g_error_count;
  out->warning_count = g_warning_count;
}

const char *ATOMS_Execution_ErrorName(ATOMS_ExecutionErrorCode code) {
  switch (code) {
  case ATOMS_EXEC_OK:
    return "OK";
  case ATOMS_EXEC_ERR_INVALID_ARGUMENT:
    return "INVALID_ARGUMENT";
  case ATOMS_EXEC_ERR_NOT_INITIALIZED:
    return "NOT_INITIALIZED";
  case ATOMS_EXEC_ERR_ALREADY_EXISTS:
    return "ALREADY_EXISTS";
  case ATOMS_EXEC_ERR_NOT_FOUND:
    return "NOT_FOUND";
  case ATOMS_EXEC_ERR_TABLE_FULL:
    return "TABLE_FULL";
  case ATOMS_EXEC_ERR_ID_EXHAUSTED:
    return "ID_EXHAUSTED";
  case ATOMS_EXEC_ERR_INVALID_STATE:
    return "INVALID_STATE";
  case ATOMS_EXEC_ERR_OWNERSHIP:
    return "OWNERSHIP";
  case ATOMS_EXEC_ERR_BUSY:
    return "BUSY";
  case ATOMS_EXEC_ERR_PERMISSION:
    return "PERMISSION";
  case ATOMS_EXEC_ERR_RESOURCE:
    return "RESOURCE";
  case ATOMS_EXEC_ERR_CORRUPT:
    return "CORRUPT";
  case ATOMS_EXEC_ERR_UNSUPPORTED:
    return "UNSUPPORTED";
  default:
    return "UNKNOWN";
  }
}

bool ATOMS_ProcessStateTransitionValid(uint32_t current, uint32_t requested) {
  if (current == requested)
    return true;
  switch ((ATOMS_ProcessState)current) {
  case ATOMS_PROC_STATE_CLOSED:
    return requested == ATOMS_PROC_STATE_CREATED;
  case ATOMS_PROC_STATE_CREATED:
    return requested == ATOMS_PROC_STATE_READY ||
           requested == ATOMS_PROC_STATE_TERMINATED;
  case ATOMS_PROC_STATE_READY:
    return requested == ATOMS_PROC_STATE_RUNNING ||
           requested == ATOMS_PROC_STATE_SUSPENDED ||
           requested == ATOMS_PROC_STATE_TERMINATED;
  case ATOMS_PROC_STATE_RUNNING:
    return requested == ATOMS_PROC_STATE_READY ||
           requested == ATOMS_PROC_STATE_WAITING ||
           requested == ATOMS_PROC_STATE_SLEEPING ||
           requested == ATOMS_PROC_STATE_SUSPENDED ||
           requested == ATOMS_PROC_STATE_ZOMBIE ||
           requested == ATOMS_PROC_STATE_TERMINATED;
  case ATOMS_PROC_STATE_WAITING:
  case ATOMS_PROC_STATE_SLEEPING:
    return requested == ATOMS_PROC_STATE_READY ||
           requested == ATOMS_PROC_STATE_SUSPENDED ||
           requested == ATOMS_PROC_STATE_TERMINATED;
  case ATOMS_PROC_STATE_SUSPENDED:
    return requested == ATOMS_PROC_STATE_READY ||
           requested == ATOMS_PROC_STATE_TERMINATED;
  case ATOMS_PROC_STATE_ZOMBIE:
    return requested == ATOMS_PROC_STATE_TERMINATED ||
           requested == ATOMS_PROC_STATE_CLOSED;
  case ATOMS_PROC_STATE_TERMINATED:
    return requested == ATOMS_PROC_STATE_ZOMBIE ||
           requested == ATOMS_PROC_STATE_CLOSED;
  default:
    return false;
  }
}

bool ATOMS_ThreadStateTransitionValid(uint32_t current, uint32_t requested) {
  if (current == requested)
    return true;
  switch ((ATOMS_ThreadState)current) {
  case ATOMS_THREAD_STATE_FREE:
    return requested == ATOMS_THREAD_STATE_CREATED;
  case ATOMS_THREAD_STATE_CREATED:
    return requested == ATOMS_THREAD_STATE_READY ||
           requested == ATOMS_THREAD_STATE_DEAD;
  case ATOMS_THREAD_STATE_READY:
    return requested == ATOMS_THREAD_STATE_RUNNING ||
           requested == ATOMS_THREAD_STATE_SUSPENDED ||
           requested == ATOMS_THREAD_STATE_DEAD;
  case ATOMS_THREAD_STATE_RUNNING:
    return requested == ATOMS_THREAD_STATE_READY ||
           requested == ATOMS_THREAD_STATE_BLOCKED ||
           requested == ATOMS_THREAD_STATE_SLEEPING ||
           requested == ATOMS_THREAD_STATE_WAITING ||
           requested == ATOMS_THREAD_STATE_SUSPENDED ||
           requested == ATOMS_THREAD_STATE_DEAD;
  case ATOMS_THREAD_STATE_BLOCKED:
  case ATOMS_THREAD_STATE_SLEEPING:
  case ATOMS_THREAD_STATE_WAITING:
    return requested == ATOMS_THREAD_STATE_READY ||
           requested == ATOMS_THREAD_STATE_SUSPENDED ||
           requested == ATOMS_THREAD_STATE_DEAD;
  case ATOMS_THREAD_STATE_SUSPENDED:
    return requested == ATOMS_THREAD_STATE_READY ||
           requested == ATOMS_THREAD_STATE_DEAD;
  case ATOMS_THREAD_STATE_DEAD:
    return requested == ATOMS_THREAD_STATE_FREE;
  default:
    return false;
  }
}
