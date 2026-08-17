#include "../include/scheduler.h"
#include "../../../../arch/x86_64/gdt/gdt.h"
#include "../../../../arch/x86_64/smp/smp.h"
#include "../../../drivers/display/display.h"
#include "../../cpu/cpu_state.h"
#include "../../execution/include/execution_contract.h"
#include "../../lib/include/crash_log.h"
#include "../../memory/heap/include/heap.h"
#include "../../memory/vmm/include/vmm.h"
#include "../../process/process_manager.h"
#include "../../thread/thread_manager.h"
#include "../../timer/include/timer.h"
#include "../include/context.h"
#include "../include/runqueue.h"
#include <stddef.h>

#define SCHEDULER_MODULE_ID 3U
#define SCHEDULER_DEFAULT_PRIORITY 16U
#define SCHEDULER_DEFAULT_QUANTUM 5

static RunQueue ready_queue;
static RunQueue waiting_queue;
static RunQueue sleep_queue;
static RunQueue blocked_queue;
static RunQueue terminated_queue;
Task *current_task = NULL;
static Task *idle_task_ptr = NULL;
static uint64_t scheduler_tick_count;
static bool scheduler_running;
static bool scheduler_initialized;
static SchedulerPolicy scheduler_policy = SCHEDULER_POLICY_PRIORITY_AGING;
static SchedulerSwitchReason pending_switch_reason = SCHEDULER_SWITCH_NONE;
static SchedulerDiagnostics scheduler_diag;

extern uint64_t task_generate_id(void);
volatile uint64_t g_context_switches;

static uint64_t irq_save(void) {
  uint64_t flags;
  __asm__ volatile("pushfq; pop %0; cli" : "=r"(flags)::"memory");
  return flags;
}

static void irq_restore(uint64_t flags) {
  __asm__ volatile("push %0; popfq" : : "r"(flags) : "memory", "cc");
}

static void zero_bytes(void *memory, uint64_t size) {
  uint8_t *bytes = (uint8_t *)memory;
  for (uint64_t i = 0; i < size; ++i)
    bytes[i] = 0;
}

static int32_t quantum_for_priority(uint8_t priority) {
  return 1 + (int32_t)(priority / 4U);
}

static RunQueue *queue_for_state(TaskState state) {
  switch (state) {
  case TASK_READY:
    return &ready_queue;
  case TASK_WAITING:
    return &waiting_queue;
  case TASK_SLEEPING:
    return &sleep_queue;
  case TASK_BLOCKED:
    return &blocked_queue;
  case TASK_TERMINATED:
    return &terminated_queue;
  default:
    return NULL;
  }
}

static void record_scheduler_error(Task *task, ATOMS_ExecutionErrorCode code,
                                   const char *cause) {
  ATOMS_ExecutionError error = {
      .module = SCHEDULER_MODULE_ID,
      .code = code,
      .severity = ATOMS_EXEC_SEVERITY_ERROR,
      .object_id = task ? (uint32_t)task->id : 0,
      .owner_id = task ? task->owner_pid : 0,
      .current_state = task ? task->state : TASK_INVALID,
      .requested_state = TASK_INVALID,
      .timestamp = scheduler_tick_count,
      .cause = cause,
      .recovery = "run scheduler consistency and queue diagnostics"};
  ATOMS_Execution_RecordError(&error);
}

static bool transition_task(Task *task, TaskState state) {
  if (!task || !task_transition(task, state)) {
    ++scheduler_diag.illegal_transitions;
    record_scheduler_error(task, ATOMS_EXEC_ERR_INVALID_STATE,
                           "scheduler task transition rejected");
    return false;
  }
  ATOMS_TCB *tcb = ATOMS_Thread_GetByTask(task);
  if (tcb) {
    ATOMS_ThreadState thread_state = ATOMS_THREAD_STATE_READY;
    switch (state) {
    case TASK_RUNNING:
      thread_state = ATOMS_THREAD_STATE_RUNNING;
      break;
    case TASK_WAITING:
      thread_state = ATOMS_THREAD_STATE_WAITING;
      break;
    case TASK_SLEEPING:
      thread_state = ATOMS_THREAD_STATE_SLEEPING;
      break;
    case TASK_BLOCKED:
      thread_state = ATOMS_THREAD_STATE_BLOCKED;
      break;
    case TASK_TERMINATED:
      thread_state = ATOMS_THREAD_STATE_DEAD;
      break;
    default:
      break;
    }
    if (tcb->state != thread_state)
      (void)ATOMS_Thread_Transition(tcb, thread_state);
  }
  return true;
}

static bool enqueue_task(RunQueue *queue, Task *task) {
  RunQueueResult result = runqueue_push_checked(queue, task);
  if (result == RUNQUEUE_OK)
    return true;
  if (result == RUNQUEUE_ERR_DUPLICATE || result == RUNQUEUE_ERR_WRONG_QUEUE)
    ++scheduler_diag.duplicate_rejections;
  if (result == RUNQUEUE_ERR_CORRUPT)
    ++scheduler_diag.queue_corruptions;
  record_scheduler_error(task,
                         result == RUNQUEUE_ERR_CORRUPT ? ATOMS_EXEC_ERR_CORRUPT
                                                        : ATOMS_EXEC_ERR_BUSY,
                         "scheduler queue insertion rejected");
  return false;
}

static bool unlink_task(Task *task) {
  if (!task)
    return false;
  if (task->queue_class == TASK_QUEUE_NONE)
    return true;
  RunQueue *queue = NULL;
  switch ((TaskQueueClass)task->queue_class) {
  case TASK_QUEUE_READY:
    queue = &ready_queue;
    break;
  case TASK_QUEUE_WAITING:
    queue = &waiting_queue;
    break;
  case TASK_QUEUE_SLEEPING:
    queue = &sleep_queue;
    break;
  case TASK_QUEUE_BLOCKED:
    queue = &blocked_queue;
    break;
  case TASK_QUEUE_TERMINATED:
    queue = &terminated_queue;
    break;
  default:
    ++scheduler_diag.queue_corruptions;
    return false;
  }
  if (runqueue_remove_checked(queue, task) != RUNQUEUE_OK) {
    ++scheduler_diag.queue_corruptions;
    record_scheduler_error(task, ATOMS_EXEC_ERR_CORRUPT,
                           "task queue membership is inconsistent");
    return false;
  }
  return true;
}

static void initialize_task_defaults(Task *task, const char *name,
                                     uint8_t priority) {
  zero_bytes(task, sizeof(*task));
  task->id = task_generate_id();
  task->name = name ? name : "Task";
  task->state = TASK_NEW;
  task->queue_class = TASK_QUEUE_NONE;
  task->base_priority = priority;
  task->effective_priority = priority;
  task->default_quantum = quantum_for_priority(priority);
  task->quantum = task->default_quantum;
  task->affinity_mask = UINT64_MAX;
  task->assigned_cpu = ATOMS_CPU_NONE;
  task->last_cpu = ATOMS_CPU_NONE;
  task->ready_since_tick = scheduler_tick_count;
  task->pml4 = vmm_get_active_pml4();
  list_node_init(&task->queue_node);
}

static void account_current_tick(void) {
  if (!current_task)
    return;
  if (current_task == idle_task_ptr) {
    ++scheduler_diag.idle_ticks;
    return;
  }
  ++scheduler_diag.busy_ticks;
  ++current_task->total_run_ticks;
  if (current_task->total_run_ticks > scheduler_diag.longest_runtime_ticks) {
    scheduler_diag.longest_runtime_ticks = current_task->total_run_ticks;
    scheduler_diag.longest_runtime_task_id = current_task->id;
  }
  ATOMS_TCB *tcb = ATOMS_Thread_GetByTask(current_task);
  if (tcb)
    ATOMS_Thread_AccountCPU(tcb->tid, !current_task->is_user_task, 1);
  if (current_task->owner_pid)
    ATOMS_Process_AccountCPU(current_task->owner_pid,
                             !current_task->is_user_task, 1);
}

static void age_ready_tasks(void) {
  list_node_t *node = ready_queue.ready_list.head;
  while (node) {
    Task *task = LIST_ENTRY(node, Task, queue_node);
    ++task->total_wait_ticks;
    ++scheduler_diag.total_wait_ticks;
    uint64_t waited = scheduler_tick_count - task->ready_since_tick;
    uint8_t boost = (uint8_t)(waited / SCHEDULER_AGING_INTERVAL_TICKS);
    uint8_t maximum_boost = SCHEDULER_PRIORITY_MAX - task->base_priority;
    if (boost > maximum_boost)
      boost = maximum_boost;
    uint8_t effective = task->base_priority + boost;
    if (effective != task->effective_priority) {
      if (effective > task->effective_priority) {
        uint8_t difference = effective - task->effective_priority;
        task->starvation_boosts += difference;
        scheduler_diag.starvation_boosts += difference;
      }
      task->effective_priority = effective;
    }
    node = node->next;
  }
}

static bool task_eligible_on_cpu(const Task *task, uint32_t cpu) {
  if (!task || cpu >= 64U || !(task->affinity_mask & (1ULL << cpu)))
    return false;
  return task->assigned_cpu == ATOMS_CPU_NONE || task->assigned_cpu == cpu;
}

static Task *select_next_task(void) {
  if (runqueue_is_empty(&ready_queue))
    return idle_task_ptr;
  uint32_t cpu = atoms_cpu_id();

  list_node_t *node = ready_queue.ready_list.head;
  Task *best = NULL;
  while (node) {
    Task *candidate = LIST_ENTRY(node, Task, queue_node);
    if (!task_eligible_on_cpu(candidate, cpu)) {
      node = node->next;
      continue;
    }
    if (scheduler_policy == SCHEDULER_POLICY_ROUND_ROBIN) {
      best = candidate;
      break;
    }
    if (!best || candidate->effective_priority > best->effective_priority ||
        (candidate->effective_priority == best->effective_priority &&
         candidate->ready_since_tick < best->ready_since_tick) ||
        (candidate->effective_priority == best->effective_priority &&
         candidate->ready_since_tick == best->ready_since_tick &&
         candidate->id < best->id))
      best = candidate;
    node = node->next;
  }
  if (best)
    runqueue_remove(&ready_queue, best);
  return best ? best : idle_task_ptr;
}

static uint64_t g_last_switch_old_id = 0;
static uint64_t g_last_switch_new_id = 0;
static SchedulerSwitchReason g_last_switch_reason = SCHEDULER_SWITCH_QUANTUM;

static void note_switch(Task *old_task, Task *new_task,
                        SchedulerSwitchReason reason) {
  g_last_switch_old_id = old_task ? old_task->id : 0;
  g_last_switch_new_id = new_task ? new_task->id : 0;
  g_last_switch_reason = reason;
  ++g_context_switches;
  scheduler_diag.context_switches = g_context_switches;
  bool voluntary =
      reason == SCHEDULER_SWITCH_YIELD || reason == SCHEDULER_SWITCH_BLOCK ||
      reason == SCHEDULER_SWITCH_SLEEP || reason == SCHEDULER_SWITCH_WAIT ||
      reason == SCHEDULER_SWITCH_EXIT;
  if (voluntary) {
    ++scheduler_diag.voluntary_switches;
    if (old_task)
      ++old_task->voluntary_switches;
  } else {
    ++scheduler_diag.involuntary_switches;
    if (old_task)
      ++old_task->involuntary_switches;
  }
  if (new_task) {
    ++new_task->dispatch_count;
    ++scheduler_diag.dispatched_tasks;
  }
  ATOMS_Execution_Trace(SCHEDULER_MODULE_ID, ATOMS_TRACE_CONTEXT_SWITCH,
                        old_task ? (uint32_t)old_task->id : 0,
                        new_task ? (uint32_t)new_task->id : 0,
                        new_task ? new_task->state : TASK_INVALID, reason);
}

static void idle_task(void) {
  while (1) {
    while (!runqueue_is_empty(&terminated_queue)) {
      uint64_t flags = irq_save();
      Task *task = runqueue_pop(&terminated_queue);
      irq_restore(flags);
      if (task && task != current_task) {
        if (task->stack)
          kfree(task->stack);
        if (task->user_stack)
          kfree(task->user_stack);
        cpu_extended_state_free_task(task);
        kfree(task);
      }
    }
    __asm__ volatile("sti; hlt" : : : "memory");
  }
}

void scheduler_init(void) {
  uint64_t flags = irq_save();
  zero_bytes(&scheduler_diag, sizeof(scheduler_diag));
  scheduler_tick_count = 0;
  g_context_switches = 0;
  scheduler_running = false;
  scheduler_policy = SCHEDULER_POLICY_PRIORITY_AGING;
  pending_switch_reason = SCHEDULER_SWITCH_NONE;
  runqueue_init_class(&ready_queue, TASK_QUEUE_READY);
  runqueue_init_class(&waiting_queue, TASK_QUEUE_WAITING);
  runqueue_init_class(&sleep_queue, TASK_QUEUE_SLEEPING);
  runqueue_init_class(&blocked_queue, TASK_QUEUE_BLOCKED);
  runqueue_init_class(&terminated_queue, TASK_QUEUE_TERMINATED);

  idle_task_ptr = (Task *)kmalloc(sizeof(Task));
  if (!idle_task_ptr) {
    irq_restore(flags);
    display_print("[SCHED] PANIC: idle task allocation failed\n");
    __asm__ volatile("cli");
    while (1)
      __asm__ volatile("hlt");
  }
  initialize_task_defaults(idle_task_ptr, "Idle", SCHEDULER_PRIORITY_MIN);
  idle_task_ptr->state = TASK_READY;
  idle_task_ptr->default_quantum = 1;
  idle_task_ptr->quantum = 1;
  idle_task_ptr->stack = kmalloc(KERNEL_TASK_STACK_SIZE);
  if (!idle_task_ptr->stack) {
    irq_restore(flags);
    display_print("[SCHED] PANIC: idle stack allocation failed\n");
    __asm__ volatile("cli");
    while (1)
      __asm__ volatile("hlt");
  }
  cpu_extended_state_init_task(idle_task_ptr);
  context_prepare_kernel_task(idle_task_ptr, idle_task);
  idle_task_ptr->assigned_cpu = atoms_cpu_id();
  atoms_cpu_bind_idle(idle_task_ptr);
  current_task = NULL;
  atoms_cpu_bind_current(NULL);
  scheduler_initialized = true;
  scheduler_diag.policy = scheduler_policy;
  scheduler_diag.queues_consistent = true;
  irq_restore(flags);
  display_print("\n[SCHED] Production scheduler initialized\n");
}

void scheduler_register_boot_task(void) {
  if (!scheduler_initialized || current_task)
    return;
  Task *boot_task = (Task *)kmalloc(sizeof(Task));
  if (!boot_task)
    return;
  initialize_task_defaults(boot_task, "GUI_System", SCHEDULER_DEFAULT_PRIORITY);
  boot_task->state = TASK_RUNNING;
  boot_task->state_transitions = 1;
  boot_task->stack = kmalloc(KERNEL_TASK_STACK_SIZE);
  if (!boot_task->stack) {
    kfree(boot_task);
    return;
  }
  cpu_extended_state_init_task(boot_task);
  boot_task->assigned_cpu = atoms_cpu_id();
  boot_task->last_cpu = atoms_cpu_id();
  current_task = boot_task;
  atoms_cpu_bind_current(boot_task);
  atoms_cpu_set_scheduler_enabled(true);
  scheduler_running = true;
  scheduler_diag.running = true;
  display_print("[SCHED] Boot execution registered as task ");
  display_print_dec(boot_task->id);
  display_print("\n");
}

bool scheduler_submit_task(Task *task) {
  if (!scheduler_initialized || !task || task == idle_task_ptr ||
      task->state == TASK_TERMINATED || task->state == TASK_INVALID)
    return false;
  uint64_t flags = irq_save();
  if (task->queue_class != TASK_QUEUE_NONE) {
    ++scheduler_diag.duplicate_rejections;
    irq_restore(flags);
    return false;
  }
  if (task->state == TASK_NEW) {
    if (!transition_task(task, TASK_READY)) {
      irq_restore(flags);
      return false;
    }
  } else if (task->state != TASK_READY) {
    ++scheduler_diag.illegal_transitions;
    irq_restore(flags);
    return false;
  }
  task->ready_since_tick = scheduler_tick_count;
  task->effective_priority = task->base_priority;
  bool result = enqueue_task(&ready_queue, task);
  irq_restore(flags);
  return result;
}

void scheduler_add_task(Task *task) { (void)scheduler_submit_task(task); }

static bool park_task(Task *task, TaskState state,
                      SchedulerSwitchReason reason) {
  if (!task || task == idle_task_ptr || task->state == TASK_TERMINATED)
    return false;
  uint64_t flags = irq_save();
  bool was_current = task == current_task;
  if (!was_current && !unlink_task(task)) {
    irq_restore(flags);
    return false;
  }
  if (!transition_task(task, state)) {
    irq_restore(flags);
    return false;
  }
  RunQueue *queue = queue_for_state(state);
  bool result = queue && enqueue_task(queue, task);
  if (was_current) {
    task->quantum = 0;
    pending_switch_reason = reason;
  }
  irq_restore(flags);
  return result;
}

bool scheduler_wait_task(Task *task) {
  return park_task(task, TASK_WAITING, SCHEDULER_SWITCH_WAIT);
}

bool scheduler_block_task(Task *task) {
  return park_task(task, TASK_BLOCKED, SCHEDULER_SWITCH_BLOCK);
}

bool scheduler_suspend_task(Task *task) { return scheduler_block_task(task); }

bool scheduler_resume_task(Task *task) {
  if (!task || task == idle_task_ptr ||
      (task->state != TASK_BLOCKED && task->state != TASK_WAITING))
    return false;
  uint64_t flags = irq_save();
  if (!unlink_task(task) || !transition_task(task, TASK_READY)) {
    irq_restore(flags);
    return false;
  }
  task->ready_since_tick = scheduler_tick_count;
  task->effective_priority = task->base_priority;
  bool result = enqueue_task(&ready_queue, task);
  if (result)
    ++scheduler_diag.wakeups;
  irq_restore(flags);
  return result;
}

void scheduler_terminate_task(Task *task) {
  if (!task || task == idle_task_ptr || task->state == TASK_TERMINATED)
    return;
  uint64_t flags = irq_save();
  bool was_current = task == current_task;
  if (!was_current && !unlink_task(task)) {
    irq_restore(flags);
    return;
  }
  if (!transition_task(task, TASK_TERMINATED)) {
    irq_restore(flags);
    return;
  }
  if (!enqueue_task(&terminated_queue, task)) {
    irq_restore(flags);
    return;
  }
  if (was_current) {
    task->quantum = 0;
    pending_switch_reason = SCHEDULER_SWITCH_EXIT;
  }
  crash_log_add("[TASK EXIT] scheduler retired task");
  irq_restore(flags);
}

Task *scheduler_create_kernel_task(const char *name, void (*entry)(void), uint8_t priority) {
  if (!entry)
    return NULL;
  Task *task = (Task *)kmalloc(sizeof(Task));
  if (!task)
    return NULL;
  initialize_task_defaults(task, name, priority > 0 ? priority : SCHEDULER_DEFAULT_PRIORITY);
  task->stack = kmalloc(KERNEL_TASK_STACK_SIZE);
  if (!task->stack) {
    kfree(task);
    return NULL;
  }
  cpu_extended_state_init_task(task);
  context_prepare_kernel_task(task, entry);
  if (!scheduler_submit_task(task)) {
    cpu_extended_state_free_task(task);
    kfree(task->stack);
    kfree(task);
    return NULL;
  }
  crash_log_add("[SPAWN] kernel scheduler task");
  return task;
}

Task *scheduler_create_user_task(const char *name, void (*entry)(void)) {
  if (!entry)
    return NULL;
  Task *task = (Task *)kmalloc(sizeof(Task));
  if (!task)
    return NULL;
  initialize_task_defaults(task, name, SCHEDULER_DEFAULT_PRIORITY);
  task->is_user_task = 1;
  task->stack = kmalloc(KERNEL_TASK_STACK_SIZE);
  task->user_stack = kmalloc(KERNEL_TASK_STACK_SIZE);
  if (!task->stack || !task->user_stack) {
    if (task->stack)
      kfree(task->stack);
    if (task->user_stack)
      kfree(task->user_stack);
    kfree(task);
    return NULL;
  }

  extern void enter_usermode(uint64_t entry_point, uint64_t user_stack);
  uint64_t *stack_ptr =
      (uint64_t *)((uint8_t *)task->stack + KERNEL_TASK_STACK_SIZE);
  *(--stack_ptr) = 0x10;
  *(--stack_ptr) = (uint64_t)task->stack + KERNEL_TASK_STACK_SIZE;
  *(--stack_ptr) = 0x202;
  *(--stack_ptr) = 0x08;
  *(--stack_ptr) = (uint64_t)enter_usermode;
  *(--stack_ptr) = 0;
  *(--stack_ptr) = 0;
  *(--stack_ptr) = 0;
  *(--stack_ptr) = 0;
  *(--stack_ptr) = 0;
  *(--stack_ptr) = 0;
  *(--stack_ptr) = (uint64_t)task->user_stack + KERNEL_TASK_STACK_SIZE;
  *(--stack_ptr) = (uint64_t)entry;
  *(--stack_ptr) = 0;
  *(--stack_ptr) = 0;
  *(--stack_ptr) = 0;
  *(--stack_ptr) = 0;
  *(--stack_ptr) = 0;
  *(--stack_ptr) = 0;
  *(--stack_ptr) = 0;
  *(--stack_ptr) = 0;
  *(--stack_ptr) = 0;
  task->rsp = (uint64_t)stack_ptr;
  task->rip = (uint64_t)entry;
  cpu_extended_state_init_task(task);
  if (!scheduler_submit_task(task)) {
    cpu_extended_state_free_task(task);
    kfree(task->user_stack);
    kfree(task->stack);
    kfree(task);
    return NULL;
  }
  crash_log_add("[SPAWN] user scheduler task");
  return task;
}

Task *scheduler_current_task(void) {
  if (!scheduler_running || !scheduler_initialized) {
    return NULL;
  }
  ATOMS_PerCPU *cpu = atoms_cpu_local();
  if (cpu && cpu->current_task && (uint64_t)cpu->current_task >= 0x100000ULL && (uint64_t)cpu->current_task < 0xFFFFFFFF00000000ULL) {
    return cpu->current_task;
  }
  if (current_task && (uint64_t)current_task >= 0x100000ULL && (uint64_t)current_task < 0xFFFFFFFF00000000ULL) {
    return current_task;
  }
  return NULL;
}

bool scheduler_set_task_affinity(Task *task, uint64_t affinity_mask) {
  if (!task || affinity_mask == 0)
    return false;
  uint64_t supported =
      atoms_smp_topology()->discovered_count >= 64U
          ? UINT64_MAX
          : ((1ULL << atoms_smp_topology()->discovered_count) - 1ULL);
  affinity_mask &= supported;
  if (!affinity_mask)
    return false;
  task->affinity_mask = affinity_mask;
  if (task->assigned_cpu != ATOMS_CPU_NONE &&
      !(affinity_mask & (1ULL << task->assigned_cpu)))
    task->assigned_cpu = ATOMS_CPU_NONE;
  return true;
}

bool scheduler_request_migration(Task *task, uint32_t target_cpu) {
  ATOMS_PerCPU *target = atoms_cpu_by_id(target_cpu);
  if (!task || !target || target->state != ATOMS_CPU_ONLINE ||
      !(task->affinity_mask & (1ULL << target_cpu)) ||
      task == scheduler_current_task())
    return false;
  uint32_t old_cpu = task->assigned_cpu;
  task->assigned_cpu = target_cpu;
  ++task->migration_count;
  ++target->migrations_in;
  if (old_cpu != ATOMS_CPU_NONE) {
    ATOMS_PerCPU *old = atoms_cpu_by_id(old_cpu);
    if (old)
      ++old->migrations_out;
  }
  if (target_cpu != atoms_cpu_id())
    (void)atoms_ipi_send(target_cpu, ATOMS_IPI_RESCHEDULE);
  return true;
}

void scheduler_sleep(uint64_t ticks) {
  Task *sleeping = current_task;
  if (!sleeping || sleeping == idle_task_ptr)
    return;
  sleeping->wake_tick = timer_get_ticks() + (ticks ? ticks : 1U);
  if (!park_task(sleeping, TASK_SLEEPING, SCHEDULER_SWITCH_SLEEP))
    return;
  while (sleeping->state == TASK_SLEEPING)
    __asm__ volatile("sti; hlt" : : : "memory");
}

void scheduler_yield(void) {
  extern void BRE_DispatchPending(void);
  BRE_DispatchPending();
  if (!current_task || current_task == idle_task_ptr)
    return;
  current_task->quantum = 0;
  pending_switch_reason = SCHEDULER_SWITCH_YIELD;
  do {
    __asm__ volatile("sti; hlt" : : : "memory");
  } while (current_task && current_task->quantum == 0 &&
           current_task->state == TASK_RUNNING);
}

static void wake_expired_sleepers(uint64_t now) {
  list_node_t *node = sleep_queue.ready_list.head;
  while (node) {
    list_node_t *next = node->next;
    Task *task = LIST_ENTRY(node, Task, queue_node);
    if (now >= task->wake_tick) {
      runqueue_remove(&sleep_queue, task);
      if (transition_task(task, TASK_READY)) {
        task->ready_since_tick = scheduler_tick_count;
        task->effective_priority = task->base_priority;
        if (enqueue_task(&ready_queue, task))
          ++scheduler_diag.wakeups;
      }
    }
    node = next;
  }
}

void scheduler_on_tick(void) {
  if (!scheduler_running)
    return;
  ++scheduler_tick_count;
  scheduler_diag.tick_count = scheduler_tick_count;

  extern volatile uint64_t g_scheduler_ticks;
  g_scheduler_ticks++;

  extern void xhci_poll(void);
  xhci_poll();
  account_current_tick();
  wake_expired_sleepers(timer_get_ticks());
  age_ready_tasks();

  if (current_task && current_task != idle_task_ptr &&
      current_task->state == TASK_RUNNING && current_task->quantum > 0)
    --current_task->quantum;

  bool must_switch = !current_task || current_task == idle_task_ptr ||
                     current_task->state != TASK_RUNNING ||
                     current_task->quantum <= 0;
  if (!must_switch && scheduler_policy == SCHEDULER_POLICY_PRIORITY_AGING &&
      !runqueue_is_empty(&ready_queue)) {
    Task *candidate = select_next_task();
    if (candidate && candidate != idle_task_ptr) {
      if (candidate->effective_priority > current_task->effective_priority) {
        enqueue_task(&ready_queue, candidate);
        pending_switch_reason = SCHEDULER_SWITCH_WAKE_PREEMPT;
        must_switch = true;
      } else {
        enqueue_task(&ready_queue, candidate);
      }
    }
  }
  if (!must_switch)
    return;

  Task *old_task = current_task;
  SchedulerSwitchReason reason = pending_switch_reason;
  if (reason == SCHEDULER_SWITCH_NONE)
    reason = SCHEDULER_SWITCH_QUANTUM;
  pending_switch_reason = SCHEDULER_SWITCH_NONE;

  if (old_task && old_task != idle_task_ptr &&
      old_task->state == TASK_RUNNING) {
    if (transition_task(old_task, TASK_READY)) {
      old_task->ready_since_tick = scheduler_tick_count;
      old_task->effective_priority = old_task->base_priority;
      enqueue_task(&ready_queue, old_task);
    }
  } else if (old_task == idle_task_ptr && old_task->state == TASK_RUNNING) {
    transition_task(old_task, TASK_READY);
  }

  Task *new_task = select_next_task();
  if (!new_task)
    new_task = idle_task_ptr;
  if (new_task == old_task) {
    if (new_task->state != TASK_RUNNING)
      transition_task(new_task, TASK_RUNNING);
    new_task->quantum = new_task->default_quantum;
    return;
  }

  cpu_extended_state_save(old_task);
  cpu_extended_state_restore(new_task);
  if (new_task->state != TASK_RUNNING &&
      !transition_task(new_task, TASK_RUNNING))
    return;
  new_task->quantum = new_task->default_quantum;
  new_task->last_run_tick = scheduler_tick_count;
  new_task->last_cpu = atoms_cpu_id();
  current_task = new_task;
  atoms_cpu_bind_current(new_task);
  ++atoms_cpu_local()->context_switches;
  note_switch(old_task, new_task, reason);
  tss_set_kernel_stack((uint64_t)new_task->stack + KERNEL_TASK_STACK_SIZE);
  if (old_task && old_task->pml4 != new_task->pml4)
    vmm_switch_address_space(new_task->pml4);

  extern void diag_set_sched_telemetry(
      uint64_t ticks, uint64_t switches, uint32_t ready, uint32_t sleeping,
      uint32_t blocked, uint32_t waiting, uint32_t terminated,
      const char *policy, const char *task_name, uint64_t task_id,
      uint8_t prio, int32_t quantum, const char *status, uint64_t old_id,
      uint64_t new_id, const char *reason);

  const char *reason_str = "QUANTUM";
  if (reason == SCHEDULER_SWITCH_YIELD) reason_str = "YIELD";
  else if (reason == SCHEDULER_SWITCH_SLEEP) reason_str = "SLEEP";
  else if (reason == SCHEDULER_SWITCH_BLOCK) reason_str = "BLOCK";
  else if (reason == SCHEDULER_SWITCH_WAKE_PREEMPT) reason_str = "PREEMPT";
  else if (reason == SCHEDULER_SWITCH_EXIT) reason_str = "EXIT";

  diag_set_sched_telemetry(
      scheduler_tick_count, g_context_switches,
      runqueue_get_size(&ready_queue), runqueue_get_size(&sleep_queue),
      runqueue_get_size(&blocked_queue), runqueue_get_size(&waiting_queue),
      runqueue_get_size(&terminated_queue),
      scheduler_policy == SCHEDULER_POLICY_PRIORITY_AGING ? "AGING" : "RR",
      new_task->name ? new_task->name : "unnamed",
      new_task->id, new_task->effective_priority, new_task->quantum,
      "ACTIVE", old_task ? old_task->id : 0, new_task->id, reason_str);
}

void scheduler_tick(void) { scheduler_on_tick(); }

void scheduler_start(void) {
  if (!scheduler_initialized || scheduler_running)
    return;
  __asm__ volatile("cli");
  Task *first_task = select_next_task();
  if (!first_task)
    first_task = idle_task_ptr;
  if (!transition_task(first_task, TASK_RUNNING))
    return;
  first_task->quantum = first_task->default_quantum;
  first_task->last_cpu = atoms_cpu_id();
  current_task = first_task;
  atoms_cpu_bind_current(first_task);
  atoms_cpu_set_scheduler_enabled(true);
  scheduler_running = true;
  scheduler_diag.running = true;
  tss_set_kernel_stack((uint64_t)first_task->stack + KERNEL_TASK_STACK_SIZE);
  vmm_switch_address_space(first_task->pml4);
  cpu_extended_state_restore(first_task);

  extern void com1_puts(const char *s);
  void (*put_hex)(uint64_t) = NULL; (void)put_hex;

  if (first_task->is_user_task) {
    char hex[] = "0123456789ABCDEF";
    com1_puts("\r\n========================================\r\n");
    com1_puts("[RING3] PROCESS SELECTED: "); com1_puts(first_task->name ? first_task->name : "UserTask"); com1_puts("\r\n");
    com1_puts("[RING3] PID: ");
    { char buf[16]; int p = 14; buf[15] = '\0'; uint64_t v = first_task->id; if (v == 0) com1_puts("0"); else { while (v > 0) { buf[p--] = '0' + (v % 10); v /= 10; } com1_puts(&buf[p + 1]); } }
    com1_puts("\r\n[RING3] CR3: 0x");
    for (int i = 60; i >= 0; i -= 4) { char c[2] = { hex[(((uint64_t)first_task->pml4) >> i) & 0xF], '\0' }; com1_puts(c); }
    com1_puts("\r\n[RING3] USER_RIP: 0x");
    for (int i = 60; i >= 0; i -= 4) { char c[2] = { hex[(first_task->rip >> i) & 0xF], '\0' }; com1_puts(c); }
    com1_puts("\r\n[RING3] USER_RSP: 0x");
    for (int i = 60; i >= 0; i -= 4) { char c[2] = { hex[(first_task->rsp >> i) & 0xF], '\0' }; com1_puts(c); }
    com1_puts("\r\n[RING3] CS: 0x23\r\n");
    com1_puts("[RING3] SS: 0x1B\r\n");
    com1_puts("[RING3] CPL: 3\r\n");
    com1_puts("[RING3] ENTERING_USERMODE\r\n");
    com1_puts("========================================\r\n");
  }

  context_switch_first(first_task);
}

uint32_t scheduler_get_task_count(void) {
  return runqueue_get_size(&ready_queue) + runqueue_get_size(&waiting_queue) +
         runqueue_get_size(&sleep_queue) + runqueue_get_size(&blocked_queue) +
         (current_task && current_task != idle_task_ptr ? 1U : 0U);
}

bool scheduler_set_task_priority(Task *task, uint8_t priority) {
  if (!task || priority > SCHEDULER_PRIORITY_MAX || task == idle_task_ptr)
    return false;
  uint64_t flags = irq_save();
  task->base_priority = priority;
  task->effective_priority = priority;
  task->default_quantum = quantum_for_priority(priority);
  if (task->quantum > task->default_quantum)
    task->quantum = task->default_quantum;
  ATOMS_TCB *tcb = ATOMS_Thread_GetByTask(task);
  if (tcb) {
    tcb->priority = priority;
    tcb->scheduling.base_priority = priority;
    tcb->scheduling.effective_priority = priority;
    tcb->scheduling.quantum_ticks = (uint32_t)task->default_quantum;
  }
  irq_restore(flags);
  return true;
}

uint8_t scheduler_get_task_priority(const Task *task) {
  return task ? task->effective_priority : 0;
}

bool scheduler_set_policy(SchedulerPolicy policy) {
  if (policy != SCHEDULER_POLICY_ROUND_ROBIN &&
      policy != SCHEDULER_POLICY_PRIORITY_AGING)
    return false;
  scheduler_policy = policy;
  scheduler_diag.policy = policy;
  return true;
}

SchedulerPolicy scheduler_get_policy(void) { return scheduler_policy; }

void scheduler_wake_task(Task *task) {
  if (!task || task == idle_task_ptr || task->state == TASK_TERMINATED)
    return;
  uint64_t flags = irq_save();
  if (task->state == TASK_SLEEPING || task->state == TASK_BLOCKED ||
      task->state == TASK_WAITING) {
    if (unlink_task(task) && transition_task(task, TASK_READY)) {
      task->ready_since_tick = scheduler_tick_count;
      task->effective_priority = task->base_priority;
      if (enqueue_task(&ready_queue, task))
        ++scheduler_diag.wakeups;
    }
  }
  irq_restore(flags);
}

uint64_t scheduler_get_tick_count(void) { return scheduler_tick_count; }
uint64_t scheduler_get_context_switch_count(void) { return g_context_switches; }
Task *scheduler_get_idle_task(void) { return idle_task_ptr; }
bool scheduler_is_running(void) { return scheduler_running; }
uint32_t scheduler_get_ready_count(void) { return runqueue_get_size(&ready_queue); }
uint32_t scheduler_get_sleeping_count(void) { return runqueue_get_size(&sleep_queue); }
uint32_t scheduler_get_blocked_count(void) { return runqueue_get_size(&blocked_queue); }
uint64_t scheduler_get_last_switch_from(void) { return g_last_switch_old_id; }
uint64_t scheduler_get_last_switch_to(void) { return g_last_switch_new_id; }
const char *scheduler_get_last_reason_str(void) {
  if (g_last_switch_reason == SCHEDULER_SWITCH_SLEEP) return "SLEEP";
  if (g_last_switch_reason == SCHEDULER_SWITCH_YIELD) return "YIELD";
  if (g_last_switch_reason == SCHEDULER_SWITCH_BLOCK) return "BLOCK";
  if (g_last_switch_reason == SCHEDULER_SWITCH_WAKE_PREEMPT) return "PREEMPT";
  if (g_last_switch_reason == SCHEDULER_SWITCH_EXIT) return "EXIT";
  return "QUANTUM";
}

static bool validate_queue_state(const RunQueue *queue, TaskState state) {
  if (!runqueue_validate(queue, NULL))
    return false;
  list_node_t *node = queue->ready_list.head;
  while (node) {
    Task *task = LIST_ENTRY(node, Task, queue_node);
    if (task->state != state || task == current_task)
      return false;
    node = node->next;
  }
  return true;
}

bool scheduler_validate_consistency(void) {
  bool valid = validate_queue_state(&ready_queue, TASK_READY) &&
               validate_queue_state(&waiting_queue, TASK_WAITING) &&
               validate_queue_state(&sleep_queue, TASK_SLEEPING) &&
               validate_queue_state(&blocked_queue, TASK_BLOCKED) &&
               validate_queue_state(&terminated_queue, TASK_TERMINATED);
  if (current_task && current_task->state != TASK_RUNNING)
    valid = false;
  if (!idle_task_ptr || idle_task_ptr->state == TASK_TERMINATED ||
      idle_task_ptr->state == TASK_BLOCKED ||
      idle_task_ptr->state == TASK_SLEEPING ||
      idle_task_ptr->state == TASK_WAITING)
    valid = false;
  scheduler_diag.queues_consistent = valid;
  if (!valid) {
    ++scheduler_diag.queue_corruptions;
    record_scheduler_error(NULL, ATOMS_EXEC_ERR_CORRUPT,
                           "scheduler consistency audit failed");
  }
  return valid;
}

void scheduler_get_diagnostics(SchedulerDiagnostics *out) {
  if (!out)
    return;
  uint64_t flags = irq_save();
  scheduler_diag.tick_count = scheduler_tick_count;
  scheduler_diag.context_switches = g_context_switches;
  scheduler_diag.ready_count = runqueue_get_size(&ready_queue);
  scheduler_diag.waiting_count = runqueue_get_size(&waiting_queue);
  scheduler_diag.sleeping_count = runqueue_get_size(&sleep_queue);
  scheduler_diag.blocked_count = runqueue_get_size(&blocked_queue);
  scheduler_diag.terminated_count = runqueue_get_size(&terminated_queue);
  scheduler_diag.current_task_id = current_task ? current_task->id : 0;
  scheduler_diag.current_process_id =
      current_task ? current_task->owner_pid : 0;
  scheduler_diag.current_quantum = current_task && current_task->quantum > 0
                                       ? (uint32_t)current_task->quantum
                                       : 0;
  scheduler_diag.current_quantum_used =
      current_task && current_task->default_quantum > current_task->quantum
          ? (uint32_t)(current_task->default_quantum - current_task->quantum)
          : 0;
  scheduler_diag.cpu_utilization_x100 =
      scheduler_tick_count ? (uint32_t)((scheduler_diag.busy_ticks * 10000U) /
                                        scheduler_tick_count)
                           : 0;
  scheduler_diag.average_wait_ticks =
      scheduler_diag.dispatched_tasks
          ? (uint32_t)(scheduler_diag.total_wait_ticks /
                       scheduler_diag.dispatched_tasks)
          : 0;
  scheduler_diag.running = scheduler_running;
  scheduler_diag.policy = scheduler_policy;
  *out = scheduler_diag;
  irq_restore(flags);
}

void scheduler_dump_runtime_diagnostics(void) {
  SchedulerDiagnostics diagnostics;
  scheduler_get_diagnostics(&diagnostics);
  display_print("\n--- Scheduler Runtime Diagnostics ---\n");
  display_print("Ticks: ");
  display_print_dec(diagnostics.tick_count);
  display_print(" Context switches: ");
  display_print_dec(diagnostics.context_switches);
  display_print("\nCurrent task/process: ");
  display_print_dec(diagnostics.current_task_id);
  display_print("/");
  display_print_dec(diagnostics.current_process_id);
  display_print(" CPU x100: ");
  display_print_dec(diagnostics.cpu_utilization_x100);
  display_print("\nReady/Wait/Sleep/Block/Dead: ");
  display_print_dec(diagnostics.ready_count);
  display_print("/");
  display_print_dec(diagnostics.waiting_count);
  display_print("/");
  display_print_dec(diagnostics.sleeping_count);
  display_print("/");
  display_print_dec(diagnostics.blocked_count);
  display_print("/");
  display_print_dec(diagnostics.terminated_count);
  display_print("\nVoluntary/Involuntary: ");
  display_print_dec(diagnostics.voluntary_switches);
  display_print("/");
  display_print_dec(diagnostics.involuntary_switches);
  display_print(" Avg wait: ");
  display_print_dec(diagnostics.average_wait_ticks);
  display_print("\nQueue errors/duplicates: ");
  display_print_dec(diagnostics.queue_corruptions);
  display_print("/");
  display_print_dec(diagnostics.duplicate_rejections);
  display_print(" Consistent: ");
  display_print(diagnostics.queues_consistent ? "YES" : "NO");
  display_print("\n-------------------------------------\n");
}

static void dump_queue(const char *name, const RunQueue *queue) {
  display_print(name);
  display_print(" [");
  display_print_dec(runqueue_get_size(queue));
  display_print("]: ");
  list_node_t *node = queue->ready_list.head;
  while (node) {
    Task *task = LIST_ENTRY(node, Task, queue_node);
    display_print_dec(task->id);
    display_print("(");
    display_print(task_state_name(task->state));
    display_print(",p");
    display_print_dec(task->effective_priority);
    display_print(") ");
    node = node->next;
  }
  display_print("\n");
}

void scheduler_dump_queues(void) {
  display_print("\n--- Scheduler Queues ---\n");
  dump_queue("READY", &ready_queue);
  dump_queue("WAITING", &waiting_queue);
  dump_queue("SLEEPING", &sleep_queue);
  dump_queue("BLOCKED", &blocked_queue);
  dump_queue("TERMINATED", &terminated_queue);
  display_print("------------------------\n");
}

void scheduler_dump_tasks(void) {
  display_print("\n--- Task Diagnostics ---\n");
  if (current_task) {
    display_print_dec(current_task->id);
    display_print(" RUNNING ");
    display_print(current_task->name ? current_task->name : "unnamed");
    display_print("\n");
  }
  scheduler_dump_queues();
}

static Task *find_in_queue(RunQueue *queue, uint64_t id) {
  list_node_t *node = queue->ready_list.head;
  while (node) {
    Task *task = LIST_ENTRY(node, Task, queue_node);
    if (task->id == id)
      return task;
    node = node->next;
  }
  return NULL;
}

void scheduler_dump_task_info(uint64_t id) {
  Task *task = current_task && current_task->id == id ? current_task : NULL;
  RunQueue *queues[] = {&ready_queue, &waiting_queue, &sleep_queue,
                        &blocked_queue, &terminated_queue};
  for (uint32_t i = 0; !task && i < 5U; ++i)
    task = find_in_queue(queues[i], id);
  if (!task) {
    display_print("Task not found\n");
    return;
  }
  display_print("\nTask ");
  display_print_dec(task->id);
  display_print(" ");
  display_print(task->name ? task->name : "unnamed");
  display_print(" state=");
  display_print(task_state_name(task->state));
  display_print(" pid=");
  display_print_dec(task->owner_pid);
  display_print(" priority=");
  display_print_dec(task->effective_priority);
  display_print(" runtime=");
  display_print_dec(task->total_run_ticks);
  display_print(" wait=");
  display_print_dec(task->total_wait_ticks);
  display_print(" switches=");
  display_print_dec(task->dispatch_count);
  display_print("\n");
}

static void terminate_matching_queue(RunQueue *queue, uint32_t pid) {
  list_node_t *node = queue->ready_list.head;
  while (node) {
    list_node_t *next = node->next;
    Task *task = LIST_ENTRY(node, Task, queue_node);
    if (task->owner_pid == pid || task->id == pid) {
      runqueue_remove(queue, task);
      if (transition_task(task, TASK_TERMINATED))
        enqueue_task(&terminated_queue, task);
    }
    node = next;
  }
}

void scheduler_terminate_tasks_by_pid(uint32_t pid) {
  if (!pid)
    return;
  uint64_t flags = irq_save();
  terminate_matching_queue(&ready_queue, pid);
  terminate_matching_queue(&waiting_queue, pid);
  terminate_matching_queue(&sleep_queue, pid);
  terminate_matching_queue(&blocked_queue, pid);
  if (current_task && current_task != idle_task_ptr &&
      (current_task->owner_pid == pid || current_task->id == pid)) {
    if (transition_task(current_task, TASK_TERMINATED)) {
      enqueue_task(&terminated_queue, current_task);
      current_task->quantum = 0;
      pending_switch_reason = SCHEDULER_SWITCH_EXIT;
    }
  }
  irq_restore(flags);
}

static uint8_t g_idle_stacks[ATOMS_MAX_CPUS][KERNEL_TASK_STACK_SIZE] __attribute__((aligned(16)));
static Task g_idle_tasks[ATOMS_MAX_CPUS] __attribute__((aligned(16)));

Task *scheduler_create_idle_task_cpu(uint32_t cpu_id) {
  if (cpu_id >= ATOMS_MAX_CPUS) return NULL;
  Task *idle = &g_idle_tasks[cpu_id];
  initialize_task_defaults(idle, "Idle_AP", SCHEDULER_PRIORITY_MIN);
  idle->state = TASK_READY;
  idle->default_quantum = 1;
  idle->quantum = 1;
  idle->assigned_cpu = cpu_id;
  idle->affinity_mask = (1ULL << cpu_id);
  idle->stack = &g_idle_stacks[cpu_id][0];
  cpu_extended_state_init_task(idle);
  context_prepare_kernel_task(idle, idle_task);
  return idle;
}

