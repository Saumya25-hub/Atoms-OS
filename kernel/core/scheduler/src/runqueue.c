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
  return RUNQUEUE_OK;
}

void runqueue_push(RunQueue *rq, Task *task) {
  (void)runqueue_push_checked(rq, task);
}

Task *runqueue_pop(RunQueue *rq) {
  if (!runqueue_header_valid(rq))
    return NULL;

  list_node_t *node = list_remove_head(&rq->ready_list);
  if (!node)
    return NULL;

  Task *task = LIST_ENTRY(node, Task, queue_node);
  task->queue_class = TASK_QUEUE_NONE;
  ++rq->dequeue_count;
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
