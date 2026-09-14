#ifndef ATOMS_DEBUG_BOOT_H
#define ATOMS_DEBUG_BOOT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifndef ATOMS_DEBUG_BOOT
#define ATOMS_DEBUG_BOOT 1
#endif

#ifndef ATOMS_DEBUG_BOOT_COUNT
#define ATOMS_DEBUG_BOOT_COUNT 5
#endif

#ifndef ATOMS_MEDIA_DEBUG_FRAMES
#define ATOMS_MEDIA_DEBUG_FRAMES 5
#endif

/* CMOS Register Allocation for Test Loop Persistence */
#define CMOS_DEBUG_MAGIC_REG  0x37
#define CMOS_DEBUG_COUNT_REG  0x38
#define CMOS_DEBUG_MAGIC_VAL  0xA5

#ifdef __cplusplus
extern "C" {
#endif

/* Serial telemetry helpers */
void com1_puts(const char *s);

/* First Failure Rule API */
void        atoms_first_failure_record(const char* failure_id);
bool        atoms_first_failure_occurred(void);
const char* atoms_first_failure_get(void);
void        atoms_first_failure_reset(void);

/* Real Hardware Telemetry */
void atoms_hw_telemetry_dump(void);

/* CMOS NVRAM Accessors */
uint8_t atoms_cmos_read(uint8_t reg);
void    atoms_cmos_write(uint8_t reg, uint8_t val);

/* Automated PXE Reboot Test Supervisor */
void atoms_debug_test_init(void);
void atoms_debug_test_reboot(void);
void bos_media_player_stop_quiesce(void);

static inline void atoms_u32_to_hex(uint32_t val, char* out) {
    static const char hex[] = "0123456789ABCDEF";
    for (int i = 7; i >= 0; i--) {
        out[i] = hex[val & 0xF];
        val >>= 4;
    }
    out[8] = '\0';
}

/* Frame Forensics CRC32 Calculation */
static inline uint32_t atoms_crc32(const void* data, size_t len) {
    if (!data || len == 0) return 0;
    const uint8_t* p = (const uint8_t*)data;
    uint32_t crc = 0xFFFFFFFFU;
    for (size_t i = 0; i < len; i++) {
        crc ^= p[i];
        for (int b = 0; b < 8; b++) {
            crc = (crc >> 1) ^ ((crc & 1U) ? 0xEDB88320U : 0);
        }
    }
    return crc ^ 0xFFFFFFFFU;
}

#ifdef __cplusplus
}
#endif

#endif /* ATOMS_DEBUG_BOOT_H */
