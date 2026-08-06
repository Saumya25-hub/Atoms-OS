#include "kernel/graphics/gpu/debug/gpu_debug.h"
#include <stdarg.h>

extern void display_print(const char* str);
extern void display_print_dec(uint64_t val);
extern void display_print_hex(uint64_t val);

static void int_to_hex_str(uint64_t val, char* buf, int width) {
    const char hex_chars[] = "0123456789ABCDEF";
    buf[0] = '0';
    buf[1] = 'x';
    for (int i = width - 1; i >= 0; i--) {
        buf[2 + i] = hex_chars[val & 0xF];
        val >>= 4;
    }
    buf[2 + width] = '\0';
}

static void int_to_dec_str(uint64_t val, char* buf) {
    if (val == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }
    char temp[32];
    int idx = 0;
    while (val > 0) {
        temp[idx++] = '0' + (val % 10);
        val /= 10;
    }
    int pos = 0;
    while (idx > 0) {
        buf[pos++] = temp[--idx];
    }
    buf[pos] = '\0';
}

void gpu_log_info(const char* msg) {
    display_print("[GPU LOG] ");
    display_print(msg);
    display_print("\n");
}

void gpu_log_warn(const char* msg) {
    display_print("[GPU WARN] ");
    display_print(msg);
    display_print("\n");
}

void gpu_log_err(const char* msg) {
    display_print("[GPU ERROR] ");
    display_print(msg);
    display_print("\n");
}

void gpu_dump_capabilities(uint64_t caps) {
    display_print("  Capabilities (");
    display_print_hex(caps);
    display_print("):\n");

    if (caps & BOS_GPU_CAP_FRAMEBUFFER)   display_print("    - Framebuffer\n");
    if (caps & BOS_GPU_CAP_VRAM)          display_print("    - VRAM Support\n");
    if (caps & BOS_GPU_CAP_DMA)           display_print("    - Direct Memory Access (DMA)\n");
    if (caps & BOS_GPU_CAP_BLITTER)       display_print("    - 2D Hardware Blitter\n");
    if (caps & BOS_GPU_CAP_SCALING)       display_print("    - Hardware Scaling\n");
    if (caps & BOS_GPU_CAP_OVERLAY)       display_print("    - Hardware Overlay\n");
    if (caps & BOS_GPU_CAP_HW_CURSOR)     display_print("    - Hardware Cursor\n");
    if (caps & BOS_GPU_CAP_PAGE_FLIP)     display_print("    - Hardware Page Flip\n");
    if (caps & BOS_GPU_CAP_DOUBLE_BUFFER) display_print("    - Double Buffer\n");
    if (caps & BOS_GPU_CAP_TRIPLE_BUFFER) display_print("    - Triple Buffer\n");
    if (caps & BOS_GPU_CAP_VSYNC)         display_print("    - VSync Support\n");
    if (caps & BOS_GPU_CAP_FIFO)          display_print("    - Hardware FIFO Command Buffer\n");
}

void gpu_dump_device_info(const bos_gpu_device_t* dev) {
    if (!dev) return;
    display_print("-----------------------------------------\n");
    display_print(" GPU Device Details: ");
    display_print(dev->name);
    display_print("\n-----------------------------------------\n");
    display_print("  Vendor ID   : "); display_print_hex(dev->vendor_id); display_print("\n");
    display_print("  Device ID   : "); display_print_hex(dev->device_id); display_print("\n");
    display_print("  Driver Name : "); display_print(dev->driver_name[0] ? dev->driver_name : "Unattached"); display_print("\n");
    display_print("  PCI Address : "); display_print_dec(dev->bus); display_print(":");
    display_print_dec(dev->slot); display_print("."); display_print_dec(dev->func); display_print("\n");
    display_print("  VRAM Size   : "); display_print_dec(dev->vram_size / (1024 * 1024)); display_print(" MB\n");
    display_print("  Primary GPU : "); display_print(dev->is_primary ? "YES" : "NO"); display_print("\n");
    gpu_dump_capabilities(dev->capabilities);
    display_print("-----------------------------------------\n");
}
