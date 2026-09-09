/*
 * ATOMS OS — Userspace Thread-Local Storage (TLS) Adapter
 * Implements pthread_key_create, pthread_setspecific, pthread_getspecific, pthread_key_delete.
 */

#include <stdint.h>
#include <stddef.h>

#define PTHREAD_KEYS_MAX 128

typedef unsigned int pthread_key_t;

typedef struct {
    int in_use;
    void (*destructor)(void *);
} KeyDescriptor;

static KeyDescriptor s_keys[PTHREAD_KEYS_MAX];
static const void *s_thread_specific_values[PTHREAD_KEYS_MAX];
static volatile int s_tls_lock = 0;

int pthread_key_create(pthread_key_t *key, void (*destructor)(void *)) {
    if (!key) return -1;

    while (__atomic_test_and_set(&s_tls_lock, __ATOMIC_ACQUIRE)) {}

    for (unsigned int i = 0; i < PTHREAD_KEYS_MAX; i++) {
        if (!s_keys[i].in_use) {
            s_keys[i].in_use = 1;
            s_keys[i].destructor = destructor;
            s_thread_specific_values[i] = NULL;
            *key = i;
            __atomic_clear(&s_tls_lock, __ATOMIC_RELEASE);
            return 0;
        }
    }

    __atomic_clear(&s_tls_lock, __ATOMIC_RELEASE);
    return -1; /* Out of keys */
}

int pthread_setspecific(pthread_key_t key, const void *value) {
    if (key >= PTHREAD_KEYS_MAX || !s_keys[key].in_use) {
        return -1;
    }
    s_thread_specific_values[key] = value;
    return 0;
}

void *pthread_getspecific(pthread_key_t key) {
    if (key >= PTHREAD_KEYS_MAX || !s_keys[key].in_use) {
        return NULL;
    }
    return (void *)s_thread_specific_values[key];
}

int pthread_key_delete(pthread_key_t key) {
    if (key >= PTHREAD_KEYS_MAX) return -1;

    while (__atomic_test_and_set(&s_tls_lock, __ATOMIC_ACQUIRE)) {}
    s_keys[key].in_use = 0;
    s_keys[key].destructor = NULL;
    s_thread_specific_values[key] = NULL;
    __atomic_clear(&s_tls_lock, __ATOMIC_RELEASE);
    return 0;
}
