#include "console.h"
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

extern void com1_puts(const char *s);

// Static backend driver pointer - explicitly initialized to NULL
static BackendDriver* active_backend = NULL;

static inline bool is_canonical_addr(uint64_t addr) {
    uint64_t high = addr >> 48;
    return high == 0 || high == 0xFFFF;
}

static inline bool console_backend_valid(BackendDriver* b) {
    if (!b) return false;
    uint64_t addr = (uint64_t)b;
    if (!is_canonical_addr(addr)) return false;
    // Driver structure pointer must be within valid kernel memory space (1MB .. 1GB)
    if (addr < 0x100000ULL || addr > 0x40000000ULL) return false;
    return true;
}

static inline bool console_func_ptr_valid(void *fn) {
    if (!fn) return false;
    uint64_t addr = (uint64_t)fn;
    if (!is_canonical_addr(addr)) return false;
    // Function pointer must point into kernel code section (1MB .. 1GB)
    if (addr < 0x100000ULL || addr > 0x40000000ULL) return false;
    return true;
}

void console_set_backend(BackendDriver* backend) {
    com1_puts("[CONSOLE] console_set_backend called\r\n");
    if (!console_backend_valid(backend)) {
        com1_puts("[CONSOLE] console_set_backend: REJECTED INVALID BACKEND POINTER\r\n");
        return;
    }
    active_backend = backend;
    if (console_func_ptr_valid((void*)active_backend->init)) {
        active_backend->init();
    }
}

void console_draw_char(uint16_t x, uint16_t y, char c, uint8_t color) {
    if (!console_backend_valid(active_backend)) return;
    if (console_func_ptr_valid((void*)active_backend->draw_character)) {
        active_backend->draw_character(x, y, c, color);
    }
}

void console_update_cursor(uint16_t x, uint16_t y) {
    if (!console_backend_valid(active_backend)) return;
    if (console_func_ptr_valid((void*)active_backend->set_hardware_cursor)) {
        active_backend->set_hardware_cursor(x, y);
    }
}

void console_clear_all(uint8_t bg_color) {
    if (!console_backend_valid(active_backend)) return;
    if (console_func_ptr_valid((void*)active_backend->clear_memory)) {
        active_backend->clear_memory(bg_color);
    }
}

uint16_t console_get_width(void) {
    if (!console_backend_valid(active_backend)) {
        return 0;
    }
    if (console_func_ptr_valid((void*)active_backend->get_width)) {
        return active_backend->get_width();
    }
    return 0;
}

uint16_t console_get_height(void) {
    if (!console_backend_valid(active_backend)) {
        return 0;
    }
    if (console_func_ptr_valid((void*)active_backend->get_height)) {
        return active_backend->get_height();
    }
    return 0;
}
