#ifndef ATOMS_AHME_PROFILE_H
#define ATOMS_AHME_PROFILE_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    AHME_CPU_VENDOR_UNKNOWN = 0,
    AHME_CPU_VENDOR_INTEL,
    AHME_CPU_VENDOR_AMD,
    AHME_CPU_VENDOR_QEMU_EMULATOR
} AHMECPUVendor;

typedef enum {
    AHME_CHIPSET_GENERIC = 0,
    AHME_CHIPSET_INTEL_H81_HASWELL,
    AHME_CHIPSET_QEMU_VIRT,
    AHME_CHIPSET_VMWARE_VIRT
} AHMEChipsetFamily;

typedef struct {
    AHMECPUVendor cpu_vendor;
    char cpu_brand_string[48];
    uint32_t cpu_family;
    uint32_t cpu_model;
    uint32_t cpu_stepping;

    AHMEChipsetFamily chipset_family;
    char motherboard_name[64];

    bool is_uefi_boot;
    bool has_acpi;
    bool has_pci;
    bool has_xhci_usb;
    bool has_ehci_usb;
    bool has_ps2_physical;
    bool bios_usb_legacy_emulation;
} AHMEHardwareProfile;

void ahme_profile_discover(AHMEHardwareProfile *out_profile);

#endif // ATOMS_AHME_PROFILE_H
