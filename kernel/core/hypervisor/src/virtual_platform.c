/*
 * ATOMS OS — Virtual Platform & Emulated Chipset Subsystem Implementation
 * Copyright © 2026 ATOMS OS Project / Saumya Chaudhari
 *
 * Phase 4: Full FreeBSD amd64 Guest Support (UART, LAPIC, IOAPIC, ACPI, PCI CF8/CFC)
 */

#include "kernel/core/hypervisor/include/virtual_platform.h"
#include "kernel/core/hypervisor/include/hypervisor.h"
#include "kernel/core/hypervisor/include/virtio_pci.h"
#include "kernel/core/hypervisor/include/guest_memory.h"
#include "kernel/core/hypervisor/include/vmx.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/lib/include/string.h"

extern void com1_puts(const char *s);

static inline uint64_t vmx_vmread_field(uint64_t field) {
    uint64_t val = 0;
    __asm__ volatile ("vmread %1, %0" : "=r"(val) : "r"(field) : "cc");
    return val;
}

static inline void vmx_vmwrite_field(uint64_t field, uint64_t val) {
    __asm__ volatile ("vmwrite %1, %0" : : "r"(field), "r"(val) : "cc");
}

static inline void vmm_wrmsr_raw(uint32_t msr, uint64_t val) {
    uint32_t low = (uint32_t)val;
    uint32_t high = (uint32_t)(val >> 32);
    __asm__ volatile ("wrmsr" : : "c"(msr), "a"(low), "d"(high));
}

static uint8_t compute_checksum(const void *data, size_t length) {
    const uint8_t *bytes = (const uint8_t *)data;
    uint8_t sum = 0;
    for (size_t i = 0; i < length; i++) {
        sum += bytes[i];
    }
    return (uint8_t)(0 - sum);
}

/* --------------------------------------------------------------------------
 * Platform Lifecycle
 * -------------------------------------------------------------------------- */
VirtualPlatform *virtual_platform_create(VirtualMachine *vm) {
    if (!vm) return NULL;

    VirtualPlatform *platform = (VirtualPlatform *)kmalloc(sizeof(VirtualPlatform));
    if (!platform) return NULL;
    memset(platform, 0, sizeof(VirtualPlatform));

    platform->vm = vm;

    /* Initialize Virtual 8250 UART */
    platform->uart.lsr = 0x60; /* Line Status: Transmitter Holding Reg Empty & Transmitter Empty */
    platform->uart.msr = 0xB0; /* Modem Status: DCD | DSR | CTS */
    platform->uart.divisor = 1; /* 115200 Baud */

    /* Initialize Virtual Local APIC */
    platform->lapic.id = 0;
    platform->lapic.version = 0x00050014; /* Standard Integrated APIC */
    platform->lapic.svr = 0x000001FF;     /* Spurious vector 0xFF, Enabled */
    platform->lapic.enabled = true;

    /* Initialize Virtual I/O APIC */
    platform->ioapic.id = 1;
    platform->ioapic.version = 0x00170020; /* 24 Redirection entries */

    /* Build Synthetic ACPI Tables in Guest RAM */
    virtual_platform_build_acpi_tables(platform);

    return platform;
}

void virtual_platform_destroy(VirtualPlatform *platform) {
    if (!platform) return;
    kfree(platform);
}

/* --------------------------------------------------------------------------
 * Synthetic ACPI Table Generation
 * -------------------------------------------------------------------------- */
bool virtual_platform_build_acpi_tables(VirtualPlatform *platform) {
    if (!platform || !platform->vm || !platform->vm->guest_mem) return false;

    GuestMemory *mem = platform->vm->guest_mem;
    if (mem->gpa_size < 0x100000) return false; /* Minimum 1MB RAM required */

    uint8_t *guest_base = (uint8_t *)mem->hva_backing;

    /* 1. RSDP at 0xE0000 */
    acpi_rsdp_t *rsdp = (acpi_rsdp_t *)(guest_base + VIRTUAL_ACPI_RSDP_GPA);
    memset(rsdp, 0, sizeof(acpi_rsdp_t));
    memcpy(rsdp->signature, "RSD PTR ", 8);
    memcpy(rsdp->oem_id, "ATOMS ", 6);
    rsdp->revision = 2;
    rsdp->rsdt_address = (uint32_t)VIRTUAL_ACPI_RSDT_GPA;
    rsdp->length = sizeof(acpi_rsdp_t);
    rsdp->xsdt_address = VIRTUAL_ACPI_XSDT_GPA;
    rsdp->checksum = compute_checksum(rsdp, 20);
    rsdp->extended_checksum = compute_checksum(rsdp, sizeof(acpi_rsdp_t));

    /* 2. DSDT at 0xE0500 with authentic PCI Host Bridge (_SB.PCI0) & _PRT table */
    static const uint8_t s_dsdt_aml[] = {
        0x10, 0x41, 0x06, 0x5C, 0x5F, 0x53, 0x42, 0x5F, /* Scope (\_SB_) */
        0x5B, 0x82, 0x48, 0x05, 0x50, 0x43, 0x49, 0x30, /* Device (PCI0) */
        0x08, 0x5F, 0x48, 0x49, 0x44, 0x0C, 0x03, 0x0A, 0xD0, 0x41, /* Name (_HID, 0x41D00A03 / PNP0A03) */
        0x08, 0x5F, 0x41, 0x44, 0x52, 0x00,             /* Name (_ADR, 0) */
        0x08, 0x5F, 0x42, 0x42, 0x4E, 0x00,             /* Name (_BBN, 0) */
        0x08, 0x5F, 0x50, 0x52, 0x54, 0x12, 0x37, 0x04, /* Name (_PRT, Package (4)) */
        0x12, 0x0C, 0x04, 0x0C, 0xFF, 0xFF, 0x01, 0x00, 0x0A, 0x00, 0x00, 0x0A, 0x0B, /* Slot 1 Pin 0 -> GSI 11 */
        0x12, 0x0C, 0x04, 0x0C, 0xFF, 0xFF, 0x02, 0x00, 0x0A, 0x00, 0x00, 0x0A, 0x0B, /* Slot 2 Pin 0 -> GSI 11 */
        0x12, 0x0C, 0x04, 0x0C, 0xFF, 0xFF, 0x03, 0x00, 0x0A, 0x00, 0x00, 0x0A, 0x0B, /* Slot 3 Pin 0 -> GSI 11 */
        0x12, 0x0C, 0x04, 0x0C, 0xFF, 0xFF, 0x04, 0x00, 0x0A, 0x00, 0x00, 0x0A, 0x0B  /* Slot 4 Pin 0 -> GSI 11 */
    };

    acpi_header_t *dsdt = (acpi_header_t *)(guest_base + VIRTUAL_ACPI_DSDT_GPA);
    memset(dsdt, 0, sizeof(acpi_header_t) + sizeof(s_dsdt_aml));
    memcpy(dsdt->signature, "DSDT", 4);
    dsdt->length = (uint32_t)(sizeof(acpi_header_t) + sizeof(s_dsdt_aml));
    dsdt->revision = 2;
    memcpy(dsdt->oem_id, "ATOMS ", 6);
    memcpy(dsdt->oem_table_id, "ATOMSVM ", 8);
    dsdt->oem_revision = 1;
    dsdt->creator_id = 0x4D534F42; /* "BOSM" */
    dsdt->creator_revision = 1;
    memcpy(guest_base + VIRTUAL_ACPI_DSDT_GPA + sizeof(acpi_header_t), s_dsdt_aml, sizeof(s_dsdt_aml));
    dsdt->checksum = compute_checksum(dsdt, dsdt->length);

    /* 3. MADT at 0xE0300 */
    acpi_madt_t *madt = (acpi_madt_t *)(guest_base + VIRTUAL_ACPI_MADT_GPA);
    memset(madt, 0, sizeof(acpi_madt_t));
    memcpy(madt->header.signature, "APIC", 4);
    madt->header.length = sizeof(acpi_madt_t);
    madt->header.revision = 2;
    memcpy(madt->header.oem_id, "ATOMS ", 6);
    memcpy(madt->header.oem_table_id, "ATOMSVM ", 8);
    madt->header.oem_revision = 1;
    madt->header.creator_id = 0x4D534F42;
    madt->header.creator_revision = 1;
    madt->lapic_address = (uint32_t)VIRTUAL_LAPIC_BASE_GPA;
    madt->flags = 1; /* PC-AT dual 8259 PIC present */

    /* Local APIC for vCPU 0 */
    madt->lapic_type = 0;
    madt->lapic_length = 8;
    madt->lapic_acpi_id = 0;
    madt->lapic_id = 0;
    madt->lapic_flags = 1; /* Enabled */

    /* I/O APIC */
    madt->ioapic_type = 1;
    madt->ioapic_length = 12;
    madt->ioapic_id = 1;
    madt->ioapic_address = (uint32_t)VIRTUAL_IOAPIC_BASE_GPA;
    madt->ioapic_gsi_base = 0;

    /* Interrupt Source Override */
    madt->iso_type = 2;
    madt->iso_length = 10;
    madt->iso_bus = 0;    /* ISA */
    madt->iso_source = 0; /* IRQ 0 */
    madt->iso_gsi = 2;    /* GSI 2 */
    madt->iso_flags = 0;  /* Conforms to bus specifications */

    madt->header.checksum = compute_checksum(madt, sizeof(acpi_madt_t));

    /* 4. FADT at 0xE0400 */
    acpi_fadt_t *fadt = (acpi_fadt_t *)(guest_base + VIRTUAL_ACPI_FADT_GPA);
    memset(fadt, 0, sizeof(acpi_fadt_t));
    memcpy(fadt->header.signature, "FACP", 4);
    fadt->header.length = sizeof(acpi_fadt_t);
    fadt->header.revision = 4;
    memcpy(fadt->header.oem_id, "ATOMS ", 6);
    memcpy(fadt->header.oem_table_id, "ATOMSVM ", 8);
    fadt->header.oem_revision = 1;
    fadt->header.creator_id = 0x4D534F42;
    fadt->header.creator_revision = 1;
    fadt->dsdt = (uint32_t)VIRTUAL_ACPI_DSDT_GPA;
    fadt->sci_int = 9;
    fadt->pm_tmr_blk = 0x408;
    fadt->pm_tmr_len = 4;
    fadt->flags = 0x00000020; /* 32-bit PM Timer */
    fadt->header.checksum = compute_checksum(fadt, sizeof(acpi_fadt_t));

    /* 5. RSDT at 0xE0100 */
    acpi_rsdt_t *rsdt = (acpi_rsdt_t *)(guest_base + VIRTUAL_ACPI_RSDT_GPA);
    memset(rsdt, 0, sizeof(acpi_rsdt_t));
    memcpy(rsdt->header.signature, "RSDT", 4);
    rsdt->header.length = sizeof(acpi_rsdt_t);
    rsdt->header.revision = 1;
    memcpy(rsdt->header.oem_id, "ATOMS ", 6);
    memcpy(rsdt->header.oem_table_id, "ATOMSVM ", 8);
    rsdt->header.oem_revision = 1;
    rsdt->header.creator_id = 0x4D534F42;
    rsdt->header.creator_revision = 1;
    rsdt->entries[0] = (uint32_t)VIRTUAL_ACPI_MADT_GPA;
    rsdt->entries[1] = (uint32_t)VIRTUAL_ACPI_FADT_GPA;
    rsdt->header.checksum = compute_checksum(rsdt, sizeof(acpi_rsdt_t));

    /* 6. XSDT at 0xE0200 */
    acpi_xsdt_t *xsdt = (acpi_xsdt_t *)(guest_base + VIRTUAL_ACPI_XSDT_GPA);
    memset(xsdt, 0, sizeof(acpi_xsdt_t));
    memcpy(xsdt->header.signature, "XSDT", 4);
    xsdt->header.length = sizeof(acpi_xsdt_t);
    xsdt->header.revision = 1;
    memcpy(xsdt->header.oem_id, "ATOMS ", 6);
    memcpy(xsdt->header.oem_table_id, "ATOMSVM ", 8);
    xsdt->header.oem_revision = 1;
    xsdt->header.creator_id = 0x4D534F42;
    xsdt->header.creator_revision = 1;
    xsdt->entries[0] = VIRTUAL_ACPI_MADT_GPA;
    xsdt->entries[1] = VIRTUAL_ACPI_FADT_GPA;
    xsdt->header.checksum = compute_checksum(xsdt, sizeof(acpi_xsdt_t));

    return true;
}

/* --------------------------------------------------------------------------
 * I/O Port Dispatch
 * -------------------------------------------------------------------------- */
bool virtual_platform_handle_io(VirtualPlatform *platform, uint16_t port, bool is_write, uint8_t size, uint32_t *val) {
    if (!platform || !val) return false;

    /* 1. COM1 UART (0x3F8 - 0x3FF) */
    if (port >= VIRTUAL_UART_COM1_BASE && port <= VIRTUAL_UART_COM1_END) {
        uint8_t reg = port - VIRTUAL_UART_COM1_BASE;
        VirtualUART *uart = &platform->uart;

        if (is_write) {
            uint8_t byte = (uint8_t)(*val);
            if (reg == 0) {
                if (uart->lcr & 0x80) {
                    /* Divisor Latch Low (DLL) */
                    uart->divisor = (uart->divisor & 0xFF00) | byte;
                } else {
                    /* Transmitter Holding Register (THR) */
                    uart->thr = byte;
                    uart->total_chars++;
                    if (uart->log_head < (VIRTUAL_UART_LOG_BUFFER_SIZE - 1)) {
                        uart->log_buffer[uart->log_head++] = (char)byte;
                        uart->log_buffer[uart->log_head] = '\0';
                    }
                    /* Realtime mirror single chars to COM1 */
                    char tmp[2] = { (char)byte, '\0' };
                    com1_puts(tmp);
                }
            } else if (reg == 1) {
                if (uart->lcr & 0x80) {
                    /* Divisor Latch High (DLM) */
                    uart->divisor = (uart->divisor & 0x00FF) | ((uint16_t)byte << 8);
                } else {
                    /* Interrupt Enable Register (IER) */
                    uart->ier = byte;
                }
            } else if (reg == 2) {
                uart->fcr = byte;
            } else if (reg == 3) {
                uart->lcr = byte;
            } else if (reg == 4) {
                uart->mcr = byte;
            } else if (reg == 7) {
                uart->scr = byte;
            }
        } else {
            if (reg == 0) {
                if (uart->lcr & 0x80) {
                    *val = (uint8_t)(uart->divisor & 0xFF);
                } else {
                    *val = uart->rbr;
                }
            } else if (reg == 1) {
                if (uart->lcr & 0x80) {
                    *val = (uint8_t)((uart->divisor >> 8) & 0xFF);
                } else {
                    *val = uart->ier;
                }
            } else if (reg == 2) {
                *val = 0x01; /* No interrupt pending */
            } else if (reg == 3) {
                *val = uart->lcr;
            } else if (reg == 4) {
                *val = uart->mcr;
            } else if (reg == 5) {
                *val = uart->lsr; /* Line Status: Always Ready (0x60) */
            } else if (reg == 6) {
                *val = uart->msr; /* Modem Status: Carrier / CTS OK (0xB0) */
            } else if (reg == 7) {
                *val = uart->scr;
            }
        }
        return true;
    }

    /* 2. PCI Configuration Space Mechanism #1 (0xCF8 / 0xCFC) */
    if (port == 0xCF8) {
        if (is_write) {
            platform->pci_cf8_addr = *val;
        } else {
            *val = platform->pci_cf8_addr;
        }
        return true;
    }

    if (port >= 0xCFC && port <= 0xCFF) {
        if (!platform->vm || !platform->vm->pci_bus) {
            if (!is_write) *val = 0xFFFFFFFF;
            return true;
        }

        uint32_t addr = platform->pci_cf8_addr;
        if (addr & 0x80000000ULL) { /* Enable bit set */
            uint8_t bus = (addr >> 16) & 0xFF;
            uint8_t slot = (addr >> 11) & 0x1F;
            uint8_t func = (addr >> 8) & 0x07;
            uint8_t offset = (addr & 0xFC) + (port - 0xCFC);

            if (is_write) {
                virtual_pci_config_write(platform->vm->pci_bus, bus, slot, func, offset, *val, size);
            } else {
                *val = virtual_pci_config_read(platform->vm->pci_bus, bus, slot, func, offset, size);
            }
            return true;
        } else {
            if (!is_write) *val = 0xFFFFFFFF;
            return true;
        }
    }

    /* 3. VirtIO PCI I/O BAR Dispatch */
    if (platform->vm && platform->vm->pci_bus) {
        VirtualPCIBus *pci_bus = platform->vm->pci_bus;
        for (uint32_t i = 0; i < pci_bus->device_count; i++) {
            VirtIOPCIDevice *pdev = pci_bus->devices[i];
            if (pdev && port >= pdev->io_bar_base && port < (pdev->io_bar_base + pdev->io_bar_size)) {
                uint32_t bar_offset = port - pdev->io_bar_base;
                if (is_write) {
                    virtio_pci_bar_write(pdev, bar_offset, *val, size);
                } else {
                    *val = virtio_pci_bar_read(pdev, bar_offset, size);
                }
                return true;
            }
        }
    }

    /* 4. 8254 PIT (0x40 - 0x43) — Hardware Timer Oscillator Emulation */
    if (port >= 0x40 && port <= 0x43) {
        if (is_write) {
            if (port == 0x43) {
                platform->pit_control = (uint8_t)(*val);
                /* Latch command: Channel 0, Latch Counter (0x00) */
                if ((*val & 0xC0) == 0x00 && (*val & 0x30) == 0x00) {
                    uint64_t tsc = 0;
                    __asm__ volatile ("rdtsc" : "=A"(tsc));
                    platform->pit_counter[0] = (uint16_t)(0xFFFF - ((tsc >> 11) & 0xFFFF));
                    platform->pit_counter[1] = 0; /* 0: read low byte, 1: read high byte */
                }
            } else if (port == 0x40) {
                platform->pit_counter[0] = (uint16_t)(*val);
            }
        } else {
            if (port == 0x40) {
                if (platform->pit_counter[1] == 0) {
                    /* Read LSB */
                    if (platform->pit_counter[0] == 0) {
                        uint64_t tsc = 0;
                        __asm__ volatile ("rdtsc" : "=A"(tsc));
                        platform->pit_counter[0] = (uint16_t)(0xFFFF - ((tsc >> 11) & 0xFFFF));
                    }
                    *val = platform->pit_counter[0] & 0xFF;
                    platform->pit_counter[1] = 1;
                } else {
                    /* Read MSB */
                    *val = (platform->pit_counter[0] >> 8) & 0xFF;
                    platform->pit_counter[1] = 0;
                    platform->pit_counter[0] = 0;
                }
            } else {
                *val = 0;
            }
        }
        return true;
    }

    /* 5. CMOS / RTC (0x70 - 0x71) */
    if (port == 0x70) {
        if (is_write) platform->rtc_index = (uint8_t)(*val & 0x7F);
        else *val = platform->rtc_index;
        return true;
    }
    if (port == 0x71) {
        if (is_write) platform->rtc_registers[platform->rtc_index & 0x7F] = (uint8_t)(*val);
        else *val = platform->rtc_registers[platform->rtc_index & 0x7F];
        return true;
    }

    /* 6. Legacy 8259 PIC (0x20/0x21, 0xA0/0xA1) */
    if (port == 0x20 || port == 0x21 || port == 0xA0 || port == 0xA1) {
        if (!is_write) *val = 0;
        return true;
    }

    /* Fallback default unhandled port */
    if (!is_write) *val = 0xFFFFFFFF;
    return true;
}

/* --------------------------------------------------------------------------
 * MMIO Dispatch (LAPIC, IOAPIC, VirtIO BAR1)
 * -------------------------------------------------------------------------- */
bool virtual_platform_handle_mmio(VirtualPlatform *platform, uint64_t gpa, bool is_write, uint8_t size, uint64_t *val) {
    if (!platform || !val) return false;

    /* 1. Local APIC (0xFEE00000 - 0xFEE00FFF) */
    if (gpa >= VIRTUAL_LAPIC_BASE_GPA && gpa < (VIRTUAL_LAPIC_BASE_GPA + VIRTUAL_LAPIC_SIZE)) {
        uint32_t offset = (uint32_t)(gpa - VIRTUAL_LAPIC_BASE_GPA);
        VirtualLAPIC *lapic = &platform->lapic;

        if (is_write) {
            uint32_t v = (uint32_t)(*val);
            switch (offset) {
                case 0x80:  lapic->tpr = v; break;
                case 0xB0:  lapic->eoi = 0; break;
                case 0xD0:  lapic->ldr = v; break;
                case 0xE0:  lapic->dfr = v; break;
                case 0xF0:  lapic->svr = v; lapic->enabled = (v & 0x100) != 0; break;
                case 0x280: lapic->esr = 0; break;
                case 0x300: lapic->icr_low = v; break;
                case 0x310: lapic->icr_high = v; break;
                case 0x320: lapic->lvt_timer = v; break;
                case 0x350: lapic->lvt_lint0 = v; break;
                case 0x360: lapic->lvt_lint1 = v; break;
                case 0x370: lapic->lvt_error = v; break;
                case 0x380: lapic->timer_initial = v; lapic->timer_current = v; break;
                case 0x3E0: lapic->timer_divide = v; break;
                default: break;
            }
        } else {
            switch (offset) {
                case 0x20:  *val = lapic->id; break;
                case 0x30:  *val = lapic->version; break;
                case 0x80:  *val = lapic->tpr; break;
                case 0xD0:  *val = lapic->ldr; break;
                case 0xE0:  *val = lapic->dfr; break;
                case 0xF0:  *val = lapic->svr; break;
                case 0x300: *val = lapic->icr_low; break;
                case 0x310: *val = lapic->icr_high; break;
                case 0x320: *val = lapic->lvt_timer; break;
                case 0x380: *val = lapic->timer_initial; break;
                case 0x390: *val = lapic->timer_current; break;
                case 0x3E0: *val = lapic->timer_divide; break;
                default:    *val = 0; break;
            }
        }
        return true;
    }

    /* 2. I/O APIC (0xFEC00000 - 0xFEC00FFF) */
    if (gpa >= VIRTUAL_IOAPIC_BASE_GPA && gpa < (VIRTUAL_IOAPIC_BASE_GPA + VIRTUAL_IOAPIC_SIZE)) {
        uint32_t offset = (uint32_t)(gpa - VIRTUAL_IOAPIC_BASE_GPA);
        VirtualIOAPIC *ioapic = &platform->ioapic;

        if (offset == 0x00) { /* IOREGSEL */
            if (is_write) ioapic->ioregsel = (uint32_t)(*val);
            else *val = ioapic->ioregsel;
            return true;
        } else if (offset == 0x10) { /* IOWIN */
            uint32_t reg = ioapic->ioregsel;
            if (reg == 0x00) { /* ID */
                if (is_write) ioapic->id = (uint32_t)(*val);
                else *val = ioapic->id;
            } else if (reg == 0x01) { /* Version / Max Redir Entry */
                if (!is_write) *val = ioapic->version;
            } else if (reg >= 0x10 && reg <= 0x3F) { /* Redirection Tables */
                uint32_t entry_idx = (reg - 0x10) / 2;
                bool is_high = (reg & 1) != 0;
                if (entry_idx < 24) {
                    if (is_write) {
                        if (is_high) {
                            ioapic->redirection_table[entry_idx] &= 0x00000000FFFFFFFFULL;
                            ioapic->redirection_table[entry_idx] |= ((uint64_t)(*val) << 32);
                        } else {
                            ioapic->redirection_table[entry_idx] &= 0xFFFFFFFF00000000ULL;
                            ioapic->redirection_table[entry_idx] |= (uint32_t)(*val);
                        }
                    } else {
                        if (is_high) *val = (uint32_t)(ioapic->redirection_table[entry_idx] >> 32);
                        else *val = (uint32_t)(ioapic->redirection_table[entry_idx]);
                    }
                }
            }
            return true;
        }
    }

    /* 3. VirtIO MMIO BAR1 (0xFEB00000 - 0xFEBFFFFF) */
    if (gpa >= 0xFEB00000ULL && gpa <= 0xFEBFFFFFULL && platform->vm && platform->vm->pci_bus) {
        VirtualPCIBus *pci_bus = platform->vm->pci_bus;
        for (uint32_t i = 0; i < pci_bus->device_count; i++) {
            VirtIOPCIDevice *pdev = pci_bus->devices[i];
            if (pdev && gpa >= pdev->mmio_bar_base && gpa < (pdev->mmio_bar_base + pdev->mmio_bar_size)) {
                uint32_t bar_offset = (uint32_t)(gpa - pdev->mmio_bar_base);
                if (is_write) {
                    virtio_pci_bar_write(pdev, bar_offset, (uint32_t)(*val), size);
                } else {
                    *val = virtio_pci_bar_read(pdev, bar_offset, size);
                }
                return true;
            }
        }
    }

    if (!is_write) *val = 0;
    return true;
}

/* --------------------------------------------------------------------------
 * CPUID Virtualization
 * -------------------------------------------------------------------------- */
void virtual_platform_handle_cpuid(vCPU *vcpu, uint32_t leaf, uint32_t subleaf, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx) {
    (void)vcpu;
    (void)subleaf;

    switch (leaf) {
        case 0x00000000:
            *eax = 0x00000016;       /* Max supported standard leaf */
            *ebx = 0x756e6547;       /* "Genu" */
            *edx = 0x49656e69;       /* "ineI" */
            *ecx = 0x6c65746e;       /* "ntel" */
            break;

        case 0x00000001:
            *eax = 0x000306C3;       /* Intel Haswell Core i3 Family/Model/Stepping */
            *ebx = 0x00010800;       /* 8 logical processors, default CLFLUSH size */
            *ecx = 0xF7FA3203;       /* SSE3, SSSE3, FMA, CX16, SSE4.1, SSE4.2, MOVBE, POPCNT, AES, AVX, Hypervisor=1 */
            *edx = 0xBFEBFBFF;       /* FPU, VME, DE, PSE, TSC, MSR, PAE, MCE, CX8, APIC, SEP, MTRR, PGE, MCA, CMOV, PAT, MMX, FXSR, SSE, SSE2 */
            break;

        case 0x00000004:             /* Deterministic Cache Parameters */
            *eax = 0x1C004121;       /* Unified L3 Cache */
            *ebx = 0x01C0003F;
            *ecx = 0x00000FFF;
            *edx = 0x00000006;
            break;

        case 0x00000007:             /* Structured Extended Features */
            *eax = 0;
            *ebx = 0x000002B9;       /* FSGSBASE, BMI1, AVX2, SMEP, BMI2, ERMS, RDSEED */
            *ecx = 0;
            *edx = 0;
            break;

        case 0x40000000:             /* Hypervisor Signature Leaf */
            *eax = 0x40000001;       /* Max hypervisor leaf */
            *ebx = 0x4D4F5441;       /* "ATOM" */
            *ecx = 0x4D565353;       /* "SSVM" */
            *edx = 0x00000000;
            break;

        case 0x80000000:             /* Max Extended Leaf */
            *eax = 0x80000008;
            *ebx = 0;
            *ecx = 0;
            *edx = 0;
            break;

        case 0x80000001:             /* Extended Processor Info & Features */
            *eax = 0x000306C3;
            *ebx = 0;
            *ecx = 0x00000121;       /* LAHF/SAHF, ABM, 3DNowPrefetch */
            *edx = 0x2C100800;       /* SYSCALL/SYSRET (bit 11), NX (bit 20), 1GB Page (bit 26), RDTSCP (bit 27), LM (bit 29) */
            break;

        case 0x80000008:             /* Address Size Information */
            *eax = 0x00003028;       /* 48-bit Virtual, 40-bit Physical Address */
            *ebx = 0;
            *ecx = 0;
            *edx = 0;
            break;

        default:
            *eax = 0;
            *ebx = 0;
            *ecx = 0;
            *edx = 0;
            break;
    }
}

/* --------------------------------------------------------------------------
 * MSR Virtualization
 * -------------------------------------------------------------------------- */
bool virtual_platform_handle_rdmsr(vCPU *vcpu, uint32_t msr, uint64_t *val) {
    if (!vcpu || !val) return false;

    switch (msr) {
        case 0x1B: /* IA32_APIC_BASE */
            *val = VIRTUAL_LAPIC_BASE_GPA | 0x800; /* APIC Enabled + BSP */
            return true;

        case 0xC0000080: /* IA32_EFER */
            *val = 0x00000D00; /* LME | LMA | NXE | SCE */
            return true;

        case 0xC0000081: /* IA32_STAR */
            *val = vcpu->msr_star;
            return true;

        case 0xC0000082: /* IA32_LSTAR */
            *val = vcpu->msr_lstar;
            return true;

        case 0xC0000084: /* IA32_FMASK */
            *val = vcpu->msr_fmask;
            return true;

        case 0xC0000100: /* IA32_FS_BASE */
            if (vcpu->vm && vcpu->vm->backend == HYPERVISOR_BACKEND_INTEL_VMX) {
                *val = vmx_vmread_field(VMCS_GUEST_FS_BASE);
            } else {
                *val = vcpu->msr_fs_base;
            }
            return true;

        case 0xC0000101: /* IA32_GS_BASE */
            if (vcpu->vm && vcpu->vm->backend == HYPERVISOR_BACKEND_INTEL_VMX) {
                *val = vmx_vmread_field(VMCS_GUEST_GS_BASE);
            } else {
                *val = vcpu->msr_gs_base;
            }
            return true;

        case 0xC0000102: /* IA32_KERNEL_GS_BASE (FreeBSD curthread pointer) */
            *val = vcpu->msr_kernel_gs_base;
            return true;

        case 0x10: /* IA32_TIME_STAMP_COUNTER */
            *val = 1000000ULL;
            return true;

        case 0x277: /* IA32_PAT */
            *val = 0x0007040600070406ULL; /* Standard PAT */
            return true;

        case 0x1A0: /* IA32_MISC_ENABLE */
            *val = 0x00000001; /* Fast strings enabled */
            return true;

        case 0x174: /* IA32_SYSENTER_CS */
        case 0x175: /* IA32_SYSENTER_ESP */
        case 0x176: /* IA32_SYSENTER_EIP */
            *val = 0;
            return true;

        default:
            *val = 0;
            return true;
    }
}

bool virtual_platform_handle_wrmsr(vCPU *vcpu, uint32_t msr, uint64_t val) {
    if (!vcpu) return false;

    switch (msr) {
        case 0x1B: /* IA32_APIC_BASE */
            return true;

        case 0xC0000080: /* IA32_EFER */
            vcpu->efer = val;
            if (vcpu->vm && vcpu->vm->backend == HYPERVISOR_BACKEND_INTEL_VMX) {
                vmx_vmwrite_field(VMCS_GUEST_IA32_EFER, val);
            }
            return true;

        case 0xC0000081: /* IA32_STAR */
            vcpu->msr_star = val;
            vmm_wrmsr_raw(0xC0000081, val);
            return true;

        case 0xC0000082: /* IA32_LSTAR */
            vcpu->msr_lstar = val;
            vmm_wrmsr_raw(0xC0000082, val);
            return true;

        case 0xC0000084: /* IA32_FMASK */
            vcpu->msr_fmask = val;
            vmm_wrmsr_raw(0xC0000084, val);
            return true;

        case 0xC0000100: /* IA32_FS_BASE */
            vcpu->msr_fs_base = val;
            if (vcpu->vm && vcpu->vm->backend == HYPERVISOR_BACKEND_INTEL_VMX) {
                vmx_vmwrite_field(VMCS_GUEST_FS_BASE, val);
            }
            return true;

        case 0xC0000101: /* IA32_GS_BASE */
            vcpu->msr_gs_base = val;
            if (vcpu->vm && vcpu->vm->backend == HYPERVISOR_BACKEND_INTEL_VMX) {
                vmx_vmwrite_field(VMCS_GUEST_GS_BASE, val);
            }
            return true;

        case 0xC0000102: /* IA32_KERNEL_GS_BASE */
            vcpu->msr_kernel_gs_base = val;
            vmm_wrmsr_raw(0xC0000102, val);
            return true;

        case 0x277: /* IA32_PAT */
            if (vcpu->vm && vcpu->vm->backend == HYPERVISOR_BACKEND_INTEL_VMX) {
                vmx_vmwrite_field(VMCS_GUEST_IA32_PAT, val);
            }
            return true;

        case 0x1A0: /* IA32_MISC_ENABLE */
        case 0x174: /* IA32_SYSENTER_CS */
        case 0x175: /* IA32_SYSENTER_ESP */
        case 0x176: /* IA32_SYSENTER_EIP */
            return true;

        default:
            return true;
    }
}
