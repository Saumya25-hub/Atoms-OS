#include "console.h"
#include <stddef.h>
#include <stdbool.h>

static BackendDriver* active_backend = NULL;

static inline bool is_valid_backend(BackendDriver* drv) {
    if (!drv) return false;
    uintptr_t addr = (uintptr_t)drv;
    // Check if pointer is non-null and above lowest 4KB page
    if (addr < 0x1000) return false;
    return true;
}

void console_set_backend(BackendDriver* backend) {
    active_backend = backend;
    if (is_valid_backend(active_backend) && active_backend->init) {
        active_backend->init();
    }
}

void console_draw_char(uint16_t x, uint16_t y, char c, uint8_t color) {
    if (is_valid_backend(active_backend) && active_backend->draw_character) {
        active_backend->draw_character(x, y, c, color);
    }
}

void console_update_cursor(uint16_t x, uint16_t y) {
    if (is_valid_backend(active_backend) && active_backend->set_hardware_cursor) {
        active_backend->set_hardware_cursor(x, y);
    }
}

void console_clear_all(uint8_t bg_color) {
    if (is_valid_backend(active_backend) && active_backend->clear_memory) {
        active_backend->clear_memory(bg_color);
    }
}

uint16_t console_get_width(void) {
    if (is_valid_backend(active_backend) && active_backend->get_width) {
        return active_backend->get_width();
    }
    return 0;
}

uint16_t console_get_height(void) {
    if (is_valid_backend(active_backend) && active_backend->get_height) {
        return active_backend->get_height();
    }
    return 0;
}
