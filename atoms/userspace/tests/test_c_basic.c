/*
 * ATOMS OS — Userspace C Runtime Basic Validation Test
 * Tests musl libc integration, dynamic memory, string manipulation, and I/O.
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* Standard libc declarations from musl */
extern int snprintf(char *str, size_t size, const char *format, ...);
extern void *malloc(size_t size);
extern void free(void *ptr);
extern void *calloc(size_t nmemb, size_t size);
extern void *realloc(void *ptr, size_t size);
extern void *memcpy(void *dest, const void *src, size_t n);
extern void *memset(void *s, int c, size_t n);
extern size_t strlen(const char *s);
extern int strcmp(const char *s1, const char *s2);
extern char *strstr(const char *haystack, const char *needle);
extern int64_t write(int fd, const void *buf, size_t count);

static void test_print(const char *msg) {
    write(1, msg, strlen(msg));
}

int main(int argc, char **argv, char **envp) {
    (void)argc; (void)argv; (void)envp;
    char buffer[256];

    test_print("[ATOMS TEST 1] Starting C Runtime Validation Test...\n");

    /* 1. Test musl snprintf formatting */
    int len = snprintf(buffer, sizeof(buffer), "[ATOMS TEST 1] musl snprintf: val=%d, hex=0x%x, str=%s\n", 42, 0xABCD, "CERTIFIED");
    if (len <= 0) {
        test_print("[FATAL] snprintf failed!\n");
        return 1;
    }
    write(1, buffer, len);

    /* 2. Test musl string functions */
    const char *text = "ATOMS OS Userspace C Runtime";
    if (strlen(text) != 28) {
        test_print("[FATAL] strlen mismatch!\n");
        return 2;
    }
    if (strcmp(text, "ATOMS OS Userspace C Runtime") != 0) {
        test_print("[FATAL] strcmp mismatch!\n");
        return 3;
    }
    if (strstr(text, "Userspace") == NULL) {
        test_print("[FATAL] strstr failed!\n");
        return 4;
    }
    test_print("[ATOMS TEST 1] musl string operations: PASS\n");

    /* 3. Test userspace heap allocation */
    uint32_t *p1 = (uint32_t *)malloc(1024);
    if (!p1) {
        test_print("[FATAL] malloc(1024) returned NULL!\n");
        return 5;
    }
    for (int i = 0; i < 256; i++) {
        p1[i] = (uint32_t)(i * 7);
    }
    for (int i = 0; i < 256; i++) {
        if (p1[i] != (uint32_t)(i * 7)) {
            test_print("[FATAL] Memory pattern verification failed!\n");
            return 6;
        }
    }
    test_print("[ATOMS TEST 1] malloc & memory integrity: PASS\n");

    /* 4. Test realloc */
    uint32_t *p2 = (uint32_t *)realloc(p1, 2048);
    if (!p2) {
        test_print("[FATAL] realloc returned NULL!\n");
        return 7;
    }
    for (int i = 0; i < 256; i++) {
        if (p2[i] != (uint32_t)(i * 7)) {
            test_print("[FATAL] realloc preserved data verification failed!\n");
            return 8;
        }
    }
    free(p2);
    test_print("[ATOMS TEST 1] realloc & free: PASS\n");

    /* 5. Test calloc */
    uint8_t *zero_buf = (uint8_t *)calloc(64, sizeof(uint8_t));
    if (!zero_buf) {
        test_print("[FATAL] calloc returned NULL!\n");
        return 9;
    }
    for (int i = 0; i < 64; i++) {
        if (zero_buf[i] != 0) {
            test_print("[FATAL] calloc memory not zeroed!\n");
            return 10;
        }
    }
    free(zero_buf);
    test_print("[ATOMS TEST 1] calloc zero-initialization: PASS\n");

    test_print("[ATOMS TEST 1] *** ALL C RUNTIME TESTS PASSED ***\n");
    return 0;
}
