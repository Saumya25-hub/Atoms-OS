#include "kernel/core/scheduler/include/runqueue.h"
#include <stddef.h>

void runqueue_init(RunQueue* rq) {
    if (!rq) return;
    rq->magic = RUNQUEUE_MAGIC;
    list_init(&rq->ready_list);
}

void runqueue_push(RunQueue* rq, Task* task) {
    if (!rq || rq->magic != RUNQUEUE_MAGIC || !task) return;
    
    // As per Rule 64, RunQueue does not transition state.
    // However, we assert architectural integrity.
    // Wait, we don't assert here, the scheduler handles states.
    // The queue just stores.

    list_insert_tail(&rq->ready_list, &task->queue_node);
}

Task* runqueue_pop(RunQueue* rq) {
    if (!rq || rq->magic != RUNQUEUE_MAGIC) return NULL;
    
    list_node_t* node = list_remove_head(&rq->ready_list);
    if (!node) return NULL;

    return LIST_ENTRY(node, Task, queue_node);
}

Task* runqueue_peek(RunQueue* rq) {
    if (!rq || rq->magic != RUNQUEUE_MAGIC) return NULL;

    if (list_is_empty(&rq->ready_list)) return NULL;

    list_node_t* node = rq->ready_list.head;
    return LIST_ENTRY(node, Task, queue_node);
}

void runqueue_remove(RunQueue* rq, Task* task) {
    if (!rq || rq->magic != RUNQUEUE_MAGIC || !task) return;

    list_remove(&rq->ready_list, &task->queue_node);
}

bool runqueue_contains(RunQueue* rq, Task* task) {
    if (!rq || rq->magic != RUNQUEUE_MAGIC || !task) return false;

    return list_contains(&rq->ready_list, &task->queue_node);
}

bool runqueue_is_empty(RunQueue* rq) {
    if (!rq || rq->magic != RUNQUEUE_MAGIC) return true;

    return list_is_empty(&rq->ready_list);
}

uint32_t runqueue_get_size(RunQueue* rq) {
    if (!rq || rq->magic != RUNQUEUE_MAGIC) return 0;

    return rq->ready_list.size;
}
