#include "process_manager.h"
#include "../execution/include/execution_contract.h"
#include "../scheduler/include/scheduler.h"
#include "kernel/drivers/display/display.h"
#include "kernel/wm/bwe/include/bwe.h"

extern void bwe_log(const char *level, const char *msg);
extern uint32_t BOS_CloseSurfacesByPID(uint32_t pid);

#define PROCESS_MODULE_ID 1U

static ATOMS_PCB g_pcb_table[ATOMS_MAX_PROCESSES];
static ATOMS_ProcessManagerDiagnostics g_diag;
static uint32_t g_next_pid = ATOMS_PID_MIN;
static uint32_t g_pid_max = ATOMS_PID_MAX_DEFAULT;
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

static void copy_string(char *destination, const char *source,
                        uint32_t capacity) {
  if (!destination || capacity == 0)
    return;
  if (!source)
    source = "";
  uint32_t i = 0;
  while (i + 1U < capacity && source[i]) {
    destination[i] = source[i];
    ++i;
  }
  destination[i] = '\0';
}

static ATOMS_PCB *find_pid_locked(uint32_t pid) {
  if (pid == ATOMS_INVALID_PID)
    return 0;
  for (uint32_t i = 0; i < ATOMS_MAX_PROCESSES; ++i) {
    if (g_pcb_table[i].pid == pid &&
        g_pcb_table[i].state != ATOMS_PROC_STATE_CLOSED) {
      return &g_pcb_table[i];
    }
  }
  return 0;
}

static uint32_t allocate_pid_locked(void) {
  uint32_t range =
      g_pid_max >= ATOMS_PID_MIN ? g_pid_max - ATOMS_PID_MIN + 1U : 0U;
  for (uint32_t attempts = 0; attempts < range; ++attempts) {
    uint32_t candidate = g_next_pid;
    g_next_pid = candidate >= g_pid_max ? ATOMS_PID_MIN : candidate + 1U;
    if (!find_pid_locked(candidate))
      return candidate;
  }
  return ATOMS_INVALID_PID;
}

static void clear_pcb_locked(ATOMS_PCB *pcb) {
  if (!pcb)
    return;
  uint32_t generation = pcb->generation;
  zero_bytes(pcb, sizeof(*pcb));
  pcb->generation = generation;
  pcb->state = ATOMS_PROC_STATE_CLOSED;
}

static void record_transition_error(ATOMS_PCB *pcb,
                                    ATOMS_ProcessState requested) {
  ATOMS_ExecutionError error = {
      .module = PROCESS_MODULE_ID,
      .code = ATOMS_EXEC_ERR_INVALID_STATE,
      .severity = ATOMS_EXEC_SEVERITY_ERROR,
      .object_id = pcb ? pcb->pid : 0,
      .owner_id = pcb ? pcb->parent_pid : 0,
      .current_state = pcb ? (uint32_t)pcb->state : 0,
      .requested_state = (uint32_t)requested,
      .timestamp = 0,
      .cause = "invalid process state transition",
      .recovery = "inspect process/thread/scheduler state ownership"};
  ATOMS_Execution_RecordError(&error);
}

void ATOMS_ProcessManager_Init(void) {
  uint64_t flags = irq_save();
  zero_bytes(g_pcb_table, sizeof(g_pcb_table));
  zero_bytes(&g_diag, sizeof(g_diag));
  g_diag.capacity = ATOMS_MAX_PROCESSES;
  g_diag.pid_min = ATOMS_PID_MIN;
  g_diag.pid_max = ATOMS_PID_MAX_DEFAULT;
  g_next_pid = ATOMS_PID_MIN;
  g_pid_max = ATOMS_PID_MAX_DEFAULT;
  g_generation = 1U;
  g_initialized = true;
  irq_restore(flags);
  bwe_log("INFO", "ATOMS Process Manager initialized");
}

ATOMS_ExecutionErrorCode
ATOMS_ProcessManager_ConfigurePIDLimit(uint32_t maximum_pid) {
  if (maximum_pid < ATOMS_PID_MIN)
    return ATOMS_EXEC_ERR_INVALID_ARGUMENT;
  uint64_t flags = irq_save();
  if (g_diag.active != 0) {
    irq_restore(flags);
    return ATOMS_EXEC_ERR_BUSY;
  }
  g_pid_max = maximum_pid;
  g_diag.pid_max = maximum_pid;
  g_next_pid = ATOMS_PID_MIN;
  irq_restore(flags);
  return ATOMS_EXEC_OK;
}

uint32_t ATOMS_PID_Alloc(void) {
  if (!g_initialized)
    return ATOMS_INVALID_PID;
  uint64_t flags = irq_save();
  uint32_t pid = allocate_pid_locked();
  irq_restore(flags);
  return pid;
}

void ATOMS_PID_Free(uint32_t pid) { (void)pid; }

ATOMS_PCB *ATOMS_Process_Create(const char *name, const char *filepath,
                                uint32_t parent_pid, uint32_t capabilities) {
  if (!g_initialized || !name)
    return 0;
  uint64_t flags = irq_save();
  ATOMS_PCB *slot = 0;
  for (uint32_t i = 0; i < ATOMS_MAX_PROCESSES; ++i) {
    if (g_pcb_table[i].state == ATOMS_PROC_STATE_CLOSED) {
      slot = &g_pcb_table[i];
      break;
    }
  }
  if (!slot) {
    irq_restore(flags);
    return 0;
  }
  uint32_t pid = allocate_pid_locked();
  if (pid == ATOMS_INVALID_PID) {
    irq_restore(flags);
    return 0;
  }
  zero_bytes(slot, sizeof(*slot));
  slot->pid = pid;
  slot->parent_pid = parent_pid;
  slot->generation = g_generation++;
  copy_string(slot->name, name, sizeof(slot->name));
  copy_string(slot->filepath, filepath, sizeof(slot->filepath));
  copy_string(slot->working_dir, "/", sizeof(slot->working_dir));
  slot->state = ATOMS_PROC_STATE_CREATED;
  slot->base_priority = ATOMS_PROCESS_PRIORITY_NORMAL;
  slot->effective_priority = ATOMS_PROCESS_PRIORITY_NORMAL;
  slot->capabilities_mask = capabilities;
  slot->security.capability_mask = capabilities;
  slot->memory.heap_base = 0x60000000ULL;
  slot->memory.heap_size = 1024U * 1024U;
  slot->memory.stack_base = 0x7FFFF000ULL;
  slot->memory.stack_size = 64U * 1024U;
  slot->heap_base = slot->memory.heap_base;
  slot->heap_size = (uint32_t)slot->memory.heap_size;
  slot->stack_base = slot->memory.stack_base;
  slot->stack_size = (uint32_t)slot->memory.stack_size;
  slot->memory_used_bytes = slot->memory.heap_size + slot->memory.stack_size;
  slot->resources.memory_bytes = slot->memory_used_bytes;
  slot->diagnostics.creation_tick = scheduler_get_tick_count();
  slot->ref_count = 1;
  slot->state = ATOMS_PROC_STATE_READY;
  slot->diagnostics.state_transitions = 1;
  ++g_diag.active;
  ++g_diag.creations;
  if (g_diag.active > g_diag.peak_active)
    g_diag.peak_active = g_diag.active;
  if (parent_pid) {
    ATOMS_PCB *parent = find_pid_locked(parent_pid);
    if (parent)
      ++parent->child_count;
  }
  irq_restore(flags);
  ATOMS_Execution_Trace(PROCESS_MODULE_ID, ATOMS_TRACE_PROCESS_CREATE, pid,
                        parent_pid, ATOMS_PROC_STATE_READY, slot->generation);
  return slot;
}

ATOMS_ExecutionErrorCode ATOMS_Process_Transition(ATOMS_PCB *pcb,
                                                  ATOMS_ProcessState state) {
  if (!pcb)
    return ATOMS_EXEC_ERR_INVALID_ARGUMENT;
  uint64_t flags = irq_save();
  if (!ATOMS_ProcessStateTransitionValid((uint32_t)pcb->state,
                                         (uint32_t)state)) {
    ++pcb->diagnostics.invalid_transitions;
    ++g_diag.invalid_transitions;
    pcb->diagnostics.last_error = ATOMS_EXEC_ERR_INVALID_STATE;
    irq_restore(flags);
    record_transition_error(pcb, state);
    return ATOMS_EXEC_ERR_INVALID_STATE;
  }
  ATOMS_ProcessState old = pcb->state;
  pcb->state = state;
  ++pcb->diagnostics.state_transitions;
  irq_restore(flags);
  ATOMS_Execution_Trace(PROCESS_MODULE_ID, ATOMS_TRACE_PROCESS_STATE, pcb->pid,
                        (uint32_t)old, (uint32_t)state, 0);
  return ATOMS_EXEC_OK;
}

bool ATOMS_Process_Terminate(uint32_t pid, int32_t exit_code) {
  uint32_t parent_pid;
  uint64_t flags = irq_save();
  ATOMS_PCB *pcb = find_pid_locked(pid);
  if (!pcb || pcb->state == ATOMS_PROC_STATE_TERMINATED ||
      pcb->state == ATOMS_PROC_STATE_CLOSED) {
    irq_restore(flags);
    return false;
  }
  parent_pid = pcb->parent_pid;
  pcb->exit_code = exit_code;
  pcb->diagnostics.exit_tick = scheduler_get_tick_count();
  pcb->state = ATOMS_PROC_STATE_TERMINATED;
  ++pcb->diagnostics.state_transitions;
  ++g_diag.exits;
  for (uint32_t i = 0; i < ATOMS_MAX_PROCESSES; ++i) {
    ATOMS_PCB *child = &g_pcb_table[i];
    if (child->state != ATOMS_PROC_STATE_CLOSED && child->parent_pid == pid) {
      child->parent_pid = 1U;
    }
  }
  irq_restore(flags);

  scheduler_terminate_tasks_by_pid(pid);
  BOS_CloseSurfacesByPID(pid);

  flags = irq_save();
  pcb = find_pid_locked(pid);
  if (!pcb) {
    irq_restore(flags);
    return true;
  }
  if (parent_pid > 1U && find_pid_locked(parent_pid)) {
    pcb->state = ATOMS_PROC_STATE_ZOMBIE;
    ++g_diag.zombies;
  } else {
    clear_pcb_locked(pcb);
    if (g_diag.active)
      --g_diag.active;
    ++g_diag.reaps;
  }
  irq_restore(flags);
  ATOMS_Execution_Trace(PROCESS_MODULE_ID, ATOMS_TRACE_PROCESS_EXIT, pid,
                        parent_pid, ATOMS_PROC_STATE_TERMINATED,
                        (uint32_t)exit_code);
  return true;
}

ATOMS_ExecutionErrorCode ATOMS_Process_Reap(uint32_t pid,
                                            int32_t *out_exit_code) {
  uint64_t flags = irq_save();
  ATOMS_PCB *pcb = find_pid_locked(pid);
  if (!pcb) {
    irq_restore(flags);
    return ATOMS_EXEC_ERR_NOT_FOUND;
  }
  if (pcb->state != ATOMS_PROC_STATE_ZOMBIE &&
      pcb->state != ATOMS_PROC_STATE_TERMINATED) {
    irq_restore(flags);
    return ATOMS_EXEC_ERR_INVALID_STATE;
  }
  if (out_exit_code)
    *out_exit_code = pcb->exit_code;
  if (pcb->state == ATOMS_PROC_STATE_ZOMBIE && g_diag.zombies)
    --g_diag.zombies;
  uint32_t parent_pid = pcb->parent_pid;
  clear_pcb_locked(pcb);
  if (g_diag.active)
    --g_diag.active;
  ++g_diag.reaps;
  ATOMS_PCB *parent = find_pid_locked(parent_pid);
  if (parent && parent->child_count)
    --parent->child_count;
  irq_restore(flags);
  return ATOMS_EXEC_OK;
}

ATOMS_PCB *ATOMS_Process_GetByPID(uint32_t pid) {
  uint64_t flags = irq_save();
  ATOMS_PCB *result = find_pid_locked(pid);
  irq_restore(flags);
  return result;
}

ATOMS_PCB *ATOMS_Process_GetByIndex(uint32_t index) {
  if (index >= ATOMS_MAX_PROCESSES)
    return 0;
  return g_pcb_table[index].state == ATOMS_PROC_STATE_CLOSED
             ? 0
             : &g_pcb_table[index];
}

uint32_t ATOMS_Process_GetCount(void) { return g_diag.active; }

uint32_t ATOMS_Process_EnumerateChildren(uint32_t parent_pid,
                                         uint32_t *out_pids,
                                         uint32_t capacity) {
  uint32_t count = 0;
  uint64_t flags = irq_save();
  for (uint32_t i = 0; i < ATOMS_MAX_PROCESSES; ++i) {
    if (g_pcb_table[i].state != ATOMS_PROC_STATE_CLOSED &&
        g_pcb_table[i].parent_pid == parent_pid) {
      if (out_pids && count < capacity)
        out_pids[count] = g_pcb_table[i].pid;
      ++count;
    }
  }
  irq_restore(flags);
  return count;
}

ATOMS_ExecutionErrorCode ATOMS_Process_RegisterThread(uint32_t pid,
                                                      uint32_t tid) {
  uint64_t flags = irq_save();
  ATOMS_PCB *pcb = find_pid_locked(pid);
  if (!pcb) {
    irq_restore(flags);
    return ATOMS_EXEC_ERR_NOT_FOUND;
  }
  for (uint32_t i = 0; i < pcb->thread_count; ++i) {
    if (pcb->thread_ids[i] == tid) {
      irq_restore(flags);
      return ATOMS_EXEC_ERR_ALREADY_EXISTS;
    }
  }
  if (pcb->thread_count >= ATOMS_MAX_THREADS_PER_PROC) {
    irq_restore(flags);
    return ATOMS_EXEC_ERR_TABLE_FULL;
  }
  pcb->thread_ids[pcb->thread_count++] = tid;
  pcb->resources.threads = pcb->thread_count;
  irq_restore(flags);
  return ATOMS_EXEC_OK;
}

ATOMS_ExecutionErrorCode ATOMS_Process_UnregisterThread(uint32_t pid,
                                                        uint32_t tid) {
  uint64_t flags = irq_save();
  ATOMS_PCB *pcb = find_pid_locked(pid);
  if (!pcb) {
    irq_restore(flags);
    return ATOMS_EXEC_ERR_NOT_FOUND;
  }
  for (uint32_t i = 0; i < pcb->thread_count; ++i) {
    if (pcb->thread_ids[i] == tid) {
      for (uint32_t j = i + 1U; j < pcb->thread_count; ++j)
        pcb->thread_ids[j - 1U] = pcb->thread_ids[j];
      pcb->thread_ids[--pcb->thread_count] = 0;
      pcb->resources.threads = pcb->thread_count;
      irq_restore(flags);
      return ATOMS_EXEC_OK;
    }
  }
  irq_restore(flags);
  return ATOMS_EXEC_ERR_NOT_FOUND;
}

void ATOMS_Process_AccountCPU(uint32_t pid, bool kernel_mode, uint64_t ticks) {
  uint64_t flags = irq_save();
  ATOMS_PCB *pcb = find_pid_locked(pid);
  if (pcb) {
    if (kernel_mode)
      pcb->diagnostics.kernel_ticks += ticks;
    else
      pcb->diagnostics.user_ticks += ticks;
    pcb->diagnostics.total_ticks += ticks;
    pcb->cpu_time_ms = pcb->diagnostics.total_ticks;
  }
  irq_restore(flags);
}

void ATOMS_Process_GetManagerDiagnostics(ATOMS_ProcessManagerDiagnostics *out) {
  if (!out)
    return;
  uint64_t flags = irq_save();
  *out = g_diag;
  irq_restore(flags);
}

bool ATOMS_Process_AuditLeaks(uint64_t *out_leaked_resources) {
  uint64_t leaked = 0;
  uint64_t flags = irq_save();
  for (uint32_t i = 0; i < ATOMS_MAX_PROCESSES; ++i) {
    ATOMS_PCB *pcb = &g_pcb_table[i];
    if ((pcb->state == ATOMS_PROC_STATE_ZOMBIE ||
         pcb->state == ATOMS_PROC_STATE_TERMINATED) &&
        (pcb->resources.threads || pcb->resources.handles ||
         pcb->resources.kernel_objects || pcb->resources.file_handles ||
         pcb->resources.ipc_objects)) {
      leaked += pcb->resources.threads + pcb->resources.handles +
                pcb->resources.kernel_objects + pcb->resources.file_handles +
                pcb->resources.ipc_objects;
    }
  }
  g_diag.leaked_resources = leaked;
  irq_restore(flags);
  if (out_leaked_resources)
    *out_leaked_resources = leaked;
  return leaked == 0;
}

int32_t ATOMS_Process_Wait(uint32_t pid, int32_t *out_exit_code) {
  ATOMS_PCB *child = ATOMS_Process_GetByPID(pid);
  if (!child)
    return -1;
  Task *current = scheduler_current_task();
  uint32_t caller_pid = current ? current->owner_pid : 0;
  if (caller_pid == 0 && current)
    caller_pid = (uint32_t)current->id;
  if (caller_pid > 1U && child->parent_pid != caller_pid)
    return -1;
  while (child->state != ATOMS_PROC_STATE_ZOMBIE &&
         child->state != ATOMS_PROC_STATE_TERMINATED &&
         child->state != ATOMS_PROC_STATE_CLOSED) {
    scheduler_yield();
  }
  return ATOMS_Process_Reap(pid, out_exit_code) == ATOMS_EXEC_OK ? (int32_t)pid
                                                                 : -1;
}

ATOMS_ExecutionErrorCode
ATOMS_Process_SetUserImage(uint32_t pid, uint64_t pml4, uint64_t image_base,
                           uint64_t image_end, uint64_t entry_point,
                           uint64_t stack_base, uint64_t stack_size,
                           uint64_t guard_page) {
  if (!pml4 || image_base >= image_end || !entry_point || !stack_base ||
      !stack_size)
    return ATOMS_EXEC_ERR_INVALID_ARGUMENT;
  uint64_t flags = irq_save();
  ATOMS_PCB *pcb = find_pid_locked(pid);
  if (!pcb) {
    irq_restore(flags);
    return ATOMS_EXEC_ERR_NOT_FOUND;
  }
  pcb->pml4_phys = pml4;
  pcb->memory.pml4_phys = pml4;
  pcb->image_base = image_base;
  pcb->image_end = image_end;
  pcb->entry_point = entry_point;
  pcb->stack_base = stack_base;
  pcb->stack_size = (uint32_t)stack_size;
  pcb->memory.stack_base = stack_base;
  pcb->memory.stack_size = stack_size;
  pcb->user_stack_guard = guard_page;
  pcb->memory.committed_bytes = (image_end - image_base) + stack_size;
  pcb->resources.memory_bytes = pcb->memory.committed_bytes;
  irq_restore(flags);
  return ATOMS_EXEC_OK;
}

void ATOMS_Process_RecordUserException(uint32_t pid, uint32_t vector,
                                       uint64_t rip, uint64_t address,
                                       uint64_t error_code) {
  uint64_t flags = irq_save();
  ATOMS_PCB *pcb = find_pid_locked(pid);
  if (pcb) {
    pcb->exception_vector = vector;
    pcb->exception_rip = rip;
    pcb->exception_address = address;
    pcb->exception_error = error_code;
    ++pcb->user_faults;
  }
  irq_restore(flags);
}

static const char *state_name(ATOMS_ProcessState state) {
  switch (state) {
  case ATOMS_PROC_STATE_CREATED:
    return "CREATED";
  case ATOMS_PROC_STATE_READY:
    return "READY";
  case ATOMS_PROC_STATE_RUNNING:
    return "RUNNING";
  case ATOMS_PROC_STATE_WAITING:
    return "WAITING";
  case ATOMS_PROC_STATE_SLEEPING:
    return "SLEEPING";
  case ATOMS_PROC_STATE_SUSPENDED:
    return "SUSPENDED";
  case ATOMS_PROC_STATE_ZOMBIE:
    return "ZOMBIE";
  case ATOMS_PROC_STATE_TERMINATED:
    return "TERMINATED";
  default:
    return "CLOSED";
  }
}

void ATOMS_Process_DumpTelemetry(void) {
  display_print("\n--- ATOMS Process Diagnostics ---\n");
  display_print("PID PPID STATE NAME CPU THREADS MEMORY\n");
  for (uint32_t i = 0; i < ATOMS_MAX_PROCESSES; ++i) {
    ATOMS_PCB *pcb = &g_pcb_table[i];
    if (pcb->state == ATOMS_PROC_STATE_CLOSED)
      continue;
    display_print_dec(pcb->pid);
    display_print(" ");
    display_print_dec(pcb->parent_pid);
    display_print(" ");
    display_print(state_name(pcb->state));
    display_print(" ");
    display_print(pcb->name);
    display_print(" ");
    display_print_dec(pcb->diagnostics.total_ticks);
    display_print(" ");
    display_print_dec(pcb->thread_count);
    display_print(" ");
    display_print_dec(pcb->resources.memory_bytes);
    display_print("\n");
  }
  display_print("Active: ");
  display_print_dec(g_diag.active);
  display_print(" Peak: ");
  display_print_dec(g_diag.peak_active);
  display_print(" Invalid transitions: ");
  display_print_dec(g_diag.invalid_transitions);
  display_print("\n---------------------------------\n");
}
