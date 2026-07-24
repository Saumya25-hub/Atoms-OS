/*
 * BOS OS — Phase 2: IPC & Shared Memory Engine
 * ipc_sync.c — Synchronization Primitives Implementation
 *
 * Uses GCC/Clang __sync builtins for atomic operations,
 * matching the existing heap spinlock pattern in BOS OS.
 */

#include "kernel/ipc/sync/ipc_sync.h"
#include "kernel/ipc/debug/ipc_debug.h"

void ipc_sync_init(void) {
    ipc_debug_log(IPC_LOG_INFO, "SYNC", "Synchronization Primitives Initialized");
}

/* ============================================================
 * Spinlock
 * ============================================================ */
void ipc_spinlock_init(ipc_spinlock_t* lock) {
    if (!lock) return;
    lock->locked = 0;
    lock->owner  = 0xFFFFFFFF;
}

void ipc_spinlock_acquire(ipc_spinlock_t* lock) {
    if (!lock) return;
    while (__sync_lock_test_and_set(&lock->locked, 1)) {
        while (lock->locked) {
            __asm__ volatile("pause" ::: "memory");
        }
    }
    lock->owner = 0; /* Kernel context */
}

void ipc_spinlock_release(ipc_spinlock_t* lock) {
    if (!lock) return;
    lock->owner = 0xFFFFFFFF;
    __sync_lock_release(&lock->locked);
}

/* ============================================================
 * Mutex
 * ============================================================ */
void ipc_mutex_init(ipc_mutex_t* mtx) {
    if (!mtx) return;
    mtx->locked     = 0;
    mtx->owner      = 0xFFFFFFFF;
    mtx->wait_count = 0;
}

void ipc_mutex_lock(ipc_mutex_t* mtx) {
    if (!mtx) return;
    __sync_fetch_and_add(&mtx->wait_count, 1);
    while (__sync_lock_test_and_set(&mtx->locked, 1)) {
        while (mtx->locked) {
            __asm__ volatile("pause" ::: "memory");
        }
    }
    __sync_fetch_and_sub(&mtx->wait_count, 1);
    mtx->owner = 0;
}

void ipc_mutex_unlock(ipc_mutex_t* mtx) {
    if (!mtx) return;
    mtx->owner = 0xFFFFFFFF;
    __sync_lock_release(&mtx->locked);
}

/* ============================================================
 * Reader-Writer Lock
 * ============================================================ */
void ipc_rwlock_init(ipc_rwlock_t* rw) {
    if (!rw) return;
    rw->readers      = 0;
    rw->writer       = 0;
    rw->writer_owner = 0xFFFFFFFF;
}

void ipc_rwlock_read_lock(ipc_rwlock_t* rw) {
    if (!rw) return;
    while (1) {
        while (rw->writer) {
            __asm__ volatile("pause" ::: "memory");
        }
        __sync_fetch_and_add(&rw->readers, 1);
        if (!rw->writer) break;
        __sync_fetch_and_sub(&rw->readers, 1);
    }
}

void ipc_rwlock_read_unlock(ipc_rwlock_t* rw) {
    if (!rw) return;
    __sync_fetch_and_sub(&rw->readers, 1);
}

void ipc_rwlock_write_lock(ipc_rwlock_t* rw) {
    if (!rw) return;
    while (__sync_lock_test_and_set(&rw->writer, 1)) {
        while (rw->writer) {
            __asm__ volatile("pause" ::: "memory");
        }
    }
    while (rw->readers > 0) {
        __asm__ volatile("pause" ::: "memory");
    }
    rw->writer_owner = 0;
}

void ipc_rwlock_write_unlock(ipc_rwlock_t* rw) {
    if (!rw) return;
    rw->writer_owner = 0xFFFFFFFF;
    __sync_lock_release(&rw->writer);
}

/* ============================================================
 * Event
 * ============================================================ */
void ipc_event_init(ipc_event_t* evt, bool auto_reset) {
    if (!evt) return;
    evt->signaled   = 0;
    evt->auto_reset = auto_reset ? 1 : 0;
}

void ipc_event_signal(ipc_event_t* evt) {
    if (!evt) return;
    __sync_lock_test_and_set(&evt->signaled, 1);
}

void ipc_event_reset(ipc_event_t* evt) {
    if (!evt) return;
    __sync_lock_release(&evt->signaled);
}

bool ipc_event_is_signaled(ipc_event_t* evt) {
    if (!evt) return false;
    if (evt->signaled) {
        if (evt->auto_reset) {
            __sync_lock_release(&evt->signaled);
        }
        return true;
    }
    return false;
}
