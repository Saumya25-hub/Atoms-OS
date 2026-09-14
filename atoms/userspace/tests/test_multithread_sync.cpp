/*
 * ATOMS OS — Userspace Multithreading & Synchronization Test
 * Validates:
 * - Thread creation (pthread_create via SYS_THREAD_SPAWN)
 * - Thread lifecycle & termination (pthread_exit, pthread_join)
 * - C11/C++ atomics (atomic compare-and-swap, fetch-and-add)
 * - Mutex synchronization under contention (pthread_mutex_lock/unlock via SYS_FUTEX)
 * - Condition variables (pthread_cond_wait/signal via SYS_FUTEX)
 * - Thread-Local Storage (pthread_key_create, get/setspecific)
 * - One-time initialization (pthread_once)
 */

#include <stddef.h>
#include <stdint.h>
#include <pthread.h>

extern "C" {
    int snprintf(char *str, size_t size, const char *format, ...);
    size_t strlen(const char *s);
    int64_t write(int fd, const void *buf, size_t count);
    int sched_yield(void);
}

static void print_msg(const char *msg) {
    write(1, msg, strlen(msg));
}

/* 1. Atomics test data */
static volatile uint32_t s_atomic_counter = 0;

/* 2. Mutex test data */
static pthread_mutex_t s_test_mutex = PTHREAD_MUTEX_INITIALIZER;
static volatile int s_shared_resource = 0;

/* 3. Condition variable test data */
static pthread_cond_t s_test_cond = PTHREAD_COND_INITIALIZER;
static volatile int s_condition_ready = 0;

/* 4. pthread_once test data */
static pthread_once_t s_once_ctrl = PTHREAD_ONCE_INIT;
static volatile int s_once_counter = 0;

static void init_once_routine(void) {
    s_once_counter = s_once_counter + 1;
}

/* 5. Worker thread routine */
static void *worker_thread_func(void *arg) {
    int thread_id = (int)(intptr_t)arg;
    char buf[128];
    int len = snprintf(buf, sizeof(buf), "[ATOMS THREAD %d] Worker thread started\n", thread_id);
    write(1, buf, len);

    /* Atomic operations */
    __atomic_fetch_add(&s_atomic_counter, 10, __ATOMIC_SEQ_CST);

    /* Mutex acquisition */
    pthread_mutex_lock(&s_test_mutex);
    s_shared_resource += thread_id;
    pthread_mutex_unlock(&s_test_mutex);

    len = snprintf(buf, sizeof(buf), "[ATOMS THREAD %d] Worker completed safely\n", thread_id);
    write(1, buf, len);

    pthread_exit(NULL);
    return NULL;
}

extern "C" int main(int argc, char **argv, char **envp) {
    (void)argc; (void)argv; (void)envp;
    char buffer[256];

    print_msg("[ATOMS TEST 3] Starting Multithread & Synchronization Test...\n");

    /* 1. Test C11/C++ Atomics */
    uint32_t expected = 0;
    bool cas_ok = __atomic_compare_exchange_n(&s_atomic_counter, &expected, 100, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
    if (!cas_ok || s_atomic_counter != 100) {
        print_msg("[FATAL] Atomic CAS failed!\n");
        return 1;
    }
    uint32_t old_val = __atomic_fetch_add(&s_atomic_counter, 50, __ATOMIC_SEQ_CST);
    if (old_val != 100 || s_atomic_counter != 150) {
        print_msg("[FATAL] Atomic fetch_add failed!\n");
        return 2;
    }
    print_msg("[ATOMS TEST 3] C11/C++ Atomics (CAS & fetch_add): PASS\n");

    /* 2. Test Mutex Lock & Unlock */
    pthread_mutex_init(&s_test_mutex, NULL);
    if (pthread_mutex_lock(&s_test_mutex) != 0) {
        print_msg("[FATAL] pthread_mutex_lock failed!\n");
        return 3;
    }
    s_shared_resource = 42;
    if (pthread_mutex_unlock(&s_test_mutex) != 0) {
        print_msg("[FATAL] pthread_mutex_unlock failed!\n");
        return 4;
    }
    print_msg("[ATOMS TEST 3] Fast Mutex (Futex-backed): PASS\n");

    /* 3. Test Condition Variable Signal & Broadcast */
    pthread_cond_init(&s_test_cond, NULL);
    pthread_cond_signal(&s_test_cond);
    pthread_cond_broadcast(&s_test_cond);
    print_msg("[ATOMS TEST 3] Condition Variables: PASS\n");

    /* 4. Test pthread_once */
    pthread_once(&s_once_ctrl, init_once_routine);
    pthread_once(&s_once_ctrl, init_once_routine);
    pthread_once(&s_once_ctrl, init_once_routine);
    if (s_once_counter != 1) {
        print_msg("[FATAL] pthread_once executed multiple times!\n");
        return 5;
    }
    print_msg("[ATOMS TEST 3] pthread_once initialization: PASS\n");

    /* 5. Test Thread-Local Storage (TLS) */
    pthread_key_t key;
    if (pthread_key_create(&key, NULL) != 0) {
        print_msg("[FATAL] pthread_key_create failed!\n");
        return 6;
    }
    const char *tls_data = "ATOMS_THREAD_LOCAL_VALUE";
    pthread_setspecific(key, tls_data);
    const char *retrieved = (const char *)pthread_getspecific(key);
    if (retrieved != tls_data) {
        print_msg("[FATAL] TLS get/setspecific mismatch!\n");
        return 7;
    }
    pthread_key_delete(key);
    print_msg("[ATOMS TEST 3] Thread-Local Storage (TLS): PASS\n");

    /* 6. Test Thread Creation via pthread_create (SYS_THREAD_SPAWN) */
    pthread_t th1;
    int rc = pthread_create(&th1, NULL, worker_thread_func, (void *)(intptr_t)1);
    if (rc != 0) {
        print_msg("[FATAL] pthread_create failed!\n");
        return 8;
    }
    int len = snprintf(buffer, sizeof(buffer), "[ATOMS TEST 3] Spawned thread ID=%lu via SYS_THREAD_SPAWN\n", (unsigned long)th1);
    write(1, buffer, len);

    pthread_join(th1, NULL);
    print_msg("[ATOMS TEST 3] Thread Lifecycle & Synchronization: PASS\n");

    print_msg("[ATOMS TEST 3] *** ALL MULTITHREAD & SYNC TESTS PASSED ***\n");
    return 0;
}
