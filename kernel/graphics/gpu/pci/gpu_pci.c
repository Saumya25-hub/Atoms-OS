#include "kernel/graphics/gpu/pci/gpu_pci.h"
#include "kernel/core/pci/pci.h"
#include "kernel/graphics/gpu/debug/gpu_debug.h"

extern void display_print(const char* str);
extern void display_print_dec(uint64_t val);
extern void display_print_hex(uint64_t val);

extern bos_gpu_device_t* gpu_manager_register_pci_device(const PCIDevice* pci_dev, const char* name_hint);

static const char* gpu_get_vendor_name(uint16_t vendor_id) {
    switch (vendor_id) {
        case BOS_GPU_VENDOR_VMWARE: return "VMware SVGA Adapter";
        case BOS_GPU_VENDOR_VIRTIO: return "VirtIO GPU Adapter";
        case BOS_GPU_VENDOR_INTEL:  return "Intel Graphics Controller";
        case BOS_GPU_VENDOR_AMD:    return "AMD Radeon GPU";
        case BOS_GPU_VENDOR_NVIDIA: return "NVIDIA GeForce/Quadro GPU";
        default:                    return "Generic PCI Display Adapter";
    }
}

uint32_t gpu_pci_scan_and_register(void) {
    gpu_log_info("Starting PCI Scan for Display Controllers...");
    uint32_t count = pci_get_device_count();
    uint32_t found_count = 0;

    for (uint32_t i = 0; i < count; i++) {
        PCIDevice* pdev = pci_get_device(i);
        if (!pdev) continue;

        /* Class 0x03 is Display Controller */
        if (pdev->base_class == 0x03) {
            display_print("[GPU PCI] Found Display Controller at PCI ");
            display_print_dec(pdev->bus); display_print(":");
            display_print_dec(pdev->slot); display_print(".");
            display_print_dec(pdev->func); display_print(" Vendor: ");
            display_print_hex(pdev->vendor_id); display_print(" Device: ");
            display_print_hex(pdev->device_id); display_print("\n");

            const char* name_hint = gpu_get_vendor_name(pdev->vendor_id);
            bos_gpu_device_t* dev = gpu_manager_register_pci_device(pdev, name_hint);
            if (dev) {
                found_count++;
            }
        }
    }

    gpu_log_info("PCI Scan Complete.");
    return found_count;
}
