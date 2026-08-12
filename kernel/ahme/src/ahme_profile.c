#include "kernel/ahme/include/ahme_profile.h"
#include "kernel/core/lib/include/string.h"
#include <stdint.h>

static void cpuid_query(uint32_t leaf, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx) {
    __asm__ volatile("cpuid"
                     : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
                     : "a"(leaf));
}

void ahme_profile_discover(AHMEHardwareProfile *out_profile) {
    if (!out_profile) return;
    memset(out_profile, 0, sizeof(AHMEHardwareProfile));

    uint32_t eax, ebx, ecx, edx;
    cpuid_query(0, &eax, &ebx, &ecx, &edx);

    char vendor_str[13];
    *(uint32_t*)(&vendor_str[0]) = ebx;
    *(uint32_t*)(&vendor_str[4]) = edx;
    *(uint32_t*)(&vendor_str[8]) = ecx;
    vendor_str[12] = '\0';

    if (strcmp(vendor_str, "GenuineIntel") == 0) {
        out_profile->cpu_vendor = AHME_CPU_VENDOR_INTEL;
    } else if (strcmp(vendor_str, "AuthenticAMD") == 0) {
        out_profile->cpu_vendor = AHME_CPU_VENDOR_AMD;
    } else if (strcmp(vendor_str, "TCGTCGTCGTCG") == 0 || strcmp(vendor_str, "KVMKVMKVM") == 0) {
        out_profile->cpu_vendor = AHME_CPU_VENDOR_QEMU_EMULATOR;
    }

    // CPUID Leaf 1: Family & Model
    cpuid_query(1, &eax, &ebx, &ecx, &edx);
    out_profile->cpu_stepping = eax & 0xF;
    out_profile->cpu_model = (eax >> 4) & 0xF;
    out_profile->cpu_family = (eax >> 8) & 0xF;
    if (out_profile->cpu_family == 0xF) {
        out_profile->cpu_family += (eax >> 20) & 0xFF;
    }
    if (out_profile->cpu_family == 6 || out_profile->cpu_family == 15) {
        out_profile->cpu_model += ((eax >> 16) & 0xF) << 4;
    }

    // Haswell CPU Model 60 (0x3C), 69 (0x45), 70 (0x46) -> Intel H81/Haswell Chipset
    if (out_profile->cpu_vendor == AHME_CPU_VENDOR_INTEL &&
        (out_profile->cpu_model == 60 || out_profile->cpu_model == 69 || out_profile->cpu_model == 70 || out_profile->cpu_family == 6)) {
        out_profile->chipset_family = AHME_CHIPSET_INTEL_H81_HASWELL;
        strcpy(out_profile->motherboard_name, "Intel H81 Haswell LGA1150 Board");
        out_profile->has_ps2_physical = false; // Emulated SMM
        out_profile->bios_usb_legacy_emulation = true;
    } else if (out_profile->cpu_vendor == AHME_CPU_VENDOR_QEMU_EMULATOR) {
        out_profile->chipset_family = AHME_CHIPSET_QEMU_VIRT;
        strcpy(out_profile->motherboard_name, "QEMU Virtual Chipset");
        out_profile->has_ps2_physical = true;
    } else {
        out_profile->chipset_family = AHME_CHIPSET_GENERIC;
        strcpy(out_profile->motherboard_name, "Generic x86_64 Motherboard");
        out_profile->has_ps2_physical = true;
    }

    out_profile->is_uefi_boot = true;
    out_profile->has_acpi = true;
    out_profile->has_pci = true;
    out_profile->has_xhci_usb = true;
}
