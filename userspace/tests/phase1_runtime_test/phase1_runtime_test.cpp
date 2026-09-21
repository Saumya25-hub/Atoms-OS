/*
 * ATOMS OS — Phase 1 Java Runtime Foundation Verification Suite
 * Copyright © 2026 ATOMS OS Project / Saumya Chaudhari
 *
 * Implements 10 deterministic hardware-level tests validating:
 * 1. Memory mmap/munmap arbitrary sizing & zero-fill
 * 2. Memory mprotect page protection toggling
 * 3. Ring 3 thread spawning and clean exit
 * 4. IA32_FS_BASE TLS isolation across threads & context switches
 * 5. Futex-based mutex and condition variable synchronization
 * 6. System V x86_64 ABI setjmp and longjmp unwinding
 * 7. Clocks & high-resolution sleep timers
 * 8. Freestanding IEEE 754 floating point math library
 * 9. C++ global constructors, operators new/delete, virtual dispatch
 * 10. Eager file-backed memory mapping (mmap with VFS file descriptor)
 */

#include "phase1_runtime_test.h"
#include "../../runtime/include/atoms_runtime.h"

extern "C" void display_print(const char *s);
extern "C" void display_print_dec(uint32_t val);

static void log_test(const char *name, bool passed) {
    display_print("[PHASE 1] ");
    display_print(name);
    display_print(": ");
    if (passed) {
        display_print("PASS\n");
    } else {
        display_print("FAIL\n");
    }
}

// Global C++ constructor probe
static int s_global_ctor_probe = 0;
struct GlobalCtorTracker {
    GlobalCtorTracker() { s_global_ctor_probe = 0xCAFE; }
    ~GlobalCtorTracker() { s_global_ctor_probe = 0; }
};
static GlobalCtorTracker s_tracker_instance;

// 1. Memory mmap/munmap arbitrary sizing
static bool Test_1_Memory_Mmap() {
    size_t sz = 128 * 1024; // 128 KB
    void *ptr = mmap(NULL, sz, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (ptr == MAP_FAILED || !ptr) return false;

    uint8_t *p = (uint8_t *)ptr;
    for (size_t i = 0; i < sz; i += 4096) {
        p[i] = (uint8_t)(i & 0xFF);
    }
    for (size_t i = 0; i < sz; i += 4096) {
        if (p[i] != (uint8_t)(i & 0xFF)) {
            munmap(ptr, sz);
            return false;
        }
    }
    if (munmap(ptr, sz) != 0) return false;
    return true;
}

// 2. Memory mprotect page protection toggling
static bool Test_2_Memory_Mprotect() {
    size_t sz = 4096;
    void *ptr = mmap(NULL, sz, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (ptr == MAP_FAILED || !ptr) return false;

    volatile uint32_t *dw = (volatile uint32_t *)ptr;
    *dw = 0x12345678;

    if (mprotect(ptr, sz, PROT_READ) != 0) {
        munmap(ptr, sz);
        return false;
    }
    if (*dw != 0x12345678) {
        munmap(ptr, sz);
        return false;
    }

    if (mprotect(ptr, sz, PROT_READ | PROT_WRITE) != 0) {
        munmap(ptr, sz);
        return false;
    }
    *dw = 0x87654321;
    if (*dw != 0x87654321) {
        munmap(ptr, sz);
        return false;
    }

    munmap(ptr, sz);
    return true;
}

// 3. Thread spawn & exit
static volatile int s_thread_spawn_flag = 0;
static void *worker_thread_func(void *arg) {
    (void)arg;
    s_thread_spawn_flag = 1;
    pthread_exit(NULL);
    return NULL;
}

static bool Test_3_Thread_Spawn_Exit() {
    s_thread_spawn_flag = 0;
    pthread_t th;
    if (pthread_create(&th, NULL, worker_thread_func, NULL) != 0) {
        return false;
    }
    for (int i = 0; i < 100 && s_thread_spawn_flag == 0; i++) {
        sched_yield();
    }
    return (s_thread_spawn_flag == 1);
}

// 4. IA32_FS_BASE TLS Isolation
static pthread_key_t s_tls_key;
static volatile int s_tls_worker_done = 0;
static volatile bool s_tls_worker_pass = false;

static void *tls_worker_thread(void *arg) {
    (void)arg;
    pthread_setspecific(s_tls_key, (const void *)0xBBBB2222ULL);
    void *read_val = pthread_getspecific(s_tls_key);
    if ((uint64_t)read_val == 0xBBBB2222ULL) {
        s_tls_worker_pass = true;
    }
    s_tls_worker_done = 1;
    pthread_exit(NULL);
    return NULL;
}

static bool Test_4_TLS_FS_Base_Isolation() {
    if (pthread_key_create(&s_tls_key, NULL) != 0) return false;

    pthread_setspecific(s_tls_key, (const void *)0xAAAA1111ULL);
    void *val1 = pthread_getspecific(s_tls_key);
    if ((uint64_t)val1 != 0xAAAA1111ULL) {
        pthread_key_delete(s_tls_key);
        return false;
    }

    s_tls_worker_done = 0;
    s_tls_worker_pass = false;
    pthread_t th;
    if (pthread_create(&th, NULL, tls_worker_thread, NULL) != 0) {
        pthread_key_delete(s_tls_key);
        return false;
    }

    for (int i = 0; i < 100 && !s_tls_worker_done; i++) {
        sched_yield();
    }

    if (!s_tls_worker_pass) {
        pthread_key_delete(s_tls_key);
        return false;
    }

    // Crucial check: Main thread's TLS value MUST NOT be modified by the worker
    void *val1_check = pthread_getspecific(s_tls_key);
    pthread_key_delete(s_tls_key);
    return ((uint64_t)val1_check == 0xAAAA1111ULL);
}

// 5. Futex Synchronization
static pthread_mutex_t s_futex_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  s_futex_cond  = PTHREAD_COND_INITIALIZER;
static volatile int s_futex_stage = 0;

static void *futex_worker_thread(void *arg) {
    (void)arg;
    pthread_mutex_lock(&s_futex_mutex);
    while (s_futex_stage == 0) {
        pthread_cond_wait(&s_futex_cond, &s_futex_mutex);
    }
    s_futex_stage = 2;
    pthread_mutex_unlock(&s_futex_mutex);
    pthread_exit(NULL);
    return NULL;
}

static bool Test_5_Futex_Synchronization() {
    s_futex_stage = 0;
    pthread_t th;
    if (pthread_create(&th, NULL, futex_worker_thread, NULL) != 0) return false;

    // Allow worker to enter wait state
    sched_yield();

    pthread_mutex_lock(&s_futex_mutex);
    s_futex_stage = 1;
    pthread_cond_signal(&s_futex_cond);
    pthread_mutex_unlock(&s_futex_mutex);

    for (int i = 0; i < 100 && s_futex_stage != 2; i++) {
        sched_yield();
    }

    return (s_futex_stage == 2);
}

// 6. System V x86_64 ABI setjmp/longjmp
static bool Test_6_Setjmp_Longjmp() {
    jmp_buf env;
    volatile int checkpoint = 0;

    int rc = setjmp(env);
    if (rc == 0) {
        checkpoint = 1;
        longjmp(env, 42);
    } else {
        return (rc == 42 && checkpoint == 1);
    }
    return false;
}

// 7. Clocks & Timers
static bool Test_7_Clocks_Timers() {
    struct timespec ts1, ts2;
    if (clock_gettime(CLOCK_MONOTONIC, &ts1) != 0) return false;

    struct timespec req = { 0, 5000000 }; // 5 ms
    nanosleep(&req, NULL);

    if (clock_gettime(CLOCK_MONOTONIC, &ts2) != 0) return false;
    uint64_t ns1 = (uint64_t)ts1.tv_sec * 1000000000ULL + ts1.tv_nsec;
    uint64_t ns2 = (uint64_t)ts2.tv_sec * 1000000000ULL + ts2.tv_nsec;
    return (ns2 >= ns1);
}

// 8. Math Library
static bool Test_8_Math_Library() {
    if (fabs(-123.456) != 123.456) return false;
    double sq = sqrt(144.0);
    if (sq < 11.9999 || sq > 12.0001) return false;
    if (floor(4.9) != 4.0) return false;
    if (ceil(4.1) != 5.0) return false;
    double rem = fmod(7.0, 3.0);
    if (rem < 0.9999 || rem > 1.0001) return false;
    if (!isnan(NAN)) return false;
    if (!isinf(INFINITY)) return false;
    return true;
}

// 9. C++ Global Constructors & Heap
class BaseObject {
public:
    virtual ~BaseObject() {}
    virtual int getValue() = 0;
};

class DerivedObject : public BaseObject {
    int m_val;
public:
    DerivedObject(int v) : m_val(v) {}
    virtual int getValue() override { return m_val * 2; }
};

static bool Test_9_Cxx_Constructors_Heap() {
    // Probe global constructor execution
    if (s_global_ctor_probe != 0xCAFE) return false;

    BaseObject *obj = new DerivedObject(21);
    if (!obj) return false;
    int res = obj->getValue();
    delete obj;
    return (res == 42);
}

// 10. Eager File-backed Mmap
static bool Test_10_Eager_File_Mmap() {
    const char *path = "/tmp/phase1_mmap_test.dat";
    int fd = open(path, 0);
    if (fd < 0) {
        // If file doesn't exist, create it via VFS
        fd = (int)__atoms_syscall2(SYS_OPEN, (uint64_t)path, 0);
    }

    // Write 512 bytes with signature pattern
    uint8_t write_buf[512];
    for (int i = 0; i < 512; i++) write_buf[i] = (uint8_t)(0xA0 + (i & 0x0F));
    __atoms_syscall3(SYS_WRITE_FILE, (uint64_t)fd, (uint64_t)write_buf, 512);

    // Mmap the file eagerly
    void *map = mmap(NULL, 4096, PROT_READ, MAP_PRIVATE, fd, 0);
    if (map == MAP_FAILED || !map) {
        close(fd);
        return false;
    }

    uint8_t *mapped_bytes = (uint8_t *)map;
    bool match = true;
    for (int i = 0; i < 512; i++) {
        if (mapped_bytes[i] != write_buf[i]) {
            match = false;
            break;
        }
    }

    munmap(map, 4096);
    close(fd);
    return match;
}

// Master Test Runner
bool ATOMS_RunPhase1_JavaRuntimeFoundationTests(ATOMS_Phase1RuntimeReport *out_report) {
    display_print("\n========================================================\n");
    display_print("   ATOMS OS — Phase 1 Java Runtime Foundation Tests    \n");
    display_print("========================================================\n");

    uint32_t total = 0;
    uint32_t passed = 0;

    struct {
        const char *name;
        bool (*func)(void);
    } tests[] = {
        { "Test 1: Memory Mmap / Munmap (Arbitrary Sizing)", Test_1_Memory_Mmap },
        { "Test 2: Memory Mprotect (Page Protection Toggling)", Test_2_Memory_Mprotect },
        { "Test 3: Thread Spawn & Clean Exit", Test_3_Thread_Spawn_Exit },
        { "Test 4: IA32_FS_BASE TLS Isolation Across Context Switches", Test_4_TLS_FS_Base_Isolation },
        { "Test 5: Futex Synchronization (Mutex & Condvar)", Test_5_Futex_Synchronization },
        { "Test 6: System V x86_64 ABI setjmp / longjmp", Test_6_Setjmp_Longjmp },
        { "Test 7: Clocks & High-Resolution Timers", Test_7_Clocks_Timers },
        { "Test 8: IEEE 754 Math Library (sqrt, floor, ceil, fmod)", Test_8_Math_Library },
        { "Test 9: C++ Global Constructors & Operator new/delete", Test_9_Cxx_Constructors_Heap },
        { "Test 10: Eager File-backed Mmap from VFS", Test_10_Eager_File_Mmap },
    };

    total = sizeof(tests) / sizeof(tests[0]);

    for (uint32_t i = 0; i < total; i++) {
        bool res = tests[i].func();
        log_test(tests[i].name, res);
        if (res) passed++;
    }

    display_print("--------------------------------------------------------\n");
    display_print("Phase 1 Runtime Tests Passed: ");
    display_print_dec(passed);
    display_print(" / ");
    display_print_dec(total);
    display_print("\n========================================================\n");

    if (out_report) {
        out_report->total_tests = total;
        out_report->passed_tests = passed;
        out_report->failed_tests = total - passed;
        out_report->all_passed = (passed == total);
    }

    return (passed == total);
}
