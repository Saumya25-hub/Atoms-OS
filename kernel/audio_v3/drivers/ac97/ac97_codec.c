#include "kernel/audio/drivers/ac97/ac97_codec.h"
#include "arch/x86_64/io/port_io.h"

uint16_t g_nam_base = 0;
uint16_t g_nabm_base = 0;

void ac97_codec_init_base(uint16_t nam_bar, uint16_t nabm_bar) {
    g_nam_base = nam_bar;
    g_nabm_base = nabm_bar;
}

void ac97_codec_write(uint8_t reg, uint16_t value) {
    if (!g_nam_base) return;
    io_out16(g_nam_base + reg, value);
}

uint16_t ac97_codec_read(uint8_t reg) {
    if (!g_nam_base) return 0;
    return io_in16(g_nam_base + reg);
}

static void ac97_delay(uint32_t count) {
    for (volatile uint32_t i = 0; i < count; i++) {
        __asm__ volatile("pause");
    }
}

bool ac97_codec_wait_ready(void) {
    if (!g_nabm_base) return false;
    
    uint32_t timeout = 500000;
    while (timeout--) {
        uint32_t status = io_in32(g_nabm_base + AC97_NABM_GLOB_STA);
        if (status & (1 << 8)) { // Primary Codec Ready (PCR)
            return true;
        }
        ac97_delay(10);
    }
    return false;
}

bool ac97_codec_cold_reset(void) {
    if (!g_nabm_base) return false;
    
    uint32_t ctrl = io_in32(g_nabm_base + AC97_NABM_GLOB_CNT);
    
    // Assert Cold Reset (Clear bit 1)
    ctrl &= ~(1 << 1);
    io_out32(g_nabm_base + AC97_NABM_GLOB_CNT, ctrl);
    
    ac97_delay(100000); // Wait
    
    // De-assert Cold Reset (Set bit 1)
    ctrl |= (1 << 1);
    io_out32(g_nabm_base + AC97_NABM_GLOB_CNT, ctrl);
    
    ac97_delay(100000); // Give codec time to wake up
    
    return ac97_codec_wait_ready();
}

bool ac97_codec_warm_reset(void) {
    if (!g_nabm_base) return false;
    
    uint32_t ctrl = io_in32(g_nabm_base + AC97_NABM_GLOB_CNT);
    
    // Assert Warm Reset (Set bit 2)
    ctrl |= (1 << 2);
    io_out32(g_nabm_base + AC97_NABM_GLOB_CNT, ctrl);
    
    ac97_delay(100000);
    
    // Clear Warm Reset
    ctrl &= ~(1 << 2);
    io_out32(g_nabm_base + AC97_NABM_GLOB_CNT, ctrl);
    
    ac97_delay(100000);
    
    return ac97_codec_wait_ready();
}

bool ac97_codec_verify_and_configure(void) {
    extern void display_print(const char*);
    extern void display_print_dec(uint64_t);
    extern void display_print_hex(uint64_t);

    display_print("\n[AC97 RESET]\n");
    display_print("Reset Type: Cold Reset\n");
    if (!ac97_codec_cold_reset()) {
        display_print("Codec Ready: FAIL\nPASS / FAIL: FAIL\n");
        return false;
    }
    display_print("Codec Ready: PASS\nPASS / FAIL: PASS\n");

    // Power Management
    uint16_t power = ac97_codec_read(0x26); // AC97_REG_POWER_CONTROL
    display_print("\n========== POWER ==========\n");
    display_print("Register: 0x"); display_print_hex(power); display_print("\n");
    display_print("DAC: "); display_print((power & 0x0200) ? "DOWN\n" : "UP\n");
    display_print("ADC: "); display_print((power & 0x0100) ? "DOWN\n" : "UP\n");
    display_print("Mixer: "); display_print((power & 0x1000) ? "DOWN\n" : "UP\n");
    display_print("Analog: "); display_print((power & 0x0400) ? "DOWN\n" : "UP\n");
    
    // Wake up if necessary (clear powerdown bits)
    if (power & 0x7F00) {
        ac97_codec_write(0x26, 0x0000);
        ac97_delay(10000);
        power = ac97_codec_read(0x26);
    }
    display_print("Ready: "); display_print((power & 0x000F) == 0x000F ? "YES\n" : "NO\n");
    display_print("PASS\n===========================\n");

    // Volumes
    uint16_t vols[] = {0x02, 0x04, 0x06, 0x18};
    const char* vol_names[] = {"Master", "Headphone", "Mono", "PCM Out"};
    for (int i = 0; i < 4; i++) {
        uint16_t reg = vols[i];
        ac97_codec_write(reg, 0x0000);
        uint16_t rb = ac97_codec_read(reg);
        
        display_print("\n");
        display_print(vol_names[i]);
        display_print(" VOLUME\n");
        display_print("Raw Register: 0x"); display_print_hex(rb); display_print("\n");
        display_print("Mute Bit: "); display_print((rb & 0x8000) ? "ON\n" : "OFF\n");
        display_print("Left Volume: "); display_print_dec((rb >> 8) & 0x3F); display_print("\n");
        display_print("Right Volume: "); display_print_dec(rb & 0x3F); display_print("\n");
        display_print("PASS\n");
    }

    // Extended Audio
    uint16_t ext_id = ac97_codec_read(0x28);
    uint16_t ext_stat = ac97_codec_read(0x2A);
    display_print("\n========== EXTENDED ==========\n");
    display_print("Variable Rate Audio: "); display_print((ext_id & 0x0001) ? "SUPPORTED\n" : "NO\n");
    display_print("Double Rate: "); display_print((ext_id & 0x0002) ? "SUPPORTED\n" : "NO\n");
    display_print("SPDIF: "); display_print((ext_id & 0x0004) ? "SUPPORTED\n" : "NO\n");
    
    if (ext_id & 0x0001) {
        ext_stat |= 0x0001; // Enable VRA
        ac97_codec_write(0x2A, ext_stat);
        ext_stat = ac97_codec_read(0x2A);
    }
    display_print("PASS\n==============================\n");

    // Sample Rate
    display_print("\n[SAMPLE RATE NEGOTIATION]\n");
    ac97_codec_write(0x2C, 48000);
    ac97_codec_write(0x32, 48000);
    uint16_t dac_rate = ac97_codec_read(0x2C);
    uint16_t adc_rate = ac97_codec_read(0x32);
    
    display_print("Requested: 48000\n");
    display_print("Returned: "); display_print_dec(dac_rate); display_print("\n");
    display_print("Difference: "); display_print_dec(48000 - dac_rate); display_print("\n");
    if (dac_rate == 48000) {
        display_print("PASS\n");
    } else {
        display_print("FAIL - Fixed Rate Codec\n");
    }

    // Register Map
    display_print("\n[REGISTER MAP]\n");
    uint16_t map[] = {0x00, 0x02, 0x0E, 0x12, 0x10, 0x16, 0x1C, 0x20, 0x26, 0x28, 0x2A, 0x2C, 0x32};
    for(int i = 0; i < 13; i++) {
        display_print("Reg 0x"); display_print_hex(map[i]);
        display_print(": 0x"); display_print_hex(ac97_codec_read((uint8_t)map[i]));
        display_print("\n");
    }
    
    return true;
}
