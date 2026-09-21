/*
 * ATOMS OS — Userspace C Runtime pthread.h
 * Adapted from musl libc pthread interface (MIT License)
 */

#ifndef ATOMS_USER_PTHREAD_H
#define ATOMS_USER_PTHREAD_H

#include <stddef.h>
#include <stdint.h>
#include "time.h"


#ifdef __cplusplus
extern "C" {
#endif

typedef uint64_t pthread_t;

typedef struct {
    void   *stack_addr;
    size_t  stack_size;
    int     detach_state;
} pthread_attr_t;

typedef struct {
    volatile int lock_val;
    uint32_t     owner_tid;
    uint32_t     recursion_count;
} pthread_mutex_t;

typedef struct {
    int dummy;
} pthread_mutexattr_t;

#define PTHREAD_MUTEX_INITIALIZER { 0, 0, 0 }

typedef struct {
    volatile uint32_t seq;
} pthread_cond_t;

typedef struct {
    int dummy;
} pthread_condattr_t;

#define PTHREAD_COND_INITIALIZER { 0 }

int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine)(void *), void *arg);
int pthread_join(pthread_t thread, void **retval);
int pthread_detach(pthread_t thread);
void pthread_exit(void *retval);
pthread_t pthread_self(void);

int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr);
int pthread_mutex_destroy(pthread_mutex_t *mutex);
int pthread_mutex_lock(pthread_mutex_t *mutex);
int pthread_mutex_trylock(pthread_mutex_t *mutex);
int pthread_mutex_unlock(pthread_mutex_t *mutex);

int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr);
int pthread_cond_destroy(pthread_cond_t *cond);
int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex);
int pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex, const struct timespec *abstime);
int pthread_cond_signal(pthread_cond_t *cond);
int pthread_cond_broadcast(pthread_cond_t *cond);

#define PTHREAD_KEYS_MAX 64
typedef unsigned int pthread_key_t;

int pthread_key_create(pthread_key_t *key, void (*destructor)(void *));
int pthread_key_delete(pthread_key_t key);
int pthread_setspecific(pthread_key_t key, const void *value);
void *pthread_getspecific(pthread_key_t key);

void atoms_set_fs_base(void *base);
void *atoms_get_fs_base(void);


#ifdef __cplusplus
}
#endif

#endif /* ATOMS_USER_PTHREAD_H */
