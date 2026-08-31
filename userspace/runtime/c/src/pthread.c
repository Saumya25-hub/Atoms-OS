/*
 * ATOMS OS — Userspace POSIX Threads & Futex Synchronization
 * Adapted from musl libc pthread design (MIT License)
 */

#include "../include/pthread.h"
#include "../include/atoms_syscall.h"
#include "../include/stdlib.h"
#include "../include/sys/mman.h"
#include "../include/unistd.h"

int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine)(void *), void *arg) {
    (void)attr;
    size_t stack_size = 64 * 1024;
    void *stack = mmap(NULL, stack_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (stack == MAP_FAILED) {
        return -1;
    }

    void *stack_top = (void *)(((uint64_t)stack + stack_size - 16) & ~0xFUL);
    uint64_t tid = __atoms_syscall3(SYS_THREAD_SPAWN, (uint64_t)start_routine, (uint64_t)stack_top, (uint64_t)arg);
    if (tid == (uint64_t)-1) {
        munmap(stack, stack_size);
        return -1;
    }

    if (thread) {
        *thread = tid;
    }
    return 0;
}

void pthread_exit(void *retval) {
    (void)retval;
    __atoms_syscall1(SYS_THREAD_EXIT, 0);
    while (1) {}
}

int pthread_join(pthread_t thread, void **retval) {
    (void)thread; (void)retval;
    /* In non-blocking / co-routine model, yield until thread terminates */
    sched_yield();
    return 0;
}

int pthread_detach(pthread_t thread) {
    (void)thread;
    return 0;
}

pthread_t pthread_self(void) {
    return (pthread_t)getpid();
}

int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr) {
    (void)attr;
    if (!mutex) return -1;
    mutex->lock_val = 0;
    mutex->owner_tid = 0;
    mutex->recursion_count = 0;
    return 0;
}

int pthread_mutex_destroy(pthread_mutex_t *mutex) {
    (void)mutex;
    return 0;
}

int pthread_mutex_lock(pthread_mutex_t *mutex) {
    if (!mutex) return -1;

    while (1) {
        int expected = 0;
        if (__atomic_compare_exchange_n(&mutex->lock_val, &expected, 1, 0, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED)) {
            return 0; /* Successfully acquired */
        }
        /* Lock held by another thread; futex wait */
        __atoms_syscall4(SYS_FUTEX, (uint64_t)&mutex->lock_val, FUTEX_WAIT, 1, 0);
    }
}

int pthread_mutex_trylock(pthread_mutex_t *mutex) {
    if (!mutex) return -1;
    int expected = 0;
    if (__atomic_compare_exchange_n(&mutex->lock_val, &expected, 1, 0, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED)) {
        return 0;
    }
    return -1; /* Busy */
}

int pthread_mutex_unlock(pthread_mutex_t *mutex) {
    if (!mutex) return -1;
    __atomic_store_n(&mutex->lock_val, 0, __ATOMIC_RELEASE);
    /* Wake 1 waiting thread */
    __atoms_syscall4(SYS_FUTEX, (uint64_t)&mutex->lock_val, FUTEX_WAKE, 1, 0);
    return 0;
}

int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr) {
    (void)attr;
    if (!cond) return -1;
    cond->seq = 0;
    return 0;
}

int pthread_cond_destroy(pthread_cond_t *cond) {
    (void)cond;
    return 0;
}

int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex) {
    if (!cond || !mutex) return -1;
    uint32_t current_seq = cond->seq;
    pthread_mutex_unlock(mutex);
    __atoms_syscall4(SYS_FUTEX, (uint64_t)&cond->seq, FUTEX_WAIT, current_seq, 0);
    pthread_mutex_lock(mutex);
    return 0;
}

int pthread_cond_signal(pthread_cond_t *cond) {
    if (!cond) return -1;
    __atomic_add_fetch(&cond->seq, 1, __ATOMIC_RELEASE);
    __atoms_syscall4(SYS_FUTEX, (uint64_t)&cond->seq, FUTEX_WAKE, 1, 0);
    return 0;
}

int pthread_cond_broadcast(pthread_cond_t *cond) {
    if (!cond) return -1;
    __atomic_add_fetch(&cond->seq, 1, __ATOMIC_RELEASE);
    __atoms_syscall4(SYS_FUTEX, (uint64_t)&cond->seq, FUTEX_WAKE, 64, 0);
    return 0;
}
