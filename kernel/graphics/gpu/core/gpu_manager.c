#include "kernel/graphics/gpu/include/gpu.h"
#include "kernel/core/pci/pci.h"
#include "kernel/graphics/gpu/pci/gpu_pci.h"
#include "kernel/graphics/gpu/memory/gpu_memory.h"
#include "kernel/graphics/gpu/surface/gpu_surface.h"
#include "kernel/graphics/gpu/debug/gpu_debug.h"

extern void display_print(const char* str);
extern void display_print_dec(uint64_t val);
extern void display_print_hex(uint64_t val);

/* Declarations for registration functions of drivers */
extern bos_gpu_status_t gpu_driver_vmware_register(void);
extern bos_gpu_status_t gpu_driver_virtio_register(void);
extern bos_gpu_status_t gpu_driver_intel_register(void);
extern bos_gpu_status_t gpu_driver_amd_register(void);
extern bos_gpu_status_t gpu_driver_nvidia_register(void);
extern bos_gpu_status_t gpu_driver_swrender_register(void);
extern bos_gpu_driver_t* gpu_driver_swrender_get(void);

static bos_gpu_device_t g_gpu_devices[MAX_GPU_DEVICES];
static uint32_t         g_gpu_device_count = 0;

static bos_gpu_driver_t* g_gpu_drivers[MAX_GPU_DRIVERS];
static uint32_t          g_gpu_driver_count = 0;

static bos_gpu_device_t* g_primary_gpu = NULL;
static bool              g_gpu_subsystem_initialized = false;

/* Register a discovered PCI device into subsystem device list */
bos_gpu_device_t* gpu_manager_register_pci_device(const PCIDevice* pci_dev, const char* name_hint) {
    if (!pci_dev || g_gpu_device_count >= MAX_GPU_DEVICES) return NULL;

    bos_gpu_device_t* dev = &g_gpu_devices[g_gpu_device_count];
    dev->device_id_idx = g_gpu_device_count;
    dev->vendor_id = pci_dev->vendor_id;
    dev->device_id = pci_dev->device_id;
    dev->bus = pci_dev->bus;
    dev->slot = pci_dev->slot;
    dev->func = pci_dev->func;
    dev->irq = pci_dev->interrupt_line;
    dev->vram_size = 0;
    dev->capabilities = 0;
    dev->ops = NULL;
    dev->private_data = NULL;
    dev->is_initialized = false;
    dev->is_active = false;
    dev->is_primary = false;
    dev->driver_name[0] = '\0';

    if (name_hint && name_hint[0] != '\0') {
        size_t idx = 0;
        while (name_hint[idx] && idx < 63) {
            dev->name[idx] = name_hint[idx];
            idx++;
        }
        dev->name[idx] = '\0';
    } else {
        const char default_name[] = "PCI Graphics Adapter";
        for (int i = 0; i < 21; i++) dev->name[i] = default_name[i];
    }

    /* Extract BAR info */
    for (int b = 0; b < 6; b++) {
        dev->bars[b].index = (uint8_t)b;
        dev->bars[b].base_address = pci_dev->bars[b].base_address;
        dev->bars[b].size = pci_dev->bars[b].size;
        dev->bars[b].prefetchable = pci_dev->bars[b].prefetchable;

        switch (pci_dev->bars[b].type) {
            case PCI_BAR_TYPE_MMIO32: dev->bars[b].type = BOS_GPU_BAR_TYPE_MMIO32; break;
            case PCI_BAR_TYPE_MMIO64: dev->bars[b].type = BOS_GPU_BAR_TYPE_MMIO64; break;
            case PCI_BAR_TYPE_IO:     dev->bars[b].type = BOS_GPU_BAR_TYPE_IO; break;
            default:                  dev->bars[b].type = BOS_GPU_BAR_TYPE_UNUSED; break;
        }

        /* Infer VRAM size from largest memory BAR */
        if (dev->bars[b].type != BOS_GPU_BAR_TYPE_UNUSED && dev->bars[b].type != BOS_GPU_BAR_TYPE_IO) {
            if (dev->bars[b].size > dev->vram_size) {
                dev->vram_size = dev->bars[b].size;
            }
        }
    }

    g_gpu_device_count++;
    gpu_log_info("GPU Found");
    return dev;
}

static void gpu_manager_register_builtin_drivers(void) {
    gpu_driver_vmware_register();
    gpu_driver_virtio_register();
    gpu_driver_intel_register();
    gpu_driver_amd_register();
    gpu_driver_nvidia_register();
    gpu_driver_swrender_register();
}

static bos_gpu_driver_t* gpu_manager_find_driver_for_device(bos_gpu_device_t* dev) {
    if (!dev) return NULL;
    for (uint32_t i = 0; i < g_gpu_driver_count; i++) {
        bos_gpu_driver_t* drv = g_gpu_drivers[i];
        if (!drv) continue;

        if (drv->vendor_id == dev->vendor_id) {
            if (drv->device_id == 0xFFFF || drv->device_id == dev->device_id || 
               (drv->vendor_id == BOS_GPU_VENDOR_VIRTIO && (dev->device_id == 0x1050 || dev->device_id == 0x1000))) {
                return drv;
            }
        } else if (drv->vendor_id == BOS_GPU_VENDOR_VMWARE && dev->vendor_id == 0x15AD) {
            return drv;
        }
    }
    return NULL;
}

static void gpu_manager_bind_drivers(void) {
    bool hw_driver_attached = false;

    for (uint32_t i = 0; i < g_gpu_device_count; i++) {
        bos_gpu_device_t* dev = &g_gpu_devices[i];
        bos_gpu_driver_t* drv = gpu_manager_find_driver_for_device(dev);

        if (drv) {
            dev->ops = &drv->ops;
            size_t idx = 0;
            while (drv->name[idx] && idx < 63) {
                dev->driver_name[idx] = drv->name[idx];
                idx++;
            }
            dev->driver_name[idx] = '\0';

            /* Initialize device via driver */
            bos_gpu_status_t st = BOS_GPU_ERR_NOT_IMPLEMENTED;
            if (dev->ops->init) {
                st = dev->ops->init(dev);
            }

            /* Retrieve Capabilities */
            if (dev->ops->get_caps) {
                dev->ops->get_caps(dev, &dev->capabilities);
            }

            gpu_log_info("Driver Loaded");
            display_print("  Device: "); display_print(dev->name);
            display_print(" -> Driver: "); display_print(dev->driver_name); display_print("\n");

            if (st == BOS_GPU_OK && dev->vendor_id != BOS_GPU_VENDOR_SOFTWARE) {
                dev->is_initialized = true;
                dev->is_active = true;
                if (!g_primary_gpu) {
                    g_primary_gpu = dev;
                    dev->is_primary = true;
                }
                hw_driver_attached = true;
            }
        }
    }

    /* Fallback to Software Renderer if no active hardware acceleration driver */
    if (!hw_driver_attached || !g_primary_gpu) {
        gpu_log_info("Using Software Renderer");
        
        /* Register Fallback Software Device */
        if (g_gpu_device_count < MAX_GPU_DEVICES) {
            bos_gpu_device_t* sw_dev = &g_gpu_devices[g_gpu_device_count];
            sw_dev->device_id_idx = g_gpu_device_count;
            sw_dev->vendor_id = BOS_GPU_VENDOR_SOFTWARE;
            sw_dev->device_id = 0x0001;
            
            const char sw_name[] = "BOS Software Renderer Fallback";
            for (int i = 0; i <= 31; i++) sw_dev->name[i] = sw_name[i];

            bos_gpu_driver_t* sw_drv = gpu_driver_swrender_get();
            if (sw_drv) {
                sw_dev->ops = &sw_drv->ops;
                const char sw_drv_name[] = "Software Renderer";
                for (int i = 0; i <= 17; i++) sw_dev->driver_name[i] = sw_drv_name[i];
            }

            sw_dev->capabilities = BOS_GPU_CAP_FRAMEBUFFER | BOS_GPU_CAP_DOUBLE_BUFFER;
            sw_dev->vram_size = 32 * 1024 * 1024;
            sw_dev->is_initialized = true;
            sw_dev->is_active = true;
            sw_dev->is_primary = true;
            
            if (sw_dev->ops && sw_dev->ops->init) {
                sw_dev->ops->init(sw_dev);
            }

            g_primary_gpu = sw_dev;
            g_gpu_device_count++;
        }
    }
}

bos_gpu_status_t bos_gpu_init(void) {
    if (g_gpu_subsystem_initialized) return BOS_GPU_OK;

    gpu_log_info("GPU Manager Started");
    g_gpu_device_count = 0;
    g_gpu_driver_count = 0;
    g_primary_gpu = NULL;

    /* Register standard placeholder & sw drivers */
    gpu_manager_register_builtin_drivers();

    /* Scan PCI bus for graphics hardware */
    gpu_pci_scan_and_register();

    /* Bind best driver to each detected device */
    gpu_manager_bind_drivers();

    g_gpu_subsystem_initialized = true;
    return BOS_GPU_OK;
}

bos_gpu_status_t bos_gpu_shutdown(void) {
    if (!g_gpu_subsystem_initialized) return BOS_GPU_OK;

    for (uint32_t i = 0; i < g_gpu_device_count; i++) {
        bos_gpu_device_t* dev = &g_gpu_devices[i];
        if (dev->ops && dev->ops->shutdown) {
            dev->ops->shutdown(dev);
        }
        dev->is_active = false;
        dev->is_initialized = false;
    }

    g_gpu_device_count = 0;
    g_gpu_driver_count = 0;
    g_primary_gpu = NULL;
    g_gpu_subsystem_initialized = false;
    
    gpu_log_info("GPU Subsystem Shutdown Complete.");
    return BOS_GPU_OK;
}

uint32_t bos_gpu_enumerate(void) {
    return g_gpu_device_count;
}

bos_gpu_status_t bos_gpu_register_driver(bos_gpu_driver_t* driver) {
    if (!driver || g_gpu_driver_count >= MAX_GPU_DRIVERS) {
        return BOS_GPU_ERR_INVALID_PARAM;
    }

    for (uint32_t i = 0; i < g_gpu_driver_count; i++) {
        if (g_gpu_drivers[i] == driver) {
            return BOS_GPU_OK;
        }
    }

    g_gpu_drivers[g_gpu_driver_count++] = driver;
    driver->is_registered = true;

    display_print("[GPU HAL] Registered Driver: ");
    display_print(driver->name);
    display_print("\n");

    return BOS_GPU_OK;
}

bos_gpu_device_t* bos_gpu_get_primary(void) {
    return g_primary_gpu;
}

bos_gpu_status_t bos_gpu_get_caps(bos_gpu_device_t* dev, uint64_t* out_caps) {
    if (!out_caps) return BOS_GPU_ERR_INVALID_PARAM;
    bos_gpu_device_t* target = dev ? dev : g_primary_gpu;
    if (!target) {
        *out_caps = BOS_GPU_CAP_NONE;
        return BOS_GPU_ERR_GENERIC;
    }
    *out_caps = target->capabilities;
    return BOS_GPU_OK;
}

bos_gpu_status_t bos_gpu_present(bos_gpu_surface_t* surface) {
    if (!surface) return BOS_GPU_ERR_INVALID_PARAM;
    bos_gpu_device_t* dev = g_primary_gpu;
    if (!dev || !dev->ops || !dev->ops->present) {
        return BOS_GPU_ERR_NOT_SUPPORTED;
    }
    return dev->ops->present(dev, surface);
}

uint32_t bos_gpu_get_device_count(void) {
    return g_gpu_device_count;
}

bos_gpu_device_t* bos_gpu_get_device(uint32_t index) {
    if (index >= g_gpu_device_count) return NULL;
    return &g_gpu_devices[index];
}

bos_gpu_status_t bos_gpu_set_primary(bos_gpu_device_t* dev) {
    if (!dev) return BOS_GPU_ERR_INVALID_PARAM;
    for (uint32_t i = 0; i < g_gpu_device_count; i++) {
        g_gpu_devices[i].is_primary = false;
    }
    dev->is_primary = true;
    g_primary_gpu = dev;
    return BOS_GPU_OK;
}

void bos_gpu_print_diagnostics(void) {
    display_print("\n=========================================\n");
    display_print(" BOS GPU Subsystem Diagnostics (Phase 1)\n");
    display_print("=========================================\n");
    display_print("Total Devices Registered : "); display_print_dec(g_gpu_device_count); display_print("\n");
    display_print("Total Drivers Registered : "); display_print_dec(g_gpu_driver_count); display_print("\n");
    display_print("Primary GPU Device       : "); 
    display_print(g_primary_gpu ? g_primary_gpu->name : "None"); display_print("\n\n");

    for (uint32_t i = 0; i < g_gpu_device_count; i++) {
        gpu_dump_device_info(&g_gpu_devices[i]);
    }
}
