#include "render_audit.h"

extern void serial_write_direct(const char* str);

static void write_hex(uint32_t val) {
    char buf[16];
    char hex[] = "0123456789ABCDEF";
    buf[0] = '0'; buf[1] = 'x';
    for (int i = 0; i < 8; i++) {
        buf[9 - i] = hex[val & 0xF];
        val >>= 4;
    }
    buf[10] = '\0';
    serial_write_direct(buf);
}

static void write_int(int val) {
    char buf[16];
    if (val == 0) {
        serial_write_direct("0");
        return;
    }
    if (val < 0) {
        serial_write_direct("-");
        val = -val;
    }
    int i = 0;
    while (val > 0) {
        buf[i++] = '0' + (val % 10);
        val /= 10;
    }
    char rev[16];
    for (int j = 0; j < i; j++) {
        rev[j] = buf[i - j - 1];
    }
    rev[i] = '\0';
    serial_write_direct(rev);
}

static int s_log_count = 0;

void audit_log_draw(const char* func, int x, int y, int w, int h, int r, uint32_t color) {
    // Only log calls that overlap the top 50 pixels (y < 50 or y-r < 50)
    // To limit output volume.
    if ((y - r) >= 50 && y >= 50) return;
    
    // Stop logging after 500 lines to avoid spam
    if (s_log_count > 500) return;
    s_log_count++;

    serial_write_direct("[AUDIT] ");
    serial_write_direct(func);
    serial_write_direct(" X="); write_int(x);
    serial_write_direct(" Y="); write_int(y);
    serial_write_direct(" W="); write_int(w);
    serial_write_direct(" H="); write_int(h);
    serial_write_direct(" R="); write_int(r);
    serial_write_direct(" C="); write_hex(color);
    serial_write_direct("\n");
}


