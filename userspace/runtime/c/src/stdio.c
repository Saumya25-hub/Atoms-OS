/*
 * ATOMS OS — Userspace Formatted I/O (printf/puts/snprintf)
 * Adapted from musl libc (MIT License)
 */

#include "../include/stdio.h"
#include "../include/string.h"
#include "../include/unistd.h"

static void format_number(char **buf, size_t *rem, uint64_t val, int base, int uppercase) {
    char temp[64];
    int idx = 0;
    const char *digits = uppercase ? "0123456789ABCDEF" : "0123456789abcdef";

    if (val == 0) {
        temp[idx++] = '0';
    } else {
        while (val > 0) {
            temp[idx++] = digits[val % (uint64_t)base];
            val /= (uint64_t)base;
        }
    }

    while (idx > 0 && *rem > 1) {
        **buf = temp[--idx];
        (*buf)++;
        (*rem)--;
    }
}

int vsnprintf(char *str, size_t size, const char *format, va_list ap) {
    if (!str || size == 0) return 0;

    char *out = str;
    size_t rem = size;
    const char *p = format;

    while (*p && rem > 1) {
        if (*p != '%') {
            *out++ = *p++;
            rem--;
            continue;
        }

        p++; /* Skip '%' */
        if (*p == '%') {
            *out++ = '%';
            rem--;
            p++;
            continue;
        }

        /* Check for 'l' or 'll' */
        int is_long = 0;
        if (*p == 'l') {
            is_long = 1;
            p++;
            if (*p == 'l') {
                is_long = 2;
                p++;
            }
        } else if (*p == 'z') {
            is_long = 2;
            p++;
        }

        switch (*p) {
        case 's': {
            const char *s = va_arg(ap, const char *);
            if (!s) s = "(null)";
            while (*s && rem > 1) {
                *out++ = *s++;
                rem--;
            }
            break;
        }
        case 'c': {
            char c = (char)va_arg(ap, int);
            if (rem > 1) {
                *out++ = c;
                rem--;
            }
            break;
        }
        case 'd':
        case 'i': {
            int64_t val = is_long ? va_arg(ap, int64_t) : va_arg(ap, int);
            if (val < 0) {
                if (rem > 1) {
                    *out++ = '-';
                    rem--;
                }
                val = -val;
            }
            format_number(&out, &rem, (uint64_t)val, 10, 0);
            break;
        }
        case 'u': {
            uint64_t val = is_long ? va_arg(ap, uint64_t) : va_arg(ap, unsigned int);
            format_number(&out, &rem, val, 10, 0);
            break;
        }
        case 'x': {
            uint64_t val = is_long ? va_arg(ap, uint64_t) : va_arg(ap, unsigned int);
            format_number(&out, &rem, val, 16, 0);
            break;
        }
        case 'X': {
            uint64_t val = is_long ? va_arg(ap, uint64_t) : va_arg(ap, unsigned int);
            format_number(&out, &rem, val, 16, 1);
            break;
        }
        case 'p': {
            uint64_t val = (uint64_t)va_arg(ap, void *);
            if (rem > 3) {
                *out++ = '0';
                *out++ = 'x';
                rem -= 2;
                format_number(&out, &rem, val, 16, 0);
            }
            break;
        }
        default:
            if (rem > 1) {
                *out++ = *p;
                rem--;
            }
            break;
        }
        p++;
    }

    *out = '\0';
    return (int)(out - str);
}

int snprintf(char *str, size_t size, const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    int res = vsnprintf(str, size, format, ap);
    va_end(ap);
    return res;
}

int sprintf(char *str, const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    int res = vsnprintf(str, 1024 * 1024, format, ap);
    va_end(ap);
    return res;
}

int printf(const char *format, ...) {
    char buf[1024];
    va_list ap;
    va_start(ap, format);
    int len = vsnprintf(buf, sizeof(buf), format, ap);
    va_end(ap);
    if (len > 0) {
        write(1, buf, (size_t)len);
    }
    return len;
}

int puts(const char *s) {
    if (!s) return -1;
    size_t len = strlen(s);
    write(1, s, len);
    write(1, "\n", 1);
    return (int)(len + 1);
}

int putchar(int c) {
    char ch = (char)c;
    write(1, &ch, 1);
    return c;
}

