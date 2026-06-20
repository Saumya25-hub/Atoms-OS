#include "drivers/timer/pit/pit.h"
#include "arch/x86_64/io/port_io.h"

// PIT Ports
#define PIT_CHANNEL0_DATA 0x40
#define PIT_COMMAND_PORT  0x43

// Base clock frequency of the 8254 PIT
#define PIT_BASE_FREQUENCY 1193182

static void pit_set_frequency(uint32_t frequency) {
    if (frequency == 0) return;
    
    // Calculate the divisor
    uint32_t divisor = PIT_BASE_FREQUENCY / frequency;
    
    // Divisor must fit in 16 bits
    if (divisor > 65535) {
        divisor = 65535;
    } else if (divisor == 0) {
        divisor = 1; // Not usually expected for standard frequencies
    }
    
    // Command byte:
    // 0x36 = 00 11 011 0
    // Channel 0 (00), Access LSB then MSB (11), Square Wave Mode (011), 16-bit binary (0)
    io_out8(PIT_COMMAND_PORT, 0x36);
    
    // Send LSB
    io_out8(PIT_CHANNEL0_DATA, (uint8_t)(divisor & 0xFF));
    
    // Send MSB
    io_out8(PIT_CHANNEL0_DATA, (uint8_t)((divisor >> 8) & 0xFF));
}

static void pit_init(uint32_t frequency) {
    pit_set_frequency(frequency);
}

// Global instance of the backend
TimerBackend pit_timer_backend = {
    .init = pit_init,
    .set_frequency = pit_set_frequency
};
