/**
 * @file display_detection.c
 * @brief ATOMS OS Display Intelligence Engine - Hardware Detection Implementation
 */

#include "display_detection.h"
#include "arch/x86_64/io/port_io.h"

static uint32_t die_pci_read(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address = (uint32_t)((bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));
    io_out32(0xCF8, address);
    return io_in32(0xCFC);
}

void DIE_Detection_DetectEnvironment(DIE_EnvironmentType* out_env, char* out_env_name, char* out_ctrl_name) {
    if (!out_env || !out_env_name || !out_ctrl_name) return;

    *out_env = DIE_ENV_PHYSICAL_PC;
    for (int i = 0; i < 32; i++) out_env_name[i] = '\0';
    for (int i = 0; i < 64; i++) out_ctrl_name[i] = '\0';

    /* Default strings */
    char def_env[] = "Physical Hardware";
    char def_ctrl[] = "Standard VBE/PCI Graphics";
    for (int i = 0; def_env[i] != '\0' && i < 31; i++) out_env_name[i] = def_env[i];
    for (int i = 0; def_ctrl[i] != '\0' && i < 63; i++) out_ctrl_name[i] = def_ctrl[i];

    /* Probe PCI bus for display controllers (Class 0x03) or known hypervisor graphics devices */
    for (uint32_t bus = 0; bus < 4; bus++) {
        for (uint32_t slot = 0; slot < 32; slot++) {
            uint32_t vd = die_pci_read((uint8_t)bus, (uint8_t)slot, 0, 0);
            if (vd == 0xFFFFFFFF || vd == 0x00000000) continue;

            uint16_t vendor_id = (uint16_t)(vd & 0xFFFF);
            uint16_t device_id = (uint16_t)((vd >> 16) & 0xFFFF);
            uint32_t class_reg = die_pci_read((uint8_t)bus, (uint8_t)slot, 0, 0x08);
            uint8_t base_class = (uint8_t)((class_reg >> 24) & 0xFF);

            /* Check known hypervisor / emulator PCI IDs */
            if (vendor_id == 0x80EE) {
                *out_env = DIE_ENV_VIRTUALBOX;
                char e[] = "VirtualBox VM";
                char c[] = "VirtualBox VBoxSVGA / VBoxVGA";
                for (int j = 0; j < 31 && e[j] != '\0'; j++) out_env_name[j] = e[j];
                out_env_name[31] = '\0';
                for (int j = 0; j < 63 && c[j] != '\0'; j++) out_ctrl_name[j] = c[j];
                out_ctrl_name[63] = '\0';
                return;
            }
            if (vendor_id == 0x1234 || (vendor_id == 0x1AF4 && device_id == 0x1050)) {
                *out_env = DIE_ENV_QEMU;
                char e[] = "QEMU Virtual Machine";
                char c[] = "QEMU Standard VGA / Bochs VBE";
                for (int j = 0; j < 31 && e[j] != '\0'; j++) out_env_name[j] = e[j];
                out_env_name[31] = '\0';
                for (int j = 0; j < 63 && c[j] != '\0'; j++) out_ctrl_name[j] = c[j];
                out_ctrl_name[63] = '\0';
                return;
            }
            if (vendor_id == 0x15AD) {
                *out_env = DIE_ENV_VMWARE;
                char e[] = "VMware Virtual Machine";
                char c[] = "VMware SVGA II Controller";
                for (int j = 0; j < 31 && e[j] != '\0'; j++) out_env_name[j] = e[j];
                out_env_name[31] = '\0';
                for (int j = 0; j < 63 && c[j] != '\0'; j++) out_ctrl_name[j] = c[j];
                out_ctrl_name[63] = '\0';
                return;
            }
            if (vendor_id == 0x1414 && device_id == 0x5353) {
                *out_env = DIE_ENV_HYPERV;
                char e[] = "Microsoft Hyper-V";
                char c[] = "Hyper-V Synthetic Video";
                for (int j = 0; j < 31 && e[j] != '\0'; j++) out_env_name[j] = e[j];
                out_env_name[31] = '\0';
                for (int j = 0; j < 63 && c[j] != '\0'; j++) out_ctrl_name[j] = c[j];
                out_ctrl_name[63] = '\0';
                return;
            }

            /* If it is a bare metal display controller (Class 0x03) */
            if (base_class == 0x03) {
                if (vendor_id == 0x8086) {
                    char e[] = "Physical PC (Intel)";
                    char c[] = "Intel Integrated PCI Graphics";
                    for (int j = 0; j < 31 && e[j] != '\0'; j++) out_env_name[j] = e[j];
                    for (int j = 0; j < 63 && c[j] != '\0'; j++) out_ctrl_name[j] = c[j];
                } else if (vendor_id == 0x1002) {
                    char e[] = "Physical PC (AMD)";
                    char c[] = "AMD/ATI Radeon Graphics";
                    for (int j = 0; j < 31 && e[j] != '\0'; j++) out_env_name[j] = e[j];
                    for (int j = 0; j < 63 && c[j] != '\0'; j++) out_ctrl_name[j] = c[j];
                } else if (vendor_id == 0x10DE) {
                    char e[] = "Physical PC (NVIDIA)";
                    char c[] = "NVIDIA GeForce / Quadro Graphics";
                    for (int j = 0; j < 31 && e[j] != '\0'; j++) out_env_name[j] = e[j];
                    for (int j = 0; j < 63 && c[j] != '\0'; j++) out_ctrl_name[j] = c[j];
                }
            }
        }
    }
}

uint32_t DIE_Detection_GetVRAMSize(void) {
    /* Return nominal 64 MB minimum VRAM allocation supported by VBE linear framebuffer */
    return 64 * 1024 * 1024;
}
