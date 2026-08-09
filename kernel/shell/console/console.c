#include "console.h"
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

extern void com1_puts(const char *s);

static BackendDriver* active_backend = NULL;

static inline bool is_canonical(uintptr_t addr) {
    uintptr_t high = addr >> 48;
    return high == 0 || high == 0xFFFF;
}

static inline bool is_valid_func_ptr(void *fn) {
    if (!fn) return false;
    uintptr_t addr = (uintptr_t)fn;
    if (!is_canonical(addr)) return false;
    // Function pointer must be in lower 2GB physical/virtual kernel code region
    if (addr < 0x100000ULL || addr > 0x40000000ULL) return false;
    return true;
}

static inline bool is_valid_backend(BackendDriver* drv) {
    if (!drv) return false;
    uintptr_t addr = (uintptr_t)drv;
    if (!is_canonical(addr)) return false;
    if (addr < 0x100000ULL || addr > 0x40000000ULL) return false;
    return true;
}

void console_set_backend(BackendDriver* backend) {
    if (!is_valid_backend(backend)) {
        com1_puts("[CONSOLE] console_set_backend: INVALID BACKEND POINTER\r\n");
        return;
    }
    active_backend = backend;
    if (is_valid_func_ptr((void*)active_backend->init)) {
        active_backend->init();
    }
}

void console_draw_char(uint16_t x, uint16_t y, char c, uint8_t color) {
    if (!is_valid_backend(active_backend)) return;
    if (is_valid_func_ptr((void*)active_backend->draw_character)) {
        active_backend->draw_character(x, y, c, color);
    }
}

void console_update_cursor(uint16_t x, uint16_t y) {
    if (!is_valid_backend(active_backend)) return;
    if (is_valid_func_ptr((void*)active_backend->set_hardware_cursor)) {
        active_backend->set_hardware_cursor(x, y);
    }
}

void console_clear_all(uint8_t bg_color) {
    if (!is_valid_backend(active_backend)) return;
    if (is_valid_func_ptr((void*)active_backend->clear_memory)) {
        active_backend->clear_memory(bg_color);
    }
}

uint16_t console_get_width(void) {
    if (!is_valid_backend(active_backend)) return 0;
    if (is_valid_func_ptr((void*)active_backend->get_width)) {
        return active_backend->get_width();
    }
    return 0;
}

uint16_t console_get_height(void) {
    if (!is_valid_backend(active_backend)) return 0;
    if (is_valid_func_ptr((void*)active_backend->get_height)) {
        return active_backend->get_height();
    }
    return 0;
}
