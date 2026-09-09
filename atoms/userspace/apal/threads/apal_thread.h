/*
 * ATOMS Platform Adaptation Layer (APAL)
 * Threading & Synchronization Adapter (Chromium base::PlatformThread / base::Lock)
 */

#ifndef ATOMS_APAL_THREAD_H
#define ATOMS_APAL_THREAD_H

#include "../include/apal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef uint64_t apal_thread_t;
typedef uint32_t apal_thread_id_t;

typedef void *(*apal_thread_fn_t)(void *arg);

/* Mutex structure backed by atomic futex */
typedef struct {
    volatile uint32_t lock_word;
    uint32_t owner_tid;
} apal_mutex_t;

/* Condition Variable */
typedef struct {
    volatile uint32_t seq;
} apal_cond_t;

/* Thread Management */
apal_status_t apal_thread_create(apal_thread_t *out_thread, size_t stack_size, apal_thread_fn_t func, void *arg);
apal_status_t apal_thread_join(apal_thread_t thread, void **out_result);
apal_status_t apal_thread_detach(apal_thread_t thread);
apal_thread_id_t apal_thread_current_id(void);
void apal_thread_yield(void);
void apal_thread_sleep_ms(uint32_t ms);

/* Synchronization: Mutex */
apal_status_t apal_mutex_init(apal_mutex_t *mutex);
apal_status_t apal_mutex_lock(apal_mutex_t *mutex);
apal_status_t apal_mutex_trylock(apal_mutex_t *mutex);
apal_status_t apal_mutex_unlock(apal_mutex_t *mutex);
apal_status_t apal_mutex_destroy(apal_mutex_t *mutex);

/* Synchronization: Condition Variable */
apal_status_t apal_cond_init(apal_cond_t *cond);
apal_status_t apal_cond_wait(apal_cond_t *cond, apal_mutex_t *mutex);
apal_status_t apal_cond_timedwait(apal_cond_t *cond, apal_mutex_t *mutex, uint32_t timeout_ms);
apal_status_t apal_cond_signal(apal_cond_t *cond);
apal_status_t apal_cond_broadcast(apal_cond_t *cond);
apal_status_t apal_cond_destroy(apal_cond_t *cond);

#ifdef __cplusplus
}
#endif

#endif /* ATOMS_APAL_THREAD_H */
