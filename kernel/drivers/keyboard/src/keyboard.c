#include "kernel/drivers/keyboard/include/keyboard.h"
#include "kernel/core/interrupt/include/irq.h"
#include "drivers/input/ps2/ps2.h"
#include "kernel/drivers/display/display.h"
#include <stddef.h>
#include <stdbool.h>

static KeyboardDriver* active_driver = NULL;
static void (*key_callback)(KeyboardEvent*) = NULL;

static bool shift_pressed = false;
static bool ctrl_pressed = false;
static bool alt_pressed = false;
static bool caps_lock_on = false;
static bool expect_e0 = false;

#define KBD_BUF_SIZE 256
static KeyboardEvent kbd_buffer[KBD_BUF_SIZE];
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

static const char scancode_to_ascii_shift[] = {
    0, 27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
    '*', 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    '-', 0, 0, 0, '+', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static uint64_t keyboard_irq_handler(registers_t* regs) {
    (void)regs;
    
    if (active_driver && active_driver->read_scancode) {
        uint8_t scancode = active_driver->read_scancode();
        
        if (scancode == 0xE0) {
            expect_e0 = true;
            return 0;
        }
        
        bool pressed = !(scancode & 0x80);
        uint8_t raw_scancode = scancode & 0x7F;
        
        uint8_t keycode = raw_scancode;
        char ascii = 0;
        
        if (expect_e0) {
            expect_e0 = false;
            // Handle extended keys
            switch (raw_scancode) {
                case 0x48: keycode = BOS_KEY_UP; break;
                case 0x50: keycode = BOS_KEY_DOWN; break;
                case 0x4B: keycode = BOS_KEY_LEFT; break;
                case 0x4D: keycode = BOS_KEY_RIGHT; break;
                case 0x47: keycode = BOS_KEY_HOME; break;
                case 0x4F: keycode = BOS_KEY_END; break;
                case 0x49: keycode = BOS_KEY_PGUP; break;
                case 0x51: keycode = BOS_KEY_PGDN; break;
                case 0x52: keycode = BOS_KEY_INS; break;
                case 0x53: keycode = BOS_KEY_DEL; break;
                case 0x1D: ctrl_pressed = pressed; keycode = BOS_KEY_CTRL; break; // Right Ctrl
                case 0x38: alt_pressed = pressed; keycode = BOS_KEY_ALT; break;  // Right Alt
            }
        } else {
            // Standard keys
            if (raw_scancode == 0x2A || raw_scancode == 0x36) {
                shift_pressed = pressed;
                keycode = BOS_KEY_SHIFT;
            } else if (raw_scancode == 0x1D) {
                ctrl_pressed = pressed; // Left Ctrl
                keycode = BOS_KEY_CTRL;
            } else if (raw_scancode == 0x38) {
                alt_pressed = pressed;  // Left Alt
                keycode = BOS_KEY_ALT;
            } else if (raw_scancode == 0x3A) { 
                if (pressed) caps_lock_on = !caps_lock_on; 
                return 0; 
            }
            if (raw_scancode == 0x45) { keycode = BOS_KEY_NUMLOCK; }

            // F1-F10
            if (raw_scancode >= 0x3B && raw_scancode <= 0x44) {
                keycode = BOS_KEY_F1 + (raw_scancode - 0x3B);
            } else if (raw_scancode == 0x57) {
                keycode = BOS_KEY_F11;
            } else if (raw_scancode == 0x58) {
                keycode = BOS_KEY_F12;
            } else if (raw_scancode == 0x01) {
                keycode = BOS_KEY_ESC;
            }
            
            if (raw_scancode < sizeof(scancode_to_ascii)) {
                bool apply_shift = shift_pressed;
                char base_ascii = scancode_to_ascii[raw_scancode];
                // If it's a letter, Caps Lock also applies shift logic
                if (base_ascii >= 'a' && base_ascii <= 'z') {
                    apply_shift = shift_pressed ^ caps_lock_on;
                }
                
                if (apply_shift) {
                    ascii = scancode_to_ascii_shift[raw_scancode];
                } else {
                    ascii = base_ascii;
                }
            }
        }
        
        KeyboardEvent event = {
            .keycode = keycode,
            .ascii = ascii,
            .pressed = pressed,
            .shift = shift_pressed,
            .ctrl = ctrl_pressed,
            .alt = alt_pressed,
            .caps_lock = caps_lock_on
        };
        
        if (key_callback) {
            key_callback(&event);
        } else {
            // Push to ring buffer (store both presses and releases if needed, but usually just presses for shell)
            // Actually, we should store all events so shell can handle KEY_UP/DOWN if it wants.
            uint32_t next_head = (kbd_buf_head + 1) % KBD_BUF_SIZE;
            if (next_head != kbd_buf_tail) {
                kbd_buffer[kbd_buf_head] = event;
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

void keyboard_get_event(KeyboardEvent* out_event) {
    __asm__ volatile("sti");
    
    while (kbd_buf_tail == kbd_buf_head) {
        extern void scheduler_yield(void);
        scheduler_yield();
    }
    
    __asm__ volatile("cli");
    *out_event = kbd_buffer[kbd_buf_tail];
    kbd_buf_tail = (kbd_buf_tail + 1) % KBD_BUF_SIZE;
}

char keyboard_getc(void) {
    KeyboardEvent evt;
    while(1) {
        keyboard_get_event(&evt);
        if (evt.pressed && evt.ascii != 0) {
            return evt.ascii;
        }
    }
}
