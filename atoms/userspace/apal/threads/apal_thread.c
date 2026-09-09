/*
 * ATOMS Platform Adaptation Layer (APAL)
 * Threading & Synchronization Implementation
 */

#include "apal_thread.h"
#include "atoms/userspace/runtime/include/atoms_syscall.h"

#define APAL_DEFAULT_STACK_SIZE (64 * 1024)

typedef struct {
    apal_thread_fn_t func;
    void *arg;
    volatile bool finished;
    void *result;
} apal_thread_trampoline_t;

static void apal_thread_runner(void *arg) {
    apal_thread_trampoline_t *t = (apal_thread_trampoline_t *)arg;
    if (t && t->func) {
        t->result = t->func(t->arg);
        t->finished = true;
    }
    atoms_sys_thread_exit();
}

apal_status_t apal_thread_create(apal_thread_t *out_thread, size_t stack_size, apal_thread_fn_t func, void *arg) {
    if (!func || !out_thread) return APAL_ERR_INVALID_PARAM;
    
    if (stack_size == 0) {
        stack_size = APAL_DEFAULT_STACK_SIZE;
    }
    /* Align stack size to 4KB page */
    stack_size = (stack_size + 4095) & ~4095;

    void *stack = atoms_sys_mmap(NULL, stack_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (!stack || stack == (void *)-1) {
        return APAL_ERR_NO_MEMORY;
    }

    /* Allocate trampoline at bottom of stack */
    apal_thread_trampoline_t *t = (apal_thread_trampoline_t *)stack;
    t->func = func;
    t->arg = arg;
    t->finished = false;
    t->result = NULL;

    /* Top of stack, 16-byte aligned */
    uint64_t stack_top = ((uint64_t)stack + stack_size - 64) & ~15ULL;

    int64_t tid = atoms_sys_thread_spawn(apal_thread_runner, (void *)stack_top, t);
    if (tid <= 0) {
        atoms_sys_munmap(stack, stack_size);
        return APAL_ERR_INTERNAL;
    }

    *out_thread = (apal_thread_t)t;
    return APAL_OK;
}

apal_status_t apal_thread_join(apal_thread_t thread, void **out_result) {
    if (thread == 0) return APAL_ERR_INVALID_PARAM;
    apal_thread_trampoline_t *t = (apal_thread_trampoline_t *)thread;

    /* Spin/yield until finished */
    while (!t->finished) {
        atoms_sys_yield();
    }

    if (out_result) {
        *out_result = t->result;
    }
    return APAL_OK;
}

apal_status_t apal_thread_detach(apal_thread_t thread) {
    (void)thread;
    return APAL_OK;
}

apal_thread_id_t apal_thread_current_id(void) {
    return (apal_thread_id_t)atoms_sys_getpid();
}

void apal_thread_yield(void) {
    atoms_sys_yield();
}

void apal_thread_sleep_ms(uint32_t ms) {
    uint64_t req[2];
    req[0] = ms / 1000;
    req[1] = (ms % 1000) * 1000000ULL;
    __syscall2(SYS_NANOSLEEP, (int64_t)req, 0);
}

/* Mutex implementation via atomic test-and-set and futex */
apal_status_t apal_mutex_init(apal_mutex_t *mutex) {
    if (!mutex) return APAL_ERR_INVALID_PARAM;
    mutex->lock_word = 0;
    mutex->owner_tid = 0;
    return APAL_OK;
}

apal_status_t apal_mutex_lock(apal_mutex_t *mutex) {
    if (!mutex) return APAL_ERR_INVALID_PARAM;
    
    while (__atomic_test_and_set(&mutex->lock_word, __ATOMIC_ACQUIRE)) {
        atoms_sys_futex((uint32_t *)&mutex->lock_word, FUTEX_WAIT, 1, NULL);
    }
    mutex->owner_tid = apal_thread_current_id();
    return APAL_OK;
}

apal_status_t apal_mutex_trylock(apal_mutex_t *mutex) {
    if (!mutex) return APAL_ERR_INVALID_PARAM;
    if (!__atomic_test_and_set(&mutex->lock_word, __ATOMIC_ACQUIRE)) {
        mutex->owner_tid = apal_thread_current_id();
        return APAL_OK;
    }
    return APAL_ERR_BUSY;
}

apal_status_t apal_mutex_unlock(apal_mutex_t *mutex) {
    if (!mutex) return APAL_ERR_INVALID_PARAM;
    mutex->owner_tid = 0;
    __atomic_clear(&mutex->lock_word, __ATOMIC_RELEASE);
    atoms_sys_futex((uint32_t *)&mutex->lock_word, FUTEX_WAKE, 1, NULL);
    return APAL_OK;
}

apal_status_t apal_mutex_destroy(apal_mutex_t *mutex) {
    if (!mutex) return APAL_ERR_INVALID_PARAM;
    mutex->lock_word = 0;
    return APAL_OK;
}

/* Condition variable implementation */
apal_status_t apal_cond_init(apal_cond_t *cond) {
    if (!cond) return APAL_ERR_INVALID_PARAM;
    cond->seq = 0;
    return APAL_OK;
}

apal_status_t apal_cond_wait(apal_cond_t *cond, apal_mutex_t *mutex) {
    if (!cond || !mutex) return APAL_ERR_INVALID_PARAM;
    uint32_t curr_seq = cond->seq;
    apal_mutex_unlock(mutex);
    atoms_sys_futex((uint32_t *)&cond->seq, FUTEX_WAIT, curr_seq, NULL);
    apal_mutex_lock(mutex);
    return APAL_OK;
}

apal_status_t apal_cond_timedwait(apal_cond_t *cond, apal_mutex_t *mutex, uint32_t timeout_ms) {
    if (!cond || !mutex) return APAL_ERR_INVALID_PARAM;
    uint32_t curr_seq = cond->seq;
    uint64_t to[2];
    to[0] = timeout_ms / 1000;
    to[1] = (timeout_ms % 1000) * 1000000ULL;
    apal_mutex_unlock(mutex);
    atoms_sys_futex((uint32_t *)&cond->seq, FUTEX_WAIT, curr_seq, to);
    apal_mutex_lock(mutex);
    return APAL_OK;
}

apal_status_t apal_cond_signal(apal_cond_t *cond) {
    if (!cond) return APAL_ERR_INVALID_PARAM;
    __atomic_fetch_add(&cond->seq, 1, __ATOMIC_SEQ_CST);
    atoms_sys_futex((uint32_t *)&cond->seq, FUTEX_WAKE, 1, NULL);
    return APAL_OK;
}

apal_status_t apal_cond_broadcast(apal_cond_t *cond) {
    if (!cond) return APAL_ERR_INVALID_PARAM;
    __atomic_fetch_add(&cond->seq, 1, __ATOMIC_SEQ_CST);
    atoms_sys_futex((uint32_t *)&cond->seq, FUTEX_WAKE, 64, NULL);
    return APAL_OK;
}

apal_status_t apal_cond_destroy(apal_cond_t *cond) {
    if (!cond) return APAL_ERR_INVALID_PARAM;
    return APAL_OK;
}
