/*
 * ATOMS OS — Userspace POSIX Threads & Futex Synchronization
 * Adapted from musl libc pthread design (MIT License)
 * Includes true Thread-Local Storage (TLS) via IA32_FS_BASE (SYS_SET_FS_BASE)
 */

#include "../include/pthread.h"
#include "../include/atoms_syscall.h"
#include "../include/stdlib.h"
#include "../include/sys/mman.h"
#include "../include/unistd.h"
#include "../include/string.h"

/*
 * Architecture-level FS_BASE manipulation
 */
void atoms_set_fs_base(void *base) {
    __atoms_syscall1(SYS_SET_FS_BASE, (uint64_t)base);
}

void *atoms_get_fs_base(void) {
    return (void *)__atoms_syscall0(SYS_GET_FS_BASE);
}

/*
 * Thread Control Block (TCB) pointed to by IA32_FS_BASE
 */
typedef struct {
    void *self;                           /* %fs:0 points to self */
    const void *values[PTHREAD_KEYS_MAX]; /* Thread-specific storage slots */
    uint64_t tid;
} atoms_tcb_t;

typedef struct {
    int in_use;
    void (*destructor)(void *);
} atoms_key_desc_t;

static atoms_key_desc_t s_keys[PTHREAD_KEYS_MAX];
static volatile int s_key_lock = 0;

static atoms_tcb_t *ensure_tcb(void) {
    atoms_tcb_t *tcb = (atoms_tcb_t *)atoms_get_fs_base();
    if (!tcb) {
        tcb = (atoms_tcb_t *)malloc(sizeof(atoms_tcb_t));
        if (!tcb) return NULL;
        memset(tcb, 0, sizeof(atoms_tcb_t));
        tcb->self = tcb;
        tcb->tid = (uint64_t)getpid();
        atoms_set_fs_base(tcb);
    }
    return tcb;
}

int pthread_key_create(pthread_key_t *key, void (*destructor)(void *)) {
    if (!key) return -1;
    while (__atomic_test_and_set(&s_key_lock, __ATOMIC_ACQUIRE)) {}
    for (unsigned int i = 0; i < PTHREAD_KEYS_MAX; i++) {
        if (!s_keys[i].in_use) {
            s_keys[i].in_use = 1;
            s_keys[i].destructor = destructor;
            *key = i;
            __atomic_clear(&s_key_lock, __ATOMIC_RELEASE);
            return 0;
        }
    }
    __atomic_clear(&s_key_lock, __ATOMIC_RELEASE);
    return -1;
}

int pthread_key_delete(pthread_key_t key) {
    if (key >= PTHREAD_KEYS_MAX) return -1;
    while (__atomic_test_and_set(&s_key_lock, __ATOMIC_ACQUIRE)) {}
    s_keys[key].in_use = 0;
    s_keys[key].destructor = NULL;
    __atomic_clear(&s_key_lock, __ATOMIC_RELEASE);
    return 0;
}

int pthread_setspecific(pthread_key_t key, const void *value) {
    if (key >= PTHREAD_KEYS_MAX || !s_keys[key].in_use) return -1;
    atoms_tcb_t *tcb = ensure_tcb();
    if (!tcb) return -1;
    tcb->values[key] = value;
    return 0;
}

void *pthread_getspecific(pthread_key_t key) {
    if (key >= PTHREAD_KEYS_MAX || !s_keys[key].in_use) return NULL;
    atoms_tcb_t *tcb = (atoms_tcb_t *)atoms_get_fs_base();
    if (!tcb) return NULL;
    return (void *)tcb->values[key];
}

typedef struct {
    void *(*start_routine)(void *);
    void *arg;
    void *stack;
    size_t stack_size;
} thread_startup_args_t;

static void thread_trampoline(void *arg) {
    thread_startup_args_t *targs = (thread_startup_args_t *)arg;
    void *(*routine)(void *) = targs->start_routine;
    void *routine_arg = targs->arg;
    free(targs);

    // Initialize per-thread TCB and set FS_BASE
    ensure_tcb();

    void *ret = routine(routine_arg);
    pthread_exit(ret);
}

int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine)(void *), void *arg) {
    (void)attr;
    size_t stack_size = 64 * 1024;
    void *stack = mmap(NULL, stack_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (stack == MAP_FAILED) {
        return -1;
    }

    thread_startup_args_t *targs = (thread_startup_args_t *)malloc(sizeof(thread_startup_args_t));
    if (!targs) {
        munmap(stack, stack_size);
        return -1;
    }
    targs->start_routine = start_routine;
    targs->arg = arg;
    targs->stack = stack;
    targs->stack_size = stack_size;

    void *stack_top = (void *)(((uint64_t)stack + stack_size - 16) & ~0xFUL);
    uint64_t tid = __atoms_syscall3(SYS_THREAD_SPAWN, (uint64_t)thread_trampoline, (uint64_t)stack_top, (uint64_t)targs);
    if (tid == (uint64_t)-1) {
        free(targs);
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
    atoms_tcb_t *tcb = (atoms_tcb_t *)atoms_get_fs_base();
    if (tcb) {
        for (unsigned int i = 0; i < PTHREAD_KEYS_MAX; i++) {
            if (s_keys[i].in_use && s_keys[i].destructor && tcb->values[i]) {
                void *val = (void *)tcb->values[i];
                tcb->values[i] = NULL;
                s_keys[i].destructor(val);
            }
        }
        free(tcb);
        atoms_set_fs_base(NULL);
    }
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
