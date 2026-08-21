#include "kernel/core/scheduler/include/runqueue.h"
#include <stddef.h>

static bool runqueue_header_valid(const RunQueue *rq) {
  return rq && rq->magic == RUNQUEUE_MAGIC &&
         rq->ready_list.magic == LIST_MAGIC;
}

void runqueue_init(RunQueue *rq) { runqueue_init_class(rq, TASK_QUEUE_NONE); }

void runqueue_init_class(RunQueue *rq, TaskQueueClass queue_class) {
  if (!rq)
    return;
  rq->magic = RUNQUEUE_MAGIC;
  rq->queue_class = queue_class;
  rq->enqueue_count = 0;
  rq->dequeue_count = 0;
  rq->rejection_count = 0;
  list_init(&rq->ready_list);
}

RunQueueResult runqueue_push_checked(RunQueue *rq, Task *task) {
  if (!rq || !task)
    return RUNQUEUE_ERR_ARGUMENT;
  if (!runqueue_header_valid(rq)) {
    ++rq->rejection_count;
    return RUNQUEUE_ERR_CORRUPT;
  }
  runqueue_validate_verbose(rq, "runqueue_push_checked:before_push");
  if (task->queue_class != TASK_QUEUE_NONE) {
    ++rq->rejection_count;
    return task->queue_class == rq->queue_class ? RUNQUEUE_ERR_DUPLICATE
                                                : RUNQUEUE_ERR_WRONG_QUEUE;
  }
  if (runqueue_contains(rq, task)) {
    ++rq->rejection_count;
    return RUNQUEUE_ERR_DUPLICATE;
  }

  list_insert_tail(&rq->ready_list, &task->queue_node);
  if (!runqueue_contains(rq, task)) {
    ++rq->rejection_count;
    return RUNQUEUE_ERR_CORRUPT;
  }
  task->queue_class = (uint8_t)rq->queue_class;
  ++rq->enqueue_count;
  runqueue_validate_verbose(rq, "runqueue_push_checked:after_push");
  return RUNQUEUE_OK;
}

void runqueue_push(RunQueue *rq, Task *task) {
  (void)runqueue_push_checked(rq, task);
}

Task *runqueue_pop(RunQueue *rq) {
  if (!runqueue_header_valid(rq))
    return NULL;
  runqueue_validate_verbose(rq, "runqueue_pop:before_pop");

  list_node_t *node = list_remove_head(&rq->ready_list);
  if (!node)
    return NULL;

  Task *task = LIST_ENTRY(node, Task, queue_node);
  task->queue_class = TASK_QUEUE_NONE;
  ++rq->dequeue_count;
  runqueue_validate_verbose(rq, "runqueue_pop:after_pop");
  return task;
}

Task *runqueue_peek(RunQueue *rq) {
  if (!runqueue_header_valid(rq) || list_is_empty(&rq->ready_list))
    return NULL;
  return LIST_ENTRY(rq->ready_list.head, Task, queue_node);
}

RunQueueResult runqueue_remove_checked(RunQueue *rq, Task *task) {
  if (!rq || !task)
    return RUNQUEUE_ERR_ARGUMENT;
  if (!runqueue_header_valid(rq)) {
    ++rq->rejection_count;
    return RUNQUEUE_ERR_CORRUPT;
  }
  runqueue_validate_verbose(rq, "runqueue_remove_checked:before_remove");
  if (task->queue_class != rq->queue_class) {
    ++rq->rejection_count;
    return RUNQUEUE_ERR_WRONG_QUEUE;
  }
  if (!runqueue_contains(rq, task)) {
    ++rq->rejection_count;
    return RUNQUEUE_ERR_NOT_FOUND;
  }

  list_remove(&rq->ready_list, &task->queue_node);
  task->queue_class = TASK_QUEUE_NONE;
  ++rq->dequeue_count;
  runqueue_validate_verbose(rq, "runqueue_remove_checked:after_remove");
  return RUNQUEUE_OK;
}

void runqueue_remove(RunQueue *rq, Task *task) {
  (void)runqueue_remove_checked(rq, task);
}

bool runqueue_contains(const RunQueue *rq, const Task *task) {
  if (!runqueue_header_valid(rq) || !task)
    return false;
  return list_contains((list_t *)&rq->ready_list,
                       (list_node_t *)&task->queue_node);
}

bool runqueue_is_empty(const RunQueue *rq) {
  if (!runqueue_header_valid(rq))
    return true;
  return list_is_empty((list_t *)&rq->ready_list);
}

uint32_t runqueue_get_size(const RunQueue *rq) {
  if (!runqueue_header_valid(rq))
    return 0;
  return rq->ready_list.size;
}

bool runqueue_validate(const RunQueue *rq, uint32_t *out_actual_size) {
  if (out_actual_size)
    *out_actual_size = 0;
  if (!runqueue_header_valid(rq))
    return false;
  if ((rq->ready_list.head == NULL) != (rq->ready_list.tail == NULL))
    return false;
  if (rq->ready_list.head && rq->ready_list.head->prev)
    return false;
  if (rq->ready_list.tail && rq->ready_list.tail->next)
    return false;

  const list_node_t *previous = NULL;
  const list_node_t *node = rq->ready_list.head;
  uint32_t count = 0;
  while (node) {
    if (count >= RUNQUEUE_AUDIT_LIMIT || node->prev != previous)
      return false;
    const Task *task = LIST_ENTRY(node, Task, queue_node);
    if (task->queue_class != rq->queue_class)
      return false;
    previous = node;
    node = node->next;
    ++count;
  }
  if (previous != rq->ready_list.tail || count != rq->ready_list.size)
    return false;
  if (out_actual_size)
    *out_actual_size = count;
  return true;
}

static void com1_put_hex64(uint64_t v) {
  extern void com1_puts(const char *s);
  char hx[] = "0123456789ABCDEF";
  char buf[19];
  buf[0] = '0'; buf[1] = 'x';
  for (int i = 15; i >= 0; i--) {
    buf[2 + (15 - i)] = hx[(v >> (i * 4)) & 0xF];
  }
  buf[18] = '\0';
  com1_puts(buf);
}

static void com1_put_dec(uint64_t v) {
  extern void com1_puts(const char *s);
  char buf[24];
  int p = 22;
  buf[23] = '\0';
  if (v == 0) {
    com1_puts("0");
    return;
  }
  while (v > 0) {
    buf[p--] = '0' + (v % 10);
    v /= 10;
  }
  com1_puts(&buf[p + 1]);
}

void runqueue_dump_and_halt(const RunQueue *rq, const list_node_t *failing_node, const char *reason, const char *caller_site) {
  extern void com1_puts(const char *s);
  extern Task *scheduler_current_task(void);
  extern uint64_t scheduler_get_tick_count(void);

  com1_puts("\r\n=======================================================\r\n");
  com1_puts("[RQ_ASSERT FORENSIC TRAP TRIGGERED]\r\n");
  com1_puts("Reason      : "); com1_puts(reason); com1_puts("\r\n");
  com1_puts("Site/Caller : "); com1_puts(caller_site ? caller_site : "UNKNOWN"); com1_puts("\r\n");
  com1_puts("Tick        : "); com1_put_dec(scheduler_get_tick_count()); com1_puts("\r\n");

  Task *curr = scheduler_current_task();
  com1_puts("Current Task: ");
  if (curr) {
    com1_puts(curr->name ? curr->name : "unnamed");
    com1_puts(" (PID="); com1_put_dec(curr->id);
    com1_puts(", State="); com1_put_dec(curr->state);
    com1_puts(", Ptr="); com1_put_hex64((uint64_t)curr);
    com1_puts(")\r\n");
  } else {
    com1_puts("NULL\r\n");
  }

  if (rq) {
    com1_puts("Queue Ptr   : "); com1_put_hex64((uint64_t)rq); com1_puts("\r\n");
    com1_puts("Queue Class : "); com1_put_dec(rq->queue_class); com1_puts("\r\n");
    com1_puts("Queue Magic : "); com1_put_hex64(rq->magic); com1_puts("\r\n");
    com1_puts("Queue Head  : "); com1_put_hex64((uint64_t)rq->ready_list.head); com1_puts("\r\n");
    com1_puts("Queue Tail  : "); com1_put_hex64((uint64_t)rq->ready_list.tail); com1_puts("\r\n");
    com1_puts("Queue Size  : "); com1_put_dec(rq->ready_list.size); com1_puts("\r\n");
  }

  if (failing_node) {
    com1_puts("Failing Node: "); com1_put_hex64((uint64_t)failing_node); com1_puts("\r\n");
    com1_puts("  Node->prev: "); com1_put_hex64((uint64_t)failing_node->prev); com1_puts("\r\n");
    com1_puts("  Node->next: "); com1_put_hex64((uint64_t)failing_node->next); com1_puts("\r\n");
    Task *t = LIST_ENTRY(failing_node, Task, queue_node);
    com1_puts("  Task Calc : "); com1_put_hex64((uint64_t)t); com1_puts("\r\n");
    com1_puts("  Task Name : "); com1_puts(t->name ? t->name : "unnamed"); com1_puts("\r\n");
    com1_puts("  Task PID  : "); com1_put_dec(t->id); com1_puts("\r\n");
    com1_puts("  Task State: "); com1_put_dec(t->state); com1_puts("\r\n");
    com1_puts("  Task QCls : "); com1_put_dec(t->queue_class); com1_puts("\r\n");
  }

  // Complete chain traversal from HEAD
  com1_puts("\r\n--- CHAIN TRAVERSAL FROM HEAD ---\r\n");
  if (rq && rq->ready_list.head) {
    list_node_t *cur = rq->ready_list.head;
    int idx = 0;
    while (cur && idx < 20) {
      Task *t = LIST_ENTRY(cur, Task, queue_node);
      com1_puts(" ["); com1_put_dec(idx); com1_puts("] Node="); com1_put_hex64((uint64_t)cur);
      com1_puts(" Prev="); com1_put_hex64((uint64_t)cur->prev);
      com1_puts(" Next="); com1_put_hex64((uint64_t)cur->next);
      com1_puts(" Task="); com1_puts(t->name ? t->name : "?");
      com1_puts("(PID="); com1_put_dec(t->id);
      com1_puts(", St="); com1_put_dec(t->state);
      com1_puts(", QC="); com1_put_dec(t->queue_class);
      com1_puts(")\r\n");
      cur = cur->next;
      idx++;
    }
  }
  com1_puts("=======================================================\r\n");

  __asm__ volatile("cli");
  while (1) {
    __asm__ volatile("hlt");
  }
}

bool runqueue_validate_verbose(const RunQueue *rq, const char *caller_site) {
  if (!rq) {
    runqueue_dump_and_halt(rq, NULL, "NULL queue pointer", caller_site);
    return false;
  }
  if (!runqueue_header_valid(rq)) {
    runqueue_dump_and_halt(rq, NULL, "Invalid queue header/magic", caller_site);
    return false;
  }
  if ((rq->ready_list.head == NULL) != (rq->ready_list.tail == NULL)) {
    runqueue_dump_and_halt(rq, rq->ready_list.head ? rq->ready_list.head : rq->ready_list.tail,
                           "head/tail null mismatch", caller_site);
    return false;
  }
  if (rq->ready_list.head && rq->ready_list.head->prev != NULL) {
    runqueue_dump_and_halt(rq, rq->ready_list.head, "head->prev != NULL", caller_site);
    return false;
  }
  if (rq->ready_list.tail && rq->ready_list.tail->next != NULL) {
    runqueue_dump_and_halt(rq, rq->ready_list.tail, "tail->next != NULL", caller_site);
    return false;
  }

  const list_node_t *previous = NULL;
  const list_node_t *node = rq->ready_list.head;
  uint32_t count = 0;
  while (node) {
    if (count >= RUNQUEUE_AUDIT_LIMIT) {
      runqueue_dump_and_halt(rq, node, "Circular loop detected", caller_site);
      return false;
    }
    if (node->prev != previous) {
      runqueue_dump_and_halt(rq, node, "node->prev linkage broken", caller_site);
      return false;
    }
    const Task *task = LIST_ENTRY(node, Task, queue_node);
    if (task->queue_class != rq->queue_class) {
      runqueue_dump_and_halt(rq, node, "task queue_class mismatch", caller_site);
      return false;
    }
    previous = node;
    node = node->next;
    ++count;
  }
  if (previous != rq->ready_list.tail || count != rq->ready_list.size) {
    runqueue_dump_and_halt(rq, previous, "size / tail mismatch", caller_site);
    return false;
  }
  return true;
}
