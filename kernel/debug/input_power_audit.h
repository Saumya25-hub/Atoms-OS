#ifndef ATOMS_INPUT_POWER_AUDIT_H
#define ATOMS_INPUT_POWER_AUDIT_H

#include <stdint.h>
#include <stdbool.h>
#include "kernel/core/core_legacy/boot/include/boot_info.h"

#define AUDIT_MAX_PORTS 16
#define AUDIT_STATUS_SAMPLES 100

typedef struct {
    uint8_t port_num;
    uint32_t raw_portsc;
    bool ccs;       /* Bit 0: Current Connect Status */
    bool ped;       /* Bit 1: Port Enabled/Disabled */
    bool oca;       /* Bit 3: Over-Current Active */
    bool pr;        /* Bit 4: Port Reset */
    uint8_t pls;    /* Bits 5-8: Port Link State */
    bool pp;        /* Bit 9: PORT POWER (1 = Powered, 0 = Unpowered) */
    uint8_t speed;  /* Bits 10-13: 1=Full, 2=Low, 3=High, 4=SuperSpeed */
} AuditPortInfo;

typedef struct {
    /* 1. PCI USB Controller Info */
    bool pci_controller_found;
    uint8_t pci_bus;
    uint8_t pci_slot;
    uint8_t pci_func;
    uint16_t vendor_id;
    uint16_t device_id;
    uint16_t pci_cmd;
    uint16_t pci_status;
    uint64_t mmio_base;
    uint8_t irq_line;
    uint8_t irq_pin;

    /* 2. xHCI Capability Info */
    uint8_t caplength;
    uint16_t hciversion;
    uint32_t max_slots;
    uint32_t max_ports;
    uint32_t eecp_offset;

    /* 3. xHCI Ownership / BIOS SMM Handoff State */
    bool has_legacy_ext;
    uint32_t raw_usblegsup;
    bool bios_owned;    /* Bit 16 */
    bool os_owned;      /* Bit 24 */
    uint32_t raw_usblegctlsts;

    /* 4. xHCI Operational Register State */
    uint32_t usbcmd;
    uint32_t usbsts;
    uint32_t config_reg;
    bool is_running;    /* USBCMD Bit 0 */
    bool is_halted;     /* USBSTS Bit 0 */
    bool host_sys_err;  /* USBSTS Bit 2 */
    bool not_ready;     /* USBSTS Bit 11 (CNR) */

    /* 5. Root Hub Ports */
    uint32_t port_count;
    AuditPortInfo ports[AUDIT_MAX_PORTS];

    /* 6. 8042 PS/2 Passive Status Log (100 Samples) */
    struct {
        uint64_t tsc;
        uint8_t status;
        bool obf;
        bool ibf;
        bool sys;
        bool cmd_data;
        bool keylock;
        bool aux;
        bool timeout;
        bool parity;
        bool has_data;
        uint8_t data_byte;
        uint8_t status_after_read;
    } status_samples[AUDIT_STATUS_SAMPLES];

    /* Summary Indicators */
    uint32_t total_connected_usb_devices;
    uint32_t total_powered_usb_ports;
    bool initial_8042_timeout;
    bool initial_8042_obf;
    bool initial_8042_aux;
} InputPowerAudit;

extern InputPowerAudit g_input_power_audit;

void input_power_audit_run(boot_info_t *boot_info);

#endif /* ATOMS_INPUT_POWER_AUDIT_H */
