#include "system_power.h"
#include "arch/x86_64/io/port_io.h"
#include "kernel/net/udp/udp.h"
#include "kernel/core/lib/include/string.h"

extern void display_print(const char* str);
extern void com1_puts(const char* str);

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline void outw(uint16_t port, uint16_t val) {
    __asm__ volatile ("outw %0, %1" : : "a"(val), "Nd"(port));
}

static inline void outl(uint16_t port, uint32_t val) {
    __asm__ volatile ("outl %0, %1" : : "a"(val), "Nd"(port));
}

#include "kernel/core/pci/pci.h"

volatile bool g_system_power_transitioning = false;

void atoms_power_shutdown(void) {
    if (g_system_power_transitioning) return;
    g_system_power_transitioning = true;
    
    com1_puts("[SYSTEM POWER] User requested graceful system shutdown.\r\n");
    
    extern void rook_shutdown_spin(bool is_restart);
    rook_shutdown_spin(false);
}

void atoms_power_reboot(void) {
    if (g_system_power_transitioning) return;
    g_system_power_transitioning = true;
    
    com1_puts("[SYSTEM POWER] User requested graceful system reboot.\r\n");
    
    extern void rook_shutdown_spin(bool is_restart);
    rook_shutdown_spin(true);
}

typedef enum {
    EFI_RESET_COLD = 0,
    EFI_RESET_WARM = 1,
    EFI_RESET_SHUTDOWN = 2,
    EFI_RESET_PLATFORM_SPECIFIC = 3
} EFI_RESET_TYPE;

typedef void (__attribute__((ms_abi)) *EFI_RESET_SYSTEM_FN)(
    EFI_RESET_TYPE ResetType,
    uint64_t ResetStatus,
    uint64_t DataSize,
    void *ResetData
);

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    __asm__ volatile ("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline uint32_t inl(uint16_t port) {
    uint32_t ret;
    __asm__ volatile ("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* ACPI Description Table Headers */
struct acpi_rsdp_descriptor {
    char signature[8];      // "RSD PTR "
    uint8_t checksum;
    char oem_id[6];
    uint8_t revision;       // 0 for ACPI 1.0, 2 for ACPI 2.0+
    uint32_t rsdt_address;  // Physical 32-bit RSDT
    uint32_t length;
    uint64_t xsdt_address;  // Physical 64-bit XSDT
    uint8_t extended_checksum;
    uint8_t reserved[3];
} __attribute__((packed));

struct acpi_sdt_header {
    char signature[4];
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char oem_id[6];
    char oem_table_id[8];
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
} __attribute__((packed));

struct acpi_fadt_table {
    struct acpi_sdt_header header;
    uint32_t firmware_ctrl;
    uint32_t dsdt;
    uint8_t reserved;
    uint8_t preferred_pm_profile;
    uint16_t sci_int;
    uint32_t smi_cmd;
    uint8_t acpi_enable;
    uint8_t acpi_disable;
    uint8_t s4bios_req;
    uint8_t pstate_cnt;
    uint32_t pm1a_evt_blk;
    uint32_t pm1b_evt_blk;
    uint32_t pm1a_cnt_blk;
    uint32_t pm1b_cnt_blk;
    uint32_t pm2_cnt_blk;
    uint32_t pmtmr_blk;
    uint32_t gpe0_blk;
    uint32_t gpe1_blk;
    uint8_t pm1_evt_len;
    uint8_t pm1_cnt_len;
    uint8_t pm2_cnt_len;
    uint8_t pmtmr_len;
    uint8_t gpe0_blk_len;
    uint8_t gpe1_blk_len;
    uint8_t gpe1_base;
    uint8_t cst_cnt;
    uint16_t p_lvl2_lat;
    uint16_t p_lvl3_lat;
    uint16_t flush_size;
    uint16_t flush_stride;
    uint8_t duty_offset;
    uint8_t duty_width;
    uint8_t day_alrm;
    uint8_t mon_alrm;
    uint8_t century;
    uint16_t iapc_boot_arch;
    uint8_t reserved2;
    uint32_t flags;
    uint8_t reset_reg[12];
    uint8_t reset_value;
    uint8_t reserved3[3];
    uint64_t x_firmware_ctrl;
    uint64_t x_dsdt;
    uint8_t x_pm1a_evt_blk[12];
    uint8_t x_pm1b_evt_blk[12];
    uint8_t x_pm1a_cnt_blk[12];
    uint8_t x_pm1b_cnt_blk[12];
} __attribute__((packed));

static struct acpi_rsdp_descriptor* find_acpi_rsdp(void) {
    // 1. Check UEFI Configuration Table pointer if available from bootloader
    uint64_t cfgtbl_addr = *(uint64_t*)0x1008;
    uint64_t num_entries = *(uint64_t*)0x1010;
    if (cfgtbl_addr != 0 && num_entries > 0 && num_entries < 1000) {
        struct {
            uint32_t data1;
            uint16_t data2;
            uint16_t data3;
            uint8_t  data4[8];
            uint64_t vendor_table;
        } __attribute__((packed)) *entries = (void*)(uintptr_t)cfgtbl_addr;

        for (uint64_t i = 0; i < num_entries; i++) {
            if (entries[i].vendor_table != 0) {
                struct acpi_rsdp_descriptor *r = (void*)(uintptr_t)entries[i].vendor_table;
                if (memcmp(r->signature, "RSD PTR ", 8) == 0) {
                    return r;
                }
            }
        }
    }

    // 2. Scan BIOS Read-Only Memory Area 0xE0000 - 0xFFFFF (Step 16)
    for (uintptr_t addr = 0xE0000; addr < 0x100000; addr += 16) {
        if (memcmp((const void*)addr, "RSD PTR ", 8) == 0) {
            return (struct acpi_rsdp_descriptor*)addr;
        }
    }

    // 3. Scan EBDA 0x9FC00 - 0x9FFFF
    for (uintptr_t addr = 0x9FC00; addr < 0xA0000; addr += 16) {
        if (memcmp((const void*)addr, "RSD PTR ", 8) == 0) {
            return (struct acpi_rsdp_descriptor*)addr;
        }
    }

    return NULL;
}

static struct acpi_fadt_table* find_acpi_fadt(struct acpi_rsdp_descriptor *rsdp) {
    if (!rsdp) return NULL;

    // Try 64-bit XSDT first
    if (rsdp->revision >= 2 && rsdp->xsdt_address != 0) {
        struct acpi_sdt_header *xsdt = (void*)(uintptr_t)rsdp->xsdt_address;
        if (xsdt && memcmp(xsdt->signature, "XSDT", 4) == 0) {
            uint32_t entries_count = (xsdt->length - sizeof(struct acpi_sdt_header)) / 8;
            uint64_t *table_ptrs = (uint64_t*)((uintptr_t)xsdt + sizeof(struct acpi_sdt_header));
            for (uint32_t i = 0; i < entries_count; i++) {
                struct acpi_sdt_header *table = (void*)(uintptr_t)table_ptrs[i];
                if (table && memcmp(table->signature, "FACP", 4) == 0) {
                    return (struct acpi_fadt_table*)table;
                }
            }
        }
    }

    // Fallback to 32-bit RSDT
    if (rsdp->rsdt_address != 0) {
        struct acpi_sdt_header *rsdt = (void*)(uintptr_t)rsdp->rsdt_address;
        if (rsdt && memcmp(rsdt->signature, "RSDT", 4) == 0) {
            uint32_t entries_count = (rsdt->length - sizeof(struct acpi_sdt_header)) / 4;
            uint32_t *table_ptrs = (uint32_t*)((uintptr_t)rsdt + sizeof(struct acpi_sdt_header));
            for (uint32_t i = 0; i < entries_count; i++) {
                struct acpi_sdt_header *table = (void*)(uintptr_t)table_ptrs[i];
                if (table && memcmp(table->signature, "FACP", 4) == 0) {
                    return (struct acpi_fadt_table*)table;
                }
            }
        }
    }

    return NULL;
}

static bool parse_s5_from_dsdt(struct acpi_fadt_table *fadt, uint16_t *out_slp_typa, uint16_t *out_slp_typb) {
    uintptr_t dsdt_addr = 0;
    if (fadt->header.length >= sizeof(struct acpi_fadt_table) && fadt->x_dsdt != 0) {
        dsdt_addr = (uintptr_t)fadt->x_dsdt;
    } else if (fadt->dsdt != 0) {
        dsdt_addr = (uintptr_t)fadt->dsdt;
    }

    if (!dsdt_addr) return false;

    struct acpi_sdt_header *dsdt = (void*)dsdt_addr;
    if (memcmp(dsdt->signature, "DSDT", 4) != 0) return false;

    const uint8_t *aml = (const uint8_t*)dsdt_addr;
    uint32_t aml_len = dsdt->length;

    // Search for AML object "_S5_"
    for (uint32_t i = 0; i < aml_len - 8; i++) {
        if (aml[i] == '_' && aml[i+1] == 'S' && aml[i+2] == '5' && aml[i+3] == '_') {
            uint32_t p = i + 4;
            // Check Package Op (0x12)
            if (aml[p] == 0x12 || aml[p+1] == 0x12) {
                while (p < aml_len && aml[p] != 0x12) p++;
                p++; // Skip 0x12
                // Skip PkgLength (1 to 4 bytes)
                uint8_t pkg_lead = aml[p];
                uint8_t byte_count = (pkg_lead >> 6) & 3;
                p += (byte_count == 0) ? 1 : (byte_count + 1);
                p++; // Skip NumElements

                // First element: SLP_TYPa
                uint16_t val_a = 0;
                if (aml[p] == 0x0A) { val_a = aml[p+1]; p += 2; }
                else if (aml[p] == 0x00) { val_a = 0; p++; }
                else if (aml[p] == 0x01) { val_a = 1; p++; }
                else { val_a = aml[p]; p++; }

                // Second element: SLP_TYPb
                uint16_t val_b = 0;
                if (aml[p] == 0x0A) { val_b = aml[p+1]; }
                else if (aml[p] == 0x00) { val_b = 0; }
                else if (aml[p] == 0x01) { val_b = 1; }
                else { val_b = aml[p]; }

                *out_slp_typa = val_a;
                *out_slp_typb = val_b;
                return true;
            }
        }
    }

    return false;
}

static inline void vmware_poweroff(void) {
    __asm__ volatile (
        "inl (%%dx), %%eax"
        :
        : "a"(0x564D5868), "b"(0), "c"(10), "d"(0x5658)
        : "memory"
    );
}

void system_shutdown(void) {
    com1_puts("\r\n[SYSTEM POWER] INITIATING BARE-METAL HARDWARE SHUTDOWN...\r\n");
    display_print("\n[SYSTEM POWER] SHUTTING DOWN ATOMS OS...\n");

    // 0. Immediate Display Blackout (Clean Screen Off)
    extern void rook_blackout_screen(void);
    rook_blackout_screen();

    // 1. VMware Workstation / ESXi Native Backdoor Power-Off
    vmware_poweroff();

    // 2. QEMU / Bochs / VirtualBox ACPI Soft-Off
    outw(0x604, 0x2000); // QEMU
    outw(0x404, 0x3400); // VirtualBox / Bochs

    // 3. Linux Universal ACPI FADT/DSDT S5 Parser (Clean ACPI S5 Soft-Off)
    struct acpi_rsdp_descriptor *rsdp = find_acpi_rsdp();
    if (rsdp) {
        com1_puts("[SYSTEM POWER] ACPI RSDP Located. Searching for FADT & DSDT...\r\n");
        struct acpi_fadt_table *fadt = find_acpi_fadt(rsdp);
        if (fadt) {
            uint16_t slp_typa = 7;
            uint16_t slp_typb = 7;
            if (parse_s5_from_dsdt(fadt, &slp_typa, &slp_typb)) {
                com1_puts("[SYSTEM POWER] ACPI \\_S5_ Object Successfully Parsed from DSDT!\r\n");
            }

            uint16_t pm1a_cnt = (uint16_t)fadt->pm1a_cnt_blk;
            uint16_t pm1b_cnt = (uint16_t)fadt->pm1b_cnt_blk;

            // Clear WAK_STS in PM1_STS
            if (pm1a_cnt >= 4) outw(pm1a_cnt - 4, 0xFFFF);
            if (pm1b_cnt >= 4) outw(pm1b_cnt - 4, 0xFFFF);

            // Execute clean S5 Sleep Sequence (SLP_TYP + SLP_EN bit 13)
            if (pm1a_cnt) {
                outw(pm1a_cnt, (uint16_t)((slp_typa << 10) | (1 << 13)));
            }
            if (pm1b_cnt) {
                outw(pm1b_cnt, (uint16_t)((slp_typb << 10) | (1 << 13)));
            }
        }
    }

    // 4. Intel Haswell / H81 / 8-Series PCH ACPI PMBASE Direct Shutdown (0x1804 / 0x0404 / 0x0804)
    outw(0x1804, 0x3C00); // Standard Intel PCH PM1_CNT S5
    outw(0x1804, 0x2000);
    outw(0x0404, 0x3400);
    outw(0x0804, 0x3400);

    // 5. Cache Writeback and Invalidation before CPU Halt
    __asm__ volatile ("wbinvd");

    // 6. Safe Bare-Metal CPU Halt (Standby 5VSB rail protected for Wake-on-LAN)
    com1_puts("[SYSTEM POWER] CPU HALTED — POWER OFF SAFE.\r\n");
    display_print("[SYSTEM POWER] System Halted Safely. It is now safe to turn off your computer.\n");
    while (1) {
        __asm__ volatile ("cli; hlt");
    }
}

void system_reboot(void) {
    com1_puts("\r\n[SYSTEM POWER] INITIATING HARDWARE REBOOT...\r\n");
    display_print("\n[SYSTEM POWER] REBOOTING ATOMS OS...\n");

    // 0. UEFI Specification Standard: Runtime Services ResetSystem(EfiResetCold)
    uint64_t rts_addr = *(uint64_t*)0x1000;
    if (rts_addr != 0 && rts_addr < 0xFFFFFFFFFF000000ULL) {
        uint64_t reset_sys_addr = *(uint64_t*)(rts_addr + 104);
        if (reset_sys_addr != 0) {
            com1_puts("[SYSTEM POWER] Calling UEFI RuntimeServices->ResetSystem(EfiResetCold)...\r\n");
            EFI_RESET_SYSTEM_FN efi_reset = (EFI_RESET_SYSTEM_FN)reset_sys_addr;
            efi_reset(EFI_RESET_COLD, 0, 0, 0);
        }
    }

    // 1. Intel PCH Fast Reset Controller (Port 0xCF9 -> Write 0x02, then 0x06)
    // Bit 1 = System Reset, Bit 2 = Hard Reset
    outb(0xCF9, 0x02);
    for (volatile int i = 0; i < 10000; i++);
    outb(0xCF9, 0x06);

    // 2. PS/2 Keyboard Controller Reset Pulse (Port 0x64 -> Command 0xFE)
    outb(0x64, 0xFE);

    // 3. Triple Fault Reset (Load 0-length IDT and trigger interrupt 3)
    struct {
        uint16_t limit;
        uint64_t base;
    } __attribute__((packed)) null_idt = {0, 0};

    __asm__ volatile (
        "cli\n"
        "lidt %0\n"
        "int3\n"
        : : "m"(null_idt)
    );

    while (1) {
        __asm__ volatile ("cli; hlt");
    }
}

void atoms_debug_test_reboot(void) {
#if defined(ATOMS_DEBUG_BOOT) && (ATOMS_DEBUG_BOOT == 1)
    extern uint8_t atoms_cmos_read(uint8_t reg);
    extern void atoms_cmos_write(uint8_t reg, uint8_t val);
    uint8_t count = atoms_cmos_read(0x38);
    if (count > 0) {
        count--;
        atoms_cmos_write(0x38, count);
        com1_puts("[DEBUG_TEST] TEST_COMPLETE\r\n");
        com1_puts("[DEBUG_TEST] requesting reboot (remaining boot count: ");
        char buf[8]; buf[0] = '0' + (count % 10); buf[1] = '\0';
        com1_puts(buf);
        com1_puts(")\r\n");
        com1_puts("[DEBUG_TEST] stopping media\r\n");
        extern void bos_media_player_stop_quiesce(void);
        bos_media_player_stop_quiesce();
        com1_puts("[DEBUG_TEST] stopping USB activity\r\n");
        com1_puts("[DEBUG_TEST] flushing required state\r\n");
        __asm__ volatile ("wbinvd");
        com1_puts("[DEBUG_TEST] reboot requested\r\n");
        system_reboot();
    } else {
        com1_puts("[DEBUG_TEST] boot_count reached zero. Halting reboot loop. Remaining on desktop.\r\n");
    }
#else
    system_reboot();
#endif
}

static void remote_power_udp_callback(uint32_t src_ip, uint16_t src_port, const uint8_t* payload, uint16_t payload_len) {
    (void)src_ip; (void)src_port;
    if (!payload || payload_len == 0) return;

    char cmd[64];
    uint16_t copy_len = (payload_len < 63) ? payload_len : 63;
    memcpy(cmd, payload, copy_len);
    cmd[copy_len] = '\0';

    if (strstr(cmd, "SHUTDOWN") || strstr(cmd, "shutdown")) {
        system_shutdown();
    } else if (strstr(cmd, "REBOOT") || strstr(cmd, "reboot")) {
        atoms_debug_test_reboot();
    } else if (strstr(cmd, "SCREENSHOT") || strstr(cmd, "screenshot")) {
        extern bool atoms_screenshot_capture_and_send(uint32_t session_id);
        atoms_screenshot_capture_and_send(99);
    }
}

void remote_power_init(void) {
    udp_register_handler(9999, remote_power_udp_callback);
    com1_puts("[POWER] Remote UDP Power Management Handler Registered on Port 9999 (REBOOT/SHUTDOWN)\r\n");
}

