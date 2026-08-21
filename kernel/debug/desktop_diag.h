#ifndef DESKTOP_DIAG_H
#define DESKTOP_DIAG_H

#include <stdint.h>
#include <stdbool.h>

extern void com1_puts(const char* s);

static inline void diag_puts(const char* s) {
    if (s) com1_puts(s);
}

static inline void diag_put_hex32(uint32_t val) {
    const char hex[] = "0123456789ABCDEF";
    diag_puts("0x");
    for (int i = 28; i >= 0; i -= 4) {
        char c[2] = { hex[(val >> i) & 0xF], '\0' };
        diag_puts(c);
    }
}

static inline void diag_put_hex64(uint64_t val) {
    const char hex[] = "0123456789ABCDEF";
    diag_puts("0x");
    for (int i = 60; i >= 0; i -= 4) {
        char c[2] = { hex[(val >> i) & 0xF], '\0' };
        diag_puts(c);
    }
}

static inline void diag_put_dec(int64_t val) {
    if (val < 0) {
        diag_puts("-");
        val = -val;
    }
    if (val == 0) {
        diag_puts("0");
        return;
    }
    char buf[32];
    int pos = 0;
    while (val > 0) {
        buf[pos++] = '0' + (val % 10);
        val /= 10;
    }
    for (int i = 0; i < pos / 2; i++) {
        char tmp = buf[i];
        buf[i] = buf[pos - 1 - i];
        buf[pos - 1 - i] = tmp;
    }
    buf[pos] = '\0';
    diag_puts(buf);
}

#endif /* DESKTOP_DIAG_H */
