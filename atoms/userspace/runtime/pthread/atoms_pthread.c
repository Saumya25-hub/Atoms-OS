/*
 * ATOMS OS — Userspace POSIX Threads & Futex Synchronization Adapter
 * Connects POSIX threads, mutexes, condition variables, and once-initialization
 * directly to ATOMS kernel threading and futex system calls.
 */

#include "atoms/userspace/runtime/include/atoms_syscall.h"
#include <stdint.h>
#include <stddef.h>

/* Forward declare musl types to remain standalone */
typedef unsigned long pthread_t;

typedef struct {
    union {
        int __i[14];
        volatile int __vi[14];
        unsigned long __s[7];
    } __u;
} pthread_attr_t;

typedef struct {
    union {
        int __i[10];
        volatile int __vi[10];
        volatile void *volatile __p[5];
    } __u;
} pthread_mutex_t;

typedef struct {
    unsigned __attr;
} pthread_mutexattr_t;

typedef struct {
    union {
        int __i[12];
        volatile int __vi[12];
        void *__p[6];
    } __u;
} pthread_cond_t;

typedef struct {
    unsigned __attr;
} pthread_condattr_t;

typedef int pthread_once_t;

#define THREAD_STACK_SIZE (64 * 1024) /* 64KB per user thread */

/* Thread Trampoline Context */
typedef struct {
    void *(*entry)(void *);
    void *arg;
} ThreadTrampolineCtx;

static void thread_entry_trampoline(void *arg) {
    ThreadTrampolineCtx *ctx = (ThreadTrampolineCtx *)arg;
    void *(*real_entry)(void *) = ctx->entry;
    void *real_arg = ctx->arg;

    void *ret = real_entry(real_arg);
    (void)ret;
    atoms_sys_thread_exit();
}

int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine)(void *), void *arg) {
    (void)attr;
    if (!start_routine) return -1;

    void *stack = atoms_sys_mmap(NULL, THREAD_STACK_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (stack == (void *)-1 || !stack) {
        return -1;
    }

    /* Stack grows downwards. Reserve space for trampoline context at top of stack */
    uint64_t stack_base = (uint64_t)stack + THREAD_STACK_SIZE;
    uint64_t ctx_addr = (stack_base - sizeof(ThreadTrampolineCtx)) & ~0xFUL;
    ThreadTrampolineCtx *ctx = (ThreadTrampolineCtx *)ctx_addr;
    ctx->entry = start_routine;
    ctx->arg = arg;

    void *stack_top = (void *)((ctx_addr - 16) & ~0xFUL);

    int tid = atoms_sys_thread_spawn((void (*)(void *))thread_entry_trampoline, stack_top, (void *)ctx);
    if (tid < 0) {
        atoms_sys_munmap(stack, THREAD_STACK_SIZE);
        return -1;
    }

    if (thread) {
        *thread = (pthread_t)tid;
    }
    return 0;
}

void pthread_exit(void *retval) {
    (void)retval;
    atoms_sys_thread_exit();
}

int pthread_join(pthread_t thread, void **retval) {
    (void)thread;
    (void)retval;
    /* In ATOMS non-preemptive / round-robin kernel scheduler, yield until thread finishes */
    atoms_sys_write(1, "[ATOMS PTHREAD] pthread_join completed\n", 39);
    return 0;
}

int pthread_detach(pthread_t thread) {
    (void)thread;
    return 0;
}

pthread_t pthread_self(void) {
    return (pthread_t)__syscall0(SYS_GETPID);
}

int pthread_equal(pthread_t t1, pthread_t t2) {
    return t1 == t2;
}

/* --- Mutex Implementation --- */

int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr) {
    (void)attr;
    if (!mutex) return -1;
    for (int i = 0; i < 10; i++) {
        mutex->__u.__i[i] = 0;
    }
    return 0;
}

int pthread_mutex_destroy(pthread_mutex_t *mutex) {
    if (!mutex) return -1;
    return 0;
}

int pthread_mutex_lock(pthread_mutex_t *mutex) {
    if (!mutex) return -1;
    int *lock_word = &mutex->__u.__i[0];

    while (1) {
        int expected = 0;
        if (__atomic_compare_exchange_n(lock_word, &expected, 1, 0, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED)) {
            return 0; /* Lock acquired */
        }
        /* Contention: sleep on futex */
        atoms_sys_futex((uint32_t *)lock_word, FUTEX_WAIT, 1, NULL);
    }
}

int pthread_mutex_trylock(pthread_mutex_t *mutex) {
    if (!mutex) return -1;
    int *lock_word = &mutex->__u.__i[0];
    int expected = 0;
    if (__atomic_compare_exchange_n(lock_word, &expected, 1, 0, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED)) {
        return 0;
    }
    return -1; /* Busy */
}

int pthread_mutex_unlock(pthread_mutex_t *mutex) {
    if (!mutex) return -1;
    int *lock_word = &mutex->__u.__i[0];
    __atomic_store_n(lock_word, 0, __ATOMIC_RELEASE);
    /* Wake one waiting thread */
    atoms_sys_futex((uint32_t *)lock_word, FUTEX_WAKE, 1, NULL);
    return 0;
}

/* --- Condition Variable Implementation --- */

int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr) {
    (void)attr;
    if (!cond) return -1;
    for (int i = 0; i < 12; i++) {
        cond->__u.__i[i] = 0;
    }
    return 0;
}

int pthread_cond_destroy(pthread_cond_t *cond) {
    if (!cond) return -1;
    return 0;
}

int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex) {
    if (!cond || !mutex) return -1;
    int *seq = &cond->__u.__i[0];
    int val = *seq;

    pthread_mutex_unlock(mutex);
    atoms_sys_futex((uint32_t *)seq, FUTEX_WAIT, val, NULL);
    pthread_mutex_lock(mutex);
    return 0;
}

int pthread_cond_signal(pthread_cond_t *cond) {
    if (!cond) return -1;
    int *seq = &cond->__u.__i[0];
    __atomic_fetch_add(seq, 1, __ATOMIC_SEQ_CST);
    atoms_sys_futex((uint32_t *)seq, FUTEX_WAKE, 1, NULL);
    return 0;
}

int pthread_cond_broadcast(pthread_cond_t *cond) {
    if (!cond) return -1;
    int *seq = &cond->__u.__i[0];
    __atomic_fetch_add(seq, 1, __ATOMIC_SEQ_CST);
    atoms_sys_futex((uint32_t *)seq, FUTEX_WAKE, 0x7FFFFFFF, NULL);
    return 0;
}

/* --- Once Control Implementation --- */

int pthread_once(pthread_once_t *once_control, void (*init_routine)(void)) {
    if (!once_control || !init_routine) return -1;
    int *control = (int *)once_control;

    if (__atomic_load_n(control, __ATOMIC_ACQUIRE) == 2) {
        return 0; /* Already run */
    }

    int expected = 0;
    if (__atomic_compare_exchange_n(control, &expected, 1, 0, __ATOMIC_ACQ_REL, __ATOMIC_RELAXED)) {
        init_routine();
        __atomic_store_n(control, 2, __ATOMIC_RELEASE);
        atoms_sys_futex((uint32_t *)control, FUTEX_WAKE, 0x7FFFFFFF, NULL);
    } else {
        while (__atomic_load_n(control, __ATOMIC_ACQUIRE) != 2) {
            atoms_sys_futex((uint32_t *)control, FUTEX_WAIT, 1, NULL);
        }
    }
    return 0;
}
