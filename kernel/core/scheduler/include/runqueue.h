#ifndef RUNQUEUE_H
#define RUNQUEUE_H

#include "../../lib/include/list.h"
#include "task.h"
#include <stdbool.h>
#include <stdint.h>

#define RUNQUEUE_MAGIC 0x88AA4422
#define RUNQUEUE_AUDIT_LIMIT 100000U

typedef enum {
  RUNQUEUE_OK = 0,
  RUNQUEUE_ERR_ARGUMENT,
  RUNQUEUE_ERR_CORRUPT,
  RUNQUEUE_ERR_DUPLICATE,
  RUNQUEUE_ERR_NOT_FOUND,
  RUNQUEUE_ERR_WRONG_QUEUE
} RunQueueResult;

typedef struct {
  uint32_t magic;
  TaskQueueClass queue_class;
  list_t ready_list;
  uint64_t enqueue_count;
  uint64_t dequeue_count;
  uint64_t rejection_count;
} RunQueue;

void runqueue_init(RunQueue *rq);
void runqueue_init_class(RunQueue *rq, TaskQueueClass queue_class);
RunQueueResult runqueue_push_checked(RunQueue *rq, Task *task);
void runqueue_push(RunQueue *rq, Task *task);
Task *runqueue_pop(RunQueue *rq);
Task *runqueue_peek(RunQueue *rq);
RunQueueResult runqueue_remove_checked(RunQueue *rq, Task *task);
void runqueue_remove(RunQueue *rq, Task *task);
bool runqueue_contains(const RunQueue *rq, const Task *task);
bool runqueue_is_empty(const RunQueue *rq);
uint32_t runqueue_get_size(const RunQueue *rq);
bool runqueue_validate(const RunQueue *rq, uint32_t *out_actual_size);
bool runqueue_validate_verbose(const RunQueue *rq, const char *caller_site);

#endif // RUNQUEUE_H
