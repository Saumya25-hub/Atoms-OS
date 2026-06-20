#include "kernel/keyboard/include/keyboard.h"
#include "kernel/interrupt/include/irq.h"
#include "drivers/input/ps2/ps2.h"
#include "kernel/display/display.h"
#include <stddef.h>

static KeyboardDriver* active_driver = NULL;
static void (*key_callback)(char) = NULL;

// Basic US QWERTY Scancode to ASCII map (Set 1)
static const char scancode_to_ascii[] = {
    0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    '-', 0, 0, 0, '+', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static void keyboard_irq_handler(registers_t* regs) {
    (void)regs;
    
    if (active_driver && active_driver->read_scancode) {
        uint8_t scancode = active_driver->read_scancode();
        
        // Very basic press vs release check (MSB set means release in Set 1)
        if (!(scancode & 0x80)) {
            if (scancode < sizeof(scancode_to_ascii)) {
                char c = scancode_to_ascii[scancode];
                if (c != 0) {
                    if (key_callback) {
                        key_callback(c);
                    } else {
                        // Default behavior if no callback is registered: print it
                        char str[2] = {c, '\0'};
                        display_print(str);
                    }
                }
            }
        }
    }
}

void keyboard_init(void) {
    // Default internally to PS/2 Keyboard Driver
    active_driver = &ps2_keyboard_driver;
    
    if (active_driver && active_driver->init) {
        active_driver->init();
    }
    
    // Register the keyboard handler to IRQ 1 (Keyboard)
    irq_register_handler(1, keyboard_irq_handler);
}

void keyboard_register_callback(void (*callback)(char)) {
    key_callback = callback;
}

char keyboard_get_char(void) {
    return 0; // Reserved for future blocking read
}

uint8_t keyboard_key_pressed(void) {
    return 0; // Reserved for future non-blocking check
}
