/*
 * ATOMS OS — Userspace Thread-Local Storage (TLS) Adapter
 * Implements pthread_key_create, pthread_setspecific, pthread_getspecific, pthread_key_delete
 * using real per-thread TCB via IA32_FS_BASE (SYS_SET_FS_BASE).
 */

#include <stdint.h>
#include <stddef.h>

#define PTHREAD_KEYS_MAX 128
#define SYS_SET_FS_BASE  44U
#define SYS_GET_FS_BASE  45U

typedef unsigned int pthread_key_t;

typedef struct {
    int in_use;
    void (*destructor)(void *);
} KeyDescriptor;

typedef struct {
    void *self;
    const void *values[PTHREAD_KEYS_MAX];
} atoms_tls_tcb_t;

extern void *malloc(size_t size);
extern void free(void *ptr);
extern void *memset(void *s, int c, size_t n);

static KeyDescriptor s_keys[PTHREAD_KEYS_MAX];
static volatile int s_tls_lock = 0;

static inline uint64_t get_fs_base(void) {
    uint64_t ret;
    __asm__ volatile("syscall" : "=a"(ret) : "a"((uint64_t)SYS_GET_FS_BASE) : "rcx", "r11", "memory");
    return ret;
}

static inline void set_fs_base(uint64_t base) {
    __asm__ volatile("syscall" : : "a"((uint64_t)SYS_SET_FS_BASE), "D"(base) : "rcx", "r11", "memory");
}

static atoms_tls_tcb_t *ensure_tcb(void) {
    atoms_tls_tcb_t *tcb = (atoms_tls_tcb_t *)get_fs_base();
    if (!tcb) {
        tcb = (atoms_tls_tcb_t *)malloc(sizeof(atoms_tls_tcb_t));
        if (!tcb) return NULL;
        memset(tcb, 0, sizeof(atoms_tls_tcb_t));
        tcb->self = tcb;
        set_fs_base((uint64_t)tcb);
    }
    return tcb;
}

int pthread_key_create(pthread_key_t *key, void (*destructor)(void *)) {
    if (!key) return -1;

    while (__atomic_test_and_set(&s_tls_lock, __ATOMIC_ACQUIRE)) {}

    for (unsigned int i = 0; i < PTHREAD_KEYS_MAX; i++) {
        if (!s_keys[i].in_use) {
            s_keys[i].in_use = 1;
            s_keys[i].destructor = destructor;
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
    atoms_tls_tcb_t *tcb = ensure_tcb();
    if (!tcb) return -1;
    tcb->values[key] = value;
    return 0;
}

void *pthread_getspecific(pthread_key_t key) {
    if (key >= PTHREAD_KEYS_MAX || !s_keys[key].in_use) {
        return NULL;
    }
    atoms_tls_tcb_t *tcb = (atoms_tls_tcb_t *)get_fs_base();
    if (!tcb) return NULL;
    return (void *)tcb->values[key];
}

int pthread_key_delete(pthread_key_t key) {
    if (key >= PTHREAD_KEYS_MAX) return -1;

    while (__atomic_test_and_set(&s_tls_lock, __ATOMIC_ACQUIRE)) {}
    s_keys[key].in_use = 0;
    s_keys[key].destructor = NULL;
    __atomic_clear(&s_tls_lock, __ATOMIC_RELEASE);
    return 0;
}
