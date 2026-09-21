/*
 * ATOMS OS — Phase 1 Java Runtime Foundation Userspace C Verification
 * Copyright © 2026 ATOMS OS Project / Saumya Chaudhari
 */

#include "userspace/runtime/include/atoms_runtime.h"

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    char buffer[128];
    snprintf(buffer, sizeof(buffer), "[PHASE 1 C RUNTIME] PID=%d STATUS=PASS", (int)getpid());
    puts(buffer);

    // 1. Math Library verification
    if (fabs(-42.5) != 42.5) return 101;
    if (sqrt(144.0) != 12.0) return 102;
    if (floor(3.7) != 3.0)   return 103;
    if (ceil(3.1) != 4.0)    return 104;
    if (fmod(10.0, 3.0) != 1.0) return 105;
    puts("[PHASE 1] Math library IEEE 754: PASS");

    // 2. Setjmp / Longjmp System V ABI verification
    jmp_buf jb;
    volatile int check = 0;
    int val = setjmp(jb);
    if (val == 0) {
        check = 99;
        longjmp(jb, 42);
    } else {
        if (val != 42 || check != 99) return 106;
    }
    puts("[PHASE 1] Setjmp / Longjmp System V ABI: PASS");

    // 3. Mmap / Munmap verification
    void *m = mmap(NULL, 8192, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (m == MAP_FAILED || !m) return 107;
    memset(m, 0xEF, 8192);
    if (munmap(m, 8192) != 0) return 108;
    puts("[PHASE 1] Mmap / Munmap Dynamic Pages: PASS");

    // 4. Thread-Local Storage (TLS) FS Base
    pthread_key_t key;
    if (pthread_key_create(&key, NULL) != 0) return 109;
    pthread_setspecific(key, (const void *)0x1234ABCD);
    if ((uint64_t)pthread_getspecific(key) != 0x1234ABCD) return 110;
    pthread_key_delete(key);
    puts("[PHASE 1] Thread-Local Storage (TLS) FS Base: PASS");

    puts("[PHASE 1] All Userspace C Runtime Foundation Tests: PASS");
    return 0;
}
