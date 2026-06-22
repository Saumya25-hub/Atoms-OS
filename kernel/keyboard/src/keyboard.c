#include "kernel/keyboard/include/keyboard.h"
#include "kernel/interrupt/include/irq.h"
#include "drivers/input/ps2/ps2.h"
#include "kernel/display/display.h"
#include <stddef.h>
#include <stdbool.h>

static KeyboardDriver* active_driver = NULL;
static void (*key_callback)(KeyboardEvent*) = NULL;

static bool shift_pressed = false;
static bool ctrl_pressed = false;
static bool alt_pressed = false;

#define KBD_BUF_SIZE 256
static char kbd_buffer[KBD_BUF_SIZE];
static volatile uint32_t kbd_buf_head = 0;
static volatile uint32_t kbd_buf_tail = 0;

// Basic US QWERTY Scancode to ASCII map (Set 1)
static const char scancode_to_ascii[] = {
    0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    '-', 0, 0, 0, '+', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static uint64_t keyboard_irq_handler(registers_t* regs) {
    (void)regs;
    
    if (active_driver && active_driver->read_scancode) {
        uint8_t scancode = active_driver->read_scancode();
        bool pressed = !(scancode & 0x80);
        uint8_t raw_scancode = scancode & 0x7F; // Clear the break bit
        
        // Very basic modifier tracking (Left Shift = 0x2A, Right Shift = 0x36)
        if (raw_scancode == 0x2A || raw_scancode == 0x36) {
            shift_pressed = pressed;
        } else if (raw_scancode == 0x1D) {
            ctrl_pressed = pressed;
        } else if (raw_scancode == 0x38) {
            alt_pressed = pressed;
        }
        
        char ascii = 0;
        if (raw_scancode < sizeof(scancode_to_ascii)) {
            ascii = scancode_to_ascii[raw_scancode];
            // Basic uppercase handling
            if (shift_pressed && ascii >= 'a' && ascii <= 'z') {
                ascii -= 32;
            }
        }
        
        KeyboardEvent event = {
            .scancode = raw_scancode,
            .ascii = ascii,
            .pressed = pressed,
            .shift = shift_pressed,
            .ctrl = ctrl_pressed,
            .alt = alt_pressed
        };
        
        if (key_callback) {
            key_callback(&event);
        } else if (pressed && ascii != 0) {
            // Push to ring buffer
            uint32_t next_head = (kbd_buf_head + 1) % KBD_BUF_SIZE;
            if (next_head != kbd_buf_tail) {
                kbd_buffer[kbd_buf_head] = ascii;
                kbd_buf_head = next_head;
            }
        }
    }
    return 0;
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

void keyboard_register_callback(void (*callback)(KeyboardEvent* event)) {
    key_callback = callback;
}

char keyboard_getc(void) {
    // Enable interrupts so IRQ1 can fire while we wait
    __asm__ volatile("sti");
    
    while (kbd_buf_tail == kbd_buf_head) {
        // Yield the CPU to other tasks instead of spinning directly
        extern void scheduler_yield(void);
        scheduler_yield();
    }
    
    // Disable interrupts while reading the queue
    __asm__ volatile("cli");
    char c = kbd_buffer[kbd_buf_tail];
    kbd_buf_tail = (kbd_buf_tail + 1) % KBD_BUF_SIZE;
    return c;
}
