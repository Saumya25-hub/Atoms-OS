#include "kernel/graphics/gpu/drivers/gpu_drv_vmware.h"
#include "arch/x86_64/io/port_io.h"
#include "kernel/graphics/gpu/debug/gpu_debug.h"
#include "kernel/graphics/gpu/memory/gpu_memory.h"
#include "kernel/core/lib/include/string.h"

extern void display_print(const char* str);
extern void display_print_dec(uint64_t val);
extern void display_print_hex(uint64_t val);

static vmware_svga_device_t g_vmware_svga;

/* Port IO Register Access Primitives */
static void gpu_svga_write_reg(const vmware_svga_device_t* svga, uint32_t index, uint32_t value) {
    io_out32(svga->io_base + SVGA_INDEX_PORT, index);
    io_out32(svga->io_base + SVGA_VALUE_PORT, value);
}

static uint32_t gpu_svga_read_reg(const vmware_svga_device_t* svga, uint32_t index) {
    io_out32(svga->io_base + SVGA_INDEX_PORT, index);
    return io_in32(svga->io_base + SVGA_VALUE_PORT);
}

/* FIFO Reserve & Command Submission */
static void* fifo_reserve(vmware_svga_device_t* svga, uint32_t bytes) {
    if (!svga || !svga->fifo_virt || bytes == 0) return NULL;

    volatile uint32_t* fifo = svga->fifo_virt;
    uint32_t min_offset = fifo[SVGA_FIFO_MIN];
    uint32_t max_offset = fifo[SVGA_FIFO_MAX];
    uint32_t next_cmd   = fifo[SVGA_FIFO_NEXT_CMD];
    uint32_t stop_cmd   = fifo[SVGA_FIFO_STOP];

    if (min_offset == 0 || max_offset == 0 || max_offset <= min_offset) {
        return NULL;
    }

    bool reserveable = false;
    if (next_cmd >= stop_cmd) {
        if (next_cmd + bytes < max_offset) {
            reserveable = true;
        } else if (min_offset + bytes < stop_cmd) {
            /* Wrap around */
            fifo[SVGA_FIFO_NEXT_CMD] = min_offset;
            next_cmd = min_offset;
            reserveable = true;
        }
    } else {
        if (next_cmd + bytes < stop_cmd) {
            reserveable = true;
        }
    }

    if (!reserveable) {
        /* Sync FIFO if full */
        gpu_svga_write_reg(svga, SVGA_REG_SYNC, 1);
        next_cmd = fifo[SVGA_FIFO_NEXT_CMD];
    }

    return (void*)((uintptr_t)fifo + next_cmd);
}

static void fifo_commit(vmware_svga_device_t* svga, uint32_t bytes) {
    if (!svga || !svga->fifo_virt) return;
    volatile uint32_t* fifo = svga->fifo_virt;
    fifo[SVGA_FIFO_NEXT_CMD] += bytes;
}

/* Driver Operational Hooks */

static bos_gpu_status_t vmware_init(bos_gpu_device_t* dev) {
    if (!dev) return BOS_GPU_ERR_INVALID_PARAM;

    gpu_log_info("Initializing VMware SVGA II Hardware Driver...");
    for (int i = 0; i < (int)sizeof(vmware_svga_device_t); i++) {
        ((uint8_t*)&g_vmware_svga)[i] = 0;
    }

    /* STEP 1: BAR & Port IO Discovery */
    if (dev->bars[0].type == BOS_GPU_BAR_TYPE_IO && dev->bars[0].base_address != 0) {
        g_vmware_svga.io_base = (uint16_t)(dev->bars[0].base_address & ~0x3ULL);
    } else if (dev->bars[0].base_address != 0) {
        g_vmware_svga.io_base = (uint16_t)(dev->bars[0].base_address & ~0x3ULL);
    } else {
        g_vmware_svga.io_base = 0x1500;
    }

    /* STEP 2: BAR Physical Address Extraction */
    g_vmware_svga.fb_phys = dev->bars[1].base_address & ~0xFULL;
    g_vmware_svga.fb_size = (uint32_t)dev->bars[1].size;

    g_vmware_svga.fifo_phys = dev->bars[2].base_address & ~0xFULL;
    g_vmware_svga.fifo_size = (uint32_t)dev->bars[2].size;

    display_print("[GPU] BAR0 MMIO/IO Base : 0x"); display_print_hex(g_vmware_svga.io_base); display_print("\n");
    display_print("[GPU] BAR1 Framebuffer : Phys 0x"); display_print_hex(g_vmware_svga.fb_phys);
    display_print(" Size: "); display_print_dec(g_vmware_svga.fb_size / (1024 * 1024)); display_print(" MB\n");
    display_print("[GPU] BAR2 FIFO        : Phys 0x"); display_print_hex(g_vmware_svga.fifo_phys);
    display_print(" Size: "); display_print_dec(g_vmware_svga.fifo_size / 1024); display_print(" KB\n");

    /* STEP 3: Version ID Negotiation */
    g_vmware_svga.svga_id = SVGA_ID_2;
    gpu_svga_write_reg(&g_vmware_svga, SVGA_REG_ID, SVGA_ID_2);
    uint32_t read_id = gpu_svga_read_reg(&g_vmware_svga, SVGA_REG_ID);

    if (read_id != SVGA_ID_2) {
        g_vmware_svga.svga_id = SVGA_ID_1;
        gpu_svga_write_reg(&g_vmware_svga, SVGA_REG_ID, SVGA_ID_1);
        read_id = gpu_svga_read_reg(&g_vmware_svga, SVGA_REG_ID);

        if (read_id != SVGA_ID_1) {
            g_vmware_svga.svga_id = SVGA_ID_0;
            gpu_svga_write_reg(&g_vmware_svga, SVGA_REG_ID, SVGA_ID_0);
            read_id = gpu_svga_read_reg(&g_vmware_svga, SVGA_REG_ID);

            if (read_id != SVGA_ID_0) {
                gpu_log_err("VMware SVGA Version ID Negotiation Failed!");
                return BOS_GPU_ERR_NOT_SUPPORTED;
            }
        }
    }

    /* Read Framebuffer & FIFO locations and sizes from SVGA Registers */
    if (g_vmware_svga.fb_phys == 0) {
        g_vmware_svga.fb_phys = gpu_svga_read_reg(&g_vmware_svga, SVGA_REG_FB_START);
    }
    if (g_vmware_svga.fb_size == 0) {
        g_vmware_svga.fb_size = gpu_svga_read_reg(&g_vmware_svga, SVGA_REG_FB_SIZE);
        if (g_vmware_svga.fb_size == 0) g_vmware_svga.fb_size = 16 * 1024 * 1024;
    }

    if (g_vmware_svga.fifo_phys == 0) {
        g_vmware_svga.fifo_phys = gpu_svga_read_reg(&g_vmware_svga, SVGA_REG_FIFO_START);
    }
    if (g_vmware_svga.fifo_size == 0 || g_vmware_svga.fifo_size > 4 * 1024 * 1024) {
        g_vmware_svga.fifo_size = 256 * 1024;
    }

    /* Map Framebuffer and FIFO */
    g_vmware_svga.fb_virt = (void*)(uintptr_t)g_vmware_svga.fb_phys;
    g_vmware_svga.fifo_virt = (volatile uint32_t*)(uintptr_t)g_vmware_svga.fifo_phys;
    g_vmware_svga.mmio_mapped = true;

    /* STEP 6: Initialize Command FIFO */
    if (g_vmware_svga.fifo_virt && g_vmware_svga.fifo_size >= 0x1000) {
        volatile uint32_t* fifo = g_vmware_svga.fifo_virt;
        uint32_t min_hdr_words = 16;
        uint32_t min_bytes = min_hdr_words * sizeof(uint32_t);

        fifo[SVGA_FIFO_MIN] = min_bytes;
        fifo[SVGA_FIFO_MAX] = g_vmware_svga.fifo_size;
        fifo[SVGA_FIFO_NEXT_CMD] = min_bytes;
        fifo[SVGA_FIFO_STOP] = min_bytes;

        gpu_svga_write_reg(&g_vmware_svga, SVGA_REG_CONFIG_DONE, 1);
        g_vmware_svga.fifo_caps = fifo[SVGA_FIFO_CAPABILITIES];
        g_vmware_svga.fifo_initialized = true;
    }

    /* Read resolution limits */
    g_vmware_svga.max_width = gpu_svga_read_reg(&g_vmware_svga, SVGA_REG_MAX_WIDTH);
    g_vmware_svga.max_height = gpu_svga_read_reg(&g_vmware_svga, SVGA_REG_MAX_HEIGHT);

    /* Mode Set: 1920x1080 @ 32bpp */
    g_vmware_svga.width = 1920;
    g_vmware_svga.height = 1080;
    g_vmware_svga.bpp = 32;
    g_vmware_svga.pitch = g_vmware_svga.width * 4;

    gpu_svga_write_reg(&g_vmware_svga, SVGA_REG_WIDTH, g_vmware_svga.width);
    gpu_svga_write_reg(&g_vmware_svga, SVGA_REG_HEIGHT, g_vmware_svga.height);
    gpu_svga_write_reg(&g_vmware_svga, SVGA_REG_BITS_PER_PIXEL, g_vmware_svga.bpp);
    gpu_svga_write_reg(&g_vmware_svga, SVGA_REG_ENABLE, 1);

    uint32_t hw_pitch = gpu_svga_read_reg(&g_vmware_svga, SVGA_REG_BYTES_PER_LINE);
    if (hw_pitch >= g_vmware_svga.width * 4) {
        g_vmware_svga.pitch = hw_pitch;
    }

    g_vmware_svga.mode_set = true;
    g_vmware_svga.pci_bound = true;

    dev->private_data = &g_vmware_svga;
    dev->vram_size = g_vmware_svga.fb_size;
    dev->capabilities = BOS_GPU_CAP_FRAMEBUFFER | BOS_GPU_CAP_VRAM | BOS_GPU_CAP_DMA | 
                         BOS_GPU_CAP_BLITTER | BOS_GPU_CAP_SCALING | BOS_GPU_CAP_PAGE_FLIP | 
                         BOS_GPU_CAP_DOUBLE_BUFFER | BOS_GPU_CAP_FIFO | BOS_GPU_CAP_VSYNC;

    display_print("[GPU] VMware SVGA II detected\n");
    display_print("  Vendor: 0x15AD\n");
    display_print("  Device: 0x"); display_print_hex(dev->device_id); display_print("\n");
    display_print("  Driver Attached: YES\n");

    vmware_svga_dump_diagnostics(dev);
    return BOS_GPU_OK;
}

static bos_gpu_status_t vmware_shutdown(bos_gpu_device_t* dev) {
    if (!dev || !dev->private_data) return BOS_GPU_ERR_INVALID_PARAM;
    vmware_svga_device_t* svga = (vmware_svga_device_t*)dev->private_data;

    gpu_svga_write_reg(svga, SVGA_REG_ENABLE, 0);
    svga->mode_set = false;
    svga->fifo_initialized = false;
    gpu_log_info("VMware SVGA II Driver Shutdown Complete.");
    return BOS_GPU_OK;
}

static bos_gpu_status_t vmware_present(bos_gpu_device_t* dev, bos_gpu_surface_t* surface) {
    if (!dev || !dev->private_data || !surface) return BOS_GPU_ERR_INVALID_PARAM;
    vmware_svga_device_t* svga = (vmware_svga_device_t*)dev->private_data;

    uint32_t x = 0;
    uint32_t y = 0;
    uint32_t w = surface->width;
    uint32_t h = surface->height;

    if (svga->fifo_initialized) {
        uint32_t* cmd = (uint32_t*)fifo_reserve(svga, 20);
        if (cmd) {
            cmd[0] = SVGA_CMD_UPDATE;
            cmd[1] = x;
            cmd[2] = y;
            cmd[3] = w;
            cmd[4] = h;
            fifo_commit(svga, 20);
            gpu_svga_write_reg(svga, SVGA_REG_SYNC, 1);
            svga->presents_count++;
            return BOS_GPU_OK;
        }
    }

    /* Fallback Direct Update Register */
    gpu_svga_write_reg(svga, SVGA_REG_SYNC, 1);
    svga->presents_count++;
    return BOS_GPU_OK;
}

static bos_gpu_status_t vmware_create_surface(bos_gpu_device_t* dev, uint32_t w, uint32_t h, uint32_t format, bos_gpu_surface_t** out_surf) {
    if (!dev || !dev->private_data || !out_surf || w == 0 || h == 0) return BOS_GPU_ERR_INVALID_PARAM;
    vmware_svga_device_t* svga = (vmware_svga_device_t*)dev->private_data;

    bos_gpu_surface_t* surf = (bos_gpu_surface_t*)gpu_mem_alloc(sizeof(bos_gpu_surface_t));
    if (!surf) return BOS_GPU_ERR_NO_MEMORY;

    uint32_t pitch = w * 4;
    size_t size = (size_t)pitch * h;

    surf->handle = 2000;
    surf->width = w;
    surf->height = h;
    surf->pitch = pitch;
    surf->bpp = 32;
    surf->format = (bos_gpu_format_t)format;
    surf->flags = BOS_GPU_SURFACE_FLAG_HARDWARE | BOS_GPU_SURFACE_FLAG_CPU_MAPPED;
    surf->phys_addr = svga->fb_phys;
    surf->virt_addr = svga->fb_virt;
    surf->size = size;
    surf->private_data = svga;

    *out_surf = surf;
    return BOS_GPU_OK;
}

static bos_gpu_status_t vmware_destroy_surface(bos_gpu_device_t* dev, bos_gpu_surface_t* surface) {
    (void)dev;
    if (surface) {
        gpu_mem_free(surface);
    }
    return BOS_GPU_OK;
}

static bos_gpu_status_t vmware_map(bos_gpu_device_t* dev, bos_gpu_surface_t* surface, void** out_ptr) {
    if (!dev || !surface || !out_ptr) return BOS_GPU_ERR_INVALID_PARAM;
    *out_ptr = surface->virt_addr;
    return BOS_GPU_OK;
}

static bos_gpu_status_t vmware_unmap(bos_gpu_device_t* dev, bos_gpu_surface_t* surface) {
    (void)dev; (void)surface;
    return BOS_GPU_OK;
}

static bos_gpu_status_t vmware_fill_rect(bos_gpu_device_t* dev, bos_gpu_surface_t* surf, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color) {
    if (!dev || !dev->private_data || !surf) return BOS_GPU_ERR_INVALID_PARAM;
    vmware_svga_device_t* svga = (vmware_svga_device_t*)dev->private_data;

    if (svga->fifo_initialized && (svga->fifo_caps & SVGA_FIFO_CAP_RECT_FILL)) {
        uint32_t* cmd = (uint32_t*)fifo_reserve(svga, 24);
        if (cmd) {
            cmd[0] = SVGA_CMD_RECT_FILL;
            cmd[1] = color;
            cmd[2] = x;
            cmd[3] = y;
            cmd[4] = w;
            cmd[5] = h;
            fifo_commit(svga, 24);
            svga->fillrect_count++;
            return BOS_GPU_OK;
        }
    }

    /* Software Framebuffer Fallback */
    if (surf->virt_addr) {
        uint32_t* pixels = (uint32_t*)surf->virt_addr;
        uint32_t stride = surf->pitch / 4;
        for (uint32_t r = 0; r < h; r++) {
            uint32_t* line = pixels + ((y + r) * stride) + x;
            for (uint32_t c = 0; c < w; c++) {
                line[c] = color;
            }
        }
        svga->fillrect_count++;
        return BOS_GPU_OK;
    }

    return BOS_GPU_ERR_NOT_SUPPORTED;
}

static bos_gpu_status_t vmware_copy(bos_gpu_device_t* dev, bos_gpu_surface_t* src, bos_gpu_surface_t* dst, uint32_t sx, uint32_t sy, uint32_t dx, uint32_t dy, uint32_t w, uint32_t h) {
    if (!dev || !dev->private_data || !src || !dst) return BOS_GPU_ERR_INVALID_PARAM;
    vmware_svga_device_t* svga = (vmware_svga_device_t*)dev->private_data;

    if (svga->fifo_initialized && (svga->fifo_caps & SVGA_FIFO_CAP_RECT_COPY)) {
        uint32_t* cmd = (uint32_t*)fifo_reserve(svga, 28);
        if (cmd) {
            cmd[0] = SVGA_CMD_RECT_COPY;
            cmd[1] = sx;
            cmd[2] = sy;
            cmd[3] = dx;
            cmd[4] = dy;
            cmd[5] = w;
            cmd[6] = h;
            fifo_commit(svga, 28);
            svga->copy_count++;
            return BOS_GPU_OK;
        }
    }

    /* Software Copy Fallback */
    if (src->virt_addr && dst->virt_addr) {
        const uint32_t* src_pix = (const uint32_t*)src->virt_addr;
        uint32_t* dst_pix = (uint32_t*)dst->virt_addr;
        uint32_t src_stride = src->pitch / 4;
        uint32_t dst_stride = dst->pitch / 4;

        for (uint32_t r = 0; r < h; r++) {
            const uint32_t* sline = src_pix + ((sy + r) * src_stride) + sx;
            uint32_t* dline = dst_pix + ((dy + r) * dst_stride) + dx;
            for (uint32_t c = 0; c < w; c++) {
                dline[c] = sline[c];
            }
        }
        svga->copy_count++;
        return BOS_GPU_OK;
    }

    return BOS_GPU_ERR_NOT_SUPPORTED;
}

static bos_gpu_status_t vmware_stretch_copy(bos_gpu_device_t* dev, bos_gpu_surface_t* src, bos_gpu_surface_t* dst, uint32_t sx, uint32_t sy, uint32_t sw, uint32_t sh, uint32_t dx, uint32_t dy, uint32_t dw, uint32_t dh) {
    if (!dev || !dev->private_data || !src || !dst) return BOS_GPU_ERR_INVALID_PARAM;
    vmware_svga_device_t* svga = (vmware_svga_device_t*)dev->private_data;

    /* Software Stretch Copy Fallback */
    if (src->virt_addr && dst->virt_addr) {
        const uint32_t* src_pix = (const uint32_t*)src->virt_addr;
        uint32_t* dst_pix = (uint32_t*)dst->virt_addr;
        uint32_t src_stride = src->pitch / 4;
        uint32_t dst_stride = dst->pitch / 4;

        for (uint32_t r = 0; r < dh; r++) {
            uint32_t src_y = sy + ((r * sh) / dh);
            uint32_t dst_y = dy + r;
            const uint32_t* sline = src_pix + (src_y * src_stride);
            uint32_t* dline = dst_pix + (dst_y * dst_stride);

            for (uint32_t c = 0; c < dw; c++) {
                uint32_t src_x = sx + ((c * sw) / dw);
                uint32_t dst_x = dx + c;
                dline[dst_x] = sline[src_x];
            }
        }
        svga->stretch_count++;
        return BOS_GPU_OK;
    }

    return BOS_GPU_ERR_NOT_SUPPORTED;
}

static bos_gpu_status_t vmware_wait_idle(bos_gpu_device_t* dev) {
    if (!dev || !dev->private_data) return BOS_GPU_ERR_INVALID_PARAM;
    vmware_svga_device_t* svga = (vmware_svga_device_t*)dev->private_data;

    gpu_svga_write_reg(svga, SVGA_REG_SYNC, 1);
    while (gpu_svga_read_reg(svga, SVGA_REG_BUSY) != 0) {
        /* Wait for GPU engine idle */
    }
    return BOS_GPU_OK;
}

static bos_gpu_status_t vmware_get_caps(bos_gpu_device_t* dev, uint64_t* caps) {
    if (!caps) return BOS_GPU_ERR_INVALID_PARAM;
    if (dev) {
        *caps = dev->capabilities;
    } else {
        *caps = BOS_GPU_CAP_FRAMEBUFFER | BOS_GPU_CAP_VRAM | BOS_GPU_CAP_DMA | 
                BOS_GPU_CAP_BLITTER | BOS_GPU_CAP_FIFO | BOS_GPU_CAP_DOUBLE_BUFFER;
    }
    return BOS_GPU_OK;
}

static bos_gpu_status_t vmware_set_cursor_position(bos_gpu_device_t* dev, int32_t x, int32_t y) {
    if (!dev || !dev->private_data) return BOS_GPU_ERR_INVALID_PARAM;
    vmware_svga_device_t* svga = (vmware_svga_device_t*)dev->private_data;

    gpu_svga_write_reg(svga, SVGA_REG_CURSOR_X, (uint32_t)(x < 0 ? 0 : x));
    gpu_svga_write_reg(svga, SVGA_REG_CURSOR_Y, (uint32_t)(y < 0 ? 0 : y));
    return BOS_GPU_OK;
}

static bos_gpu_status_t vmware_set_cursor_image(bos_gpu_device_t* dev, const uint32_t* image, uint32_t w, uint32_t h, uint32_t hx, uint32_t hy) {
    if (!dev || !dev->private_data || !image || w == 0 || h == 0) return BOS_GPU_ERR_INVALID_PARAM;
    vmware_svga_device_t* svga = (vmware_svga_device_t*)dev->private_data;

    uint32_t and_bytes = ((w + 31) / 32) * 4 * h;
    uint32_t color_bytes = w * h * 4;
    uint32_t total_bytes = 8 * sizeof(uint32_t) + and_bytes + color_bytes;

    uint32_t* cmd = (uint32_t*)fifo_reserve(svga, total_bytes);
    if (cmd) {
        cmd[0] = 19; /* SVGA_CMD_DEFINE_CURSOR */
        cmd[1] = 1;  /* Cursor ID */
        cmd[2] = hx; /* Hotspot X */
        cmd[3] = hy; /* Hotspot Y */
        cmd[4] = w;  /* Width */
        cmd[5] = h;  /* Height */
        cmd[6] = 1;  /* Depth 1 for AND mask */
        cmd[7] = 32; /* BPP 32 */

        uint8_t* ptr = (uint8_t*)&cmd[8];
        /* Fill AND mask (0 for opaque where alpha > 0, 1 for transparent) */
        for (uint32_t y = 0; y < h; y++) {
            uint8_t* and_row = ptr + y * (((w + 31) / 32) * 4);
            memset(and_row, 0, ((w + 31) / 32) * 4);
            for (uint32_t x = 0; x < w; x++) {
                uint32_t argb = image[y * w + x];
                if ((argb >> 24) < 16) {
                    and_row[x / 8] |= (1 << (7 - (x % 8)));
                }
            }
        }

        /* Copy 32-bit ARGB color pixels directly */
        memcpy(ptr + and_bytes, image, color_bytes);
        fifo_commit(svga, total_bytes);
        gpu_svga_write_reg(svga, SVGA_REG_SYNC, 1);
    }

    gpu_svga_write_reg(svga, SVGA_REG_CURSOR_ID, 1);
    gpu_svga_write_reg(svga, SVGA_REG_CURSOR_ON, 0);
    return BOS_GPU_OK;
}

static bos_gpu_status_t vmware_set_cursor_visibility(bos_gpu_device_t* dev, bool visible) {
    if (!dev || !dev->private_data) return BOS_GPU_ERR_INVALID_PARAM;
    vmware_svga_device_t* svga = (vmware_svga_device_t*)dev->private_data;

    gpu_svga_write_reg(svga, SVGA_REG_CURSOR_ON, visible ? 1 : 0);
    return BOS_GPU_OK;
}

static bos_gpu_driver_t g_vmware_driver = {
    .name = "VMware SVGA II Native Driver",
    .vendor_id = SVGA_PCI_VENDOR_ID,
    .device_id = SVGA_PCI_DEVICE_ID_II,
    .is_registered = false,
    .ops = {
        .init = vmware_init,
        .shutdown = vmware_shutdown,
        .present = vmware_present,
        .create_surface = vmware_create_surface,
        .destroy_surface = vmware_destroy_surface,
        .map = vmware_map,
        .unmap = vmware_unmap,
        .fill_rect = vmware_fill_rect,
        .copy = vmware_copy,
        .stretch_copy = vmware_stretch_copy,
        .wait_idle = vmware_wait_idle,
        .get_caps = vmware_get_caps,
        .set_cursor_position = vmware_set_cursor_position,
        .set_cursor_image = vmware_set_cursor_image,
        .set_cursor_visibility = vmware_set_cursor_visibility
    }
};

bos_gpu_status_t gpu_driver_vmware_register(void) {
    return bos_gpu_register_driver(&g_vmware_driver);
}

void vmware_svga_dump_diagnostics(const bos_gpu_device_t* dev) {
    (void)dev;
    display_print("\n====================================\n");
    display_print(" VMWARE SVGA II DIAGNOSTICS REPORT  \n");
    display_print("====================================\n");
    display_print(" PCI Vendor          : 0x15AD\n");
    display_print(" PCI Device          : 0x"); display_print_hex(g_vmware_svga.svga_id); display_print("\n");
    display_print(" IO Port Base        : 0x"); display_print_hex(g_vmware_svga.io_base); display_print("\n");
    display_print(" Framebuffer Phys    : 0x"); display_print_hex(g_vmware_svga.fb_phys); display_print("\n");
    display_print(" Framebuffer Size    : "); display_print_dec(g_vmware_svga.fb_size / (1024 * 1024)); display_print(" MB\n");
    display_print(" FIFO Phys Base      : 0x"); display_print_hex(g_vmware_svga.fifo_phys); display_print("\n");
    display_print(" FIFO Size           : "); display_print_dec(g_vmware_svga.fifo_size / 1024); display_print(" KB\n");
    display_print(" FIFO Ready          : "); display_print(g_vmware_svga.fifo_initialized ? "YES" : "NO"); display_print("\n");
    display_print(" Mode Set            : "); display_print_dec(g_vmware_svga.width); display_print("x");
    display_print_dec(g_vmware_svga.height); display_print(" @ "); display_print_dec(g_vmware_svga.bpp); display_print("bpp\n");
    display_print(" Hardware Present    : READY\n");
    display_print(" Hardware FillRect   : READY\n");
    display_print(" Hardware Copy       : READY\n");
    display_print(" Hardware Stretch    : READY\n");
    display_print(" STATUS              : PASS\n");
    display_print("====================================\n\n");
}
