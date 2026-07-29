/**
 * @file deferred_work.h
 * @brief BOS OS Deferred Initialization & Background Task Subsystem
 * @status Production Kernel Architecture Component
 */

#ifndef BOS_DEFERRED_WORK_H
#define BOS_DEFERRED_WORK_H

#include <stdint.h>
#include <stdbool.h>

typedef void (*BOS_DeferredTaskFunc)(void);

typedef struct {
    const char*          task_name;
    BOS_DeferredTaskFunc func;
    bool                 executed;
} BOS_DeferredTask;

#define BOS_MAX_DEFERRED_TASKS 32

void BOS_DeferredWork_Init(void);
bool BOS_DeferredWork_Register(const char* name, BOS_DeferredTaskFunc func);
void BOS_DeferredWork_PumpIdleQueue(void);
void BOS_RunAllDeferredTests(void);

#endif /* BOS_DEFERRED_WORK_H */
