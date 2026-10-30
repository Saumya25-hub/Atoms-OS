/*
 * ATOMS OS — Virtual Platform & Emulated Chipset Subsystem
 * Copyright © 2026 ATOMS OS Project / Saumya Chaudhari
 *
 * Phase 4: Virtual Platform (UART, LAPIC, IOAPIC, PIT, RTC, ACPI, PCI CF8/CFC)
 */

#ifndef ATOMS_VIRTUAL_PLATFORM_H
#define ATOMS_VIRTUAL_PLATFORM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct atoms_vm VirtualMachine;
typedef struct vcpu vCPU;

/* Standard Platform Constants */
#define VIRTUAL_UART_COM1_BASE              0x03F8
#define VIRTUAL_UART_COM1_END               0x03FF
#define VIRTUAL_UART_LOG_BUFFER_SIZE        4096

#define VIRTUAL_LAPIC_BASE_GPA              0xFEE00000ULL
#define VIRTUAL_LAPIC_SIZE                  0x1000ULL

#define VIRTUAL_IOAPIC_BASE_GPA             0xFEC00000ULL
#define VIRTUAL_IOAPIC_SIZE                 0x1000ULL

#define VIRTUAL_ACPI_RSDP_GPA               0x000E0000ULL
#define VIRTUAL_ACPI_RSDT_GPA               0x000E0100ULL
#define VIRTUAL_ACPI_XSDT_GPA               0x000E0200ULL
#define VIRTUAL_ACPI_MADT_GPA               0x000E0300ULL
#define VIRTUAL_ACPI_FADT_GPA               0x000E0400ULL
#define VIRTUAL_ACPI_DSDT_GPA               0x000E0500ULL

/* ACPI Table Structures */
typedef struct {
    char signature[8];
    uint8_t checksum;
    char oem_id[6];
    uint8_t revision;
    uint32_t rsdt_address;
    uint32_t length;
    uint64_t xsdt_address;
    uint8_t extended_checksum;
    uint8_t reserved[3];
} __attribute__((packed)) acpi_rsdp_t;

typedef struct {
    char signature[4];
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char oem_id[6];
    char oem_table_id[8];
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
} __attribute__((packed)) acpi_header_t;

typedef struct {
    acpi_header_t header;
    uint32_t entries[2]; /* Pointers to MADT and FADT */
} __attribute__((packed)) acpi_rsdt_t;

typedef struct {
    acpi_header_t header;
    uint64_t entries[2]; /* 64-bit pointers to MADT and FADT */
} __attribute__((packed)) acpi_xsdt_t;

typedef struct {
    acpi_header_t header;
    uint32_t lapic_address;
    uint32_t flags;
    /* Record 0: Local APIC */
    uint8_t lapic_type;
    uint8_t lapic_length;
    uint8_t lapic_acpi_id;
    uint8_t lapic_id;
    uint32_t lapic_flags;
    /* Record 1: I/O APIC */
    uint8_t ioapic_type;
    uint8_t ioapic_length;
    uint8_t ioapic_id;
    uint8_t ioapic_reserved;
    uint32_t ioapic_address;
    uint32_t ioapic_gsi_base;
    /* Record 2: Interrupt Source Override (IRQ 0 -> GSI 2) */
    uint8_t iso_type;
    uint8_t iso_length;
    uint8_t iso_bus;
    uint8_t iso_source;
    uint32_t iso_gsi;
    uint16_t iso_flags;
} __attribute__((packed)) acpi_madt_t;

typedef struct {
    acpi_header_t header;
    uint32_t firmware_ctrl;
    uint32_t dsdt;
    uint8_t reserved1;
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
    uint32_t pm_tmr_blk;
    uint32_t gpe0_blk;
    uint32_t gpe1_blk;
    uint8_t pm1_evt_len;
    uint8_t pm1_cnt_len;
    uint8_t pm2_cnt_len;
    uint8_t pm_tmr_len;
    uint8_t gpe0_len;
    uint8_t gpe1_len;
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
} __attribute__((packed)) acpi_fadt_t;

#define VIRTUAL_UART_RX_FIFO_SIZE   256

/* Virtual 8250 UART Structure */
typedef struct {
    uint8_t rbr;    /* Receiver Buffer Register */
    uint8_t thr;    /* Transmitter Holding Register */
    uint8_t ier;    /* Interrupt Enable Register */
    uint8_t iir;    /* Interrupt Identification Register */
    uint8_t fcr;    /* FIFO Control Register */
    uint8_t lcr;    /* Line Control Register */
    uint8_t mcr;    /* Modem Control Register */
    uint8_t lsr;    /* Line Status Register */
    uint8_t msr;    /* Modem Status Register */
    uint8_t scr;    /* Scratch Register */
    uint16_t divisor;

    /* RX FIFO for bridge console input */
    uint8_t rx_fifo[VIRTUAL_UART_RX_FIFO_SIZE];
    uint16_t rx_head;
    uint16_t rx_tail;
    uint16_t rx_count;
    bool rx_irq_pending;
    uint64_t irq_skipped_masked;
    uint8_t last_injected_vector;

    char log_buffer[VIRTUAL_UART_LOG_BUFFER_SIZE];
    uint32_t log_head;
    uint32_t total_chars;
} VirtualUART;

/* Virtual Local APIC Structure */
typedef struct {
    uint32_t id;
    uint32_t version;
    uint32_t tpr;
    uint32_t apr;
    uint32_t ppr;
    uint32_t eoi;
    uint32_t ldr;
    uint32_t dfr;
    uint32_t svr;
    uint32_t isr[8];
    uint32_t tmr[8];
    uint32_t irr[8];
    uint32_t esr;
    uint32_t icr_low;
    uint32_t icr_high;
    uint32_t lvt_timer;
    uint32_t lvt_lint0;
    uint32_t lvt_lint1;
    uint32_t lvt_error;
    uint32_t timer_initial;
    uint32_t timer_current;
    uint32_t timer_divide;
    uint64_t tsc_deadline;
    bool enabled;
} VirtualLAPIC;

/* Virtual I/O APIC Structure */
typedef struct {
    uint32_t id;
    uint32_t version;
    uint32_t ioregsel;
    uint64_t redirection_table[24];
} VirtualIOAPIC;

/* Virtual Platform Container */
#define I8042_FIFO_SIZE 64

typedef struct {
    uint8_t fifo[I8042_FIFO_SIZE];
    uint8_t head;
    uint8_t tail;
    uint8_t count;

    uint8_t status;
    uint8_t command_byte;
    uint8_t last_cmd_64;
    uint8_t last_data_60;
    bool expecting_cmd_byte;
    bool expecting_led_mask;
    bool expecting_scancode_set;
    bool expecting_typematic;
    bool expecting_d1_outport;
    bool expecting_d4_mouse;
    uint8_t scancode_set;

    uint64_t ev_host;
    uint64_t ev_queued;
    uint64_t ev_read;
    uint64_t irq_raised;
    uint64_t irq_injected;
    uint64_t irq_skipped_masked;
    uint8_t last_injected_vector;
} VirtualI8042;

/* Virtual 8259 PIC Structure */
typedef struct {
    uint8_t master_imr;     /* Port 0x21: Master PIC IMR (default 0xFF) */
    uint8_t slave_imr;      /* Port 0xA1: Slave PIC IMR (default 0xFF) */
    uint8_t master_base;    /* ICW2 Master Vector Base (default 0x20) */
    uint8_t slave_base;     /* ICW2 Slave Vector Base (default 0x28) */
    uint8_t master_icw_step;
    uint8_t slave_icw_step;
    bool pic_mode;          /* true if 8259 active and unmasked */
} VirtualPIC;

typedef struct VirtualPlatform {
    VirtualUART uart;
    VirtualLAPIC lapic;
    VirtualIOAPIC ioapic;
    VirtualPIC pic;

    /* PCI CF8/CFC Address/Data Port State */
    uint32_t pci_cf8_addr;

    /* PIT & RTC State */
    uint8_t pit_control;
    uint16_t pit_counter[3];
    uint8_t rtc_index;
    uint8_t rtc_registers[128];

    /* PS/2 Keyboard Controller (i8042) State */
    VirtualI8042 i8042;
    uint8_t kbd_status;
    uint8_t kbd_data;
    uint8_t kbd_cmd;
    bool kbd_irq_pending;

    /* ACPI Power Management Register State (FADT PM1a Block) */
    uint16_t acpi_pm1_status;      /* PM1a Event Status Register (port 0x600-0x601) */
    uint16_t acpi_pm1_enable;      /* PM1a Event Enable Register (port 0x602-0x603) */
    uint16_t acpi_pm1_control;     /* PM1a Control Register (port 0x604-0x605) */

    VirtualMachine *vm;
} VirtualPlatform;

/* Core Platform API */
VirtualPlatform *virtual_platform_create(VirtualMachine *vm);
void virtual_platform_destroy(VirtualPlatform *platform);

/* I/O Port Dispatch (0x0000 - 0xFFFF) */
bool virtual_platform_handle_io(VirtualPlatform *platform, uint16_t port, bool is_write, uint8_t size, uint32_t *val);

/* MMIO Dispatch (Local APIC, I/O APIC, VirtIO BAR1) */
bool virtual_platform_handle_mmio(VirtualPlatform *platform, uint64_t gpa, bool is_write, uint8_t size, uint64_t *val);
void virtual_lapic_set_isr(VirtualPlatform *platform, uint8_t vector);
void virtual_lapic_eoi(VirtualPlatform *platform);

/* Virtual UART RX Input (Bridge) */
bool virtual_uart_rx_push(VirtualPlatform *platform, uint8_t byte);

/* CPUID Virtualization */
void virtual_platform_handle_cpuid(vCPU *vcpu, uint32_t leaf, uint32_t subleaf, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx);

/* MSR Virtualization */
bool virtual_platform_handle_rdmsr(vCPU *vcpu, uint32_t msr, uint64_t *val);
bool virtual_platform_handle_wrmsr(vCPU *vcpu, uint32_t msr, uint64_t val);

/* Synthetic ACPI Table Generation */
bool virtual_platform_build_acpi_tables(VirtualPlatform *platform);

/* Unified Host/Guest Input Focus Router */
typedef enum {
    INPUT_FOCUS_HOST = 0,
    INPUT_FOCUS_GUEST = 1
} input_focus_state_t;

extern volatile input_focus_state_t g_input_focus_state;
extern volatile uint64_t g_focus_transitions_count;
extern volatile uint64_t g_keys_routed_to_guest;
extern volatile uint64_t g_uart_host_keys;
extern volatile uint64_t g_uart_rx_pushed;
extern volatile uint64_t g_uart_rx_read_by_guest;
extern volatile uint64_t g_uart_irq4_injected;

input_focus_state_t atoms_input_get_focus(void);
void atoms_input_set_focus(input_focus_state_t state, const char *reason);
void atoms_input_toggle_focus(const char *reason);
const char* atoms_input_get_focus_header_string(void);
void atoms_input_release_all_guest_keys(void);
void atoms_input_forward_hid_to_uart(uint8_t usage_id, uint8_t modifiers);
bool virtual_i8042_push_byte(VirtualPlatform *platform, uint8_t byte);
uint8_t virtual_i8042_pop_byte(VirtualPlatform *platform);
void atoms_input_forward_hid_to_i8042(bool is_press, uint8_t usage_id);
void atoms_input_forward_modifier_to_i8042(bool is_press, uint8_t mod_mask);


#ifdef __cplusplus
}
#endif

#endif /* ATOMS_VIRTUAL_PLATFORM_H */
