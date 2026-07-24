/*
 * BOS OS — Phase 2: IPC & Shared Memory Engine
 * ipc_sync.h — Synchronization Primitives
 */

#ifndef BOS_IPC_SYNC_H
#define BOS_IPC_SYNC_H

#include "kernel/ipc/include/ipc_types.h"

void ipc_sync_init(void);

/* Spinlock */
void ipc_spinlock_init(ipc_spinlock_t* lock);
void ipc_spinlock_acquire(ipc_spinlock_t* lock);
void ipc_spinlock_release(ipc_spinlock_t* lock);

/* Mutex */
void ipc_mutex_init(ipc_mutex_t* mtx);
void ipc_mutex_lock(ipc_mutex_t* mtx);
void ipc_mutex_unlock(ipc_mutex_t* mtx);

/* Reader-Writer Lock */
void ipc_rwlock_init(ipc_rwlock_t* rw);
void ipc_rwlock_read_lock(ipc_rwlock_t* rw);
void ipc_rwlock_read_unlock(ipc_rwlock_t* rw);
void ipc_rwlock_write_lock(ipc_rwlock_t* rw);
void ipc_rwlock_write_unlock(ipc_rwlock_t* rw);

/* Event */
void ipc_event_init(ipc_event_t* evt, bool auto_reset);
void ipc_event_signal(ipc_event_t* evt);
void ipc_event_reset(ipc_event_t* evt);
bool ipc_event_is_signaled(ipc_event_t* evt);

#endif /* BOS_IPC_SYNC_H */
