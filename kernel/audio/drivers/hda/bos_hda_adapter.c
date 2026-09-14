/*
 * ATOMS OS — Intel High Definition Audio (HDA) Driver
 * BOS Audio HAL Adapter & DMA Stream Engine
 *
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 * Hardware Interface Constants Adapted from FreeBSD snd_hda (BSD-2-Clause)
 */

#include "bos_hda.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/core/memory/vmm/include/vmm.h"
#include "kernel/core/memory/vmm/include/paging.h"
#include "kernel/core/memory/pmm/include/pmm.h"
#include "kernel/drivers/display/display.h"

static inline uint32_t stream_read32(bos_hda_controller_t* ctrl, uint32_t reg) {
    return *(volatile uint32_t*)(ctrl->mmio_virt + ctrl->stream_reg_offset + reg);
}

static inline void stream_write8(bos_hda_controller_t* ctrl, uint32_t reg, uint8_t val) {
    *(volatile uint8_t*)(ctrl->mmio_virt + ctrl->stream_reg_offset + reg) = val;
}

static inline void stream_write16(bos_hda_controller_t* ctrl, uint32_t reg, uint16_t val) {
    *(volatile uint16_t*)(ctrl->mmio_virt + ctrl->stream_reg_offset + reg) = val;
}

static inline void stream_write32(bos_hda_controller_t* ctrl, uint32_t reg, uint32_t val) {
    *(volatile uint32_t*)(ctrl->mmio_virt + ctrl->stream_reg_offset + reg) = val;
}

/*
 * Generate 440 Hz Reference Diagnostic PCM Test Signal (Test D)
 */
static void hda_generate_test_tone(int16_t* buffer, uint32_t num_samples) {
    /* 48000 Hz / 440 Hz ≈ 109 samples per period */
    for (uint32_t i = 0; i < num_samples; i += 2) {
        int phase = (i / 2) % 109;
        int16_t sample = (phase < 54) ? 8000 : -8000;
        buffer[i]     = sample; /* Left */
        buffer[i + 1] = sample; /* Right */
    }
}

/*
 * Initialize Output DMA Stream Engine
 */
bool bos_hda_stream_init(bos_hda_controller_t* ctrl) {
    if (!ctrl || !ctrl->initialized) return false;

    /* Output Stream #0 starts after Input Streams (num_iss) */
    ctrl->stream_reg_offset = HDA_SD_BASE + ((uint32_t)ctrl->num_iss * HDA_SD_STRIDE);
    ctrl->stream_tag = BOS_HDA_DEFAULT_STREAM_ID;

    /* Step 1: Reset Stream Descriptor */
    stream_write32(ctrl, HDA_SD_CTL, HDA_SD_CTL_SRST);
    for (volatile int timeout = 0; timeout < 50000; timeout++) {
        if (stream_read32(ctrl, HDA_SD_CTL) & HDA_SD_CTL_SRST) break;
    }

    stream_write32(ctrl, HDA_SD_CTL, 0);
    for (volatile int timeout = 0; timeout < 50000; timeout++) {
        if (!(stream_read32(ctrl, HDA_SD_CTL) & HDA_SD_CTL_SRST)) break;
    }

    /* Step 2: Program Stream Tag (Tag 1 in bits 20..23) */
    stream_write8(ctrl, HDA_SD_CTL_B2, (ctrl->stream_tag << 4));

    /* Step 3: Set Stream Format: 48kHz, 16-bit, 2-Channel Stereo (0x0011) */
    stream_write16(ctrl, HDA_SD_FMT, HDA_FORMAT_48K_16BIT_STEREO);

    /* Step 4: Allocate Physical DMA Buffer (64 KB = 16 pages) */
    void* dma_pages = pmm_alloc_pages(16);
    if (!dma_pages) {
        display_print("[BOS-AUDIO] [ERROR] Failed to allocate 64KB DMA buffer\n");
        return false;
    }
    uint64_t dma_phys = (uint64_t)dma_pages;
    void* pml4 = vmm_get_active_pml4();
    void* k_pml4 = vmm_get_kernel_pml4();

    for (size_t p = 0; p < 16; p++) {
        uint64_t page_addr = dma_phys + (p * 4096);
        if (page_addr >= 0x40000000ULL) {
            vmm_map_page(pml4, page_addr, page_addr, PAGE_PRESENT | PAGE_WRITABLE | PAGE_CACHE_DISABLE);
            if (k_pml4 && k_pml4 != pml4) {
                vmm_map_page(k_pml4, page_addr, page_addr, PAGE_PRESENT | PAGE_WRITABLE | PAGE_CACHE_DISABLE);
            }
        }
    }

    ctrl->dma_buffer_virt = dma_pages;
    ctrl->dma_buffer_phys = dma_phys;
    ctrl->dma_buffer_size = BOS_HDA_DMA_BUFFER_SIZE;
    ctrl->write_pos = 0;

    /* Pre-fill buffer with 440 Hz reference diagnostic tone */
    hda_generate_test_tone((int16_t*)ctrl->dma_buffer_virt, ctrl->dma_buffer_size / sizeof(int16_t));

    /* Step 5: Allocate Buffer Descriptor List (BDL) Page */
    void* bdl_page = pmm_alloc_pages(1);
    if (!bdl_page) {
        display_print("[BOS-AUDIO] [ERROR] Failed to allocate BDL page\n");
        return false;
    }
    uint64_t bdl_phys = (uint64_t)bdl_page;
    if (bdl_phys >= 0x40000000ULL) {
        vmm_map_page(pml4, bdl_phys, bdl_phys, PAGE_PRESENT | PAGE_WRITABLE | PAGE_CACHE_DISABLE);
        if (k_pml4 && k_pml4 != pml4) {
            vmm_map_page(k_pml4, bdl_phys, bdl_phys, PAGE_PRESENT | PAGE_WRITABLE | PAGE_CACHE_DISABLE);
        }
    }
    memset(bdl_page, 0, 4096);

    ctrl->bdl_virt = (hda_bdl_entry_t*)bdl_page;
    ctrl->bdl_phys = bdl_phys;
    ctrl->bdl_entries = BOS_HDA_BDL_NUM_ENTRIES;

    /* Configure 4 x 16 KB BDL descriptors */
    uint32_t chunk_size = ctrl->dma_buffer_size / ctrl->bdl_entries; /* 16384 bytes */
    for (uint32_t e = 0; e < ctrl->bdl_entries; e++) {
        uint64_t entry_phys = ctrl->dma_buffer_phys + (e * chunk_size);
        ctrl->bdl_virt[e].addr_low  = (uint32_t)(entry_phys & 0xFFFFFFFFU);
        ctrl->bdl_virt[e].addr_high = (uint32_t)(entry_phys >> 32);
        ctrl->bdl_virt[e].length    = chunk_size;
        ctrl->bdl_virt[e].flags     = HDA_BDL_IOC; /* Interrupt On Completion */
    }

    /* Program BDL Base Address */
    stream_write32(ctrl, HDA_SD_BDLPL, (uint32_t)(ctrl->bdl_phys & 0xFFFFFFFFU));
    stream_write32(ctrl, HDA_SD_BDLPU, (uint32_t)(ctrl->bdl_phys >> 32));

    /* Cyclic Buffer Length (CBL): Total bytes in ring */
    stream_write32(ctrl, HDA_SD_CBL, ctrl->dma_buffer_size);

    /* Last Valid Index (LVI): entries - 1 (3) */
    stream_write16(ctrl, HDA_SD_LVI, (uint16_t)(ctrl->bdl_entries - 1));

    display_print("[BOS-AUDIO] DMA stream initialized\n");
    return true;
}

#include "kernel/audio/mixer/audio_mixer.h"

/*
 * Start Audio DMA Streaming
 */
bool bos_hda_stream_start(uint32_t sample_rate, uint8_t channels, uint8_t bit_depth) {
    bos_hda_controller_t* ctrl = bos_hda_get_controller();
    if (!ctrl || !ctrl->initialized) return false;

    (void)sample_rate;
    (void)channels;
    (void)bit_depth;

    /* Clear Status bits */
    stream_write8(ctrl, HDA_SD_STS, 0x1C);

    /* Pre-fill initial DMA buffer with fresh audio from mixer */
    static const AudioPcmFormat g_hda_format = {
        .format = PCM_FORMAT_S16_LE,
        .sample_rate = 48000,
        .channels = 2,
        .bit_depth = 16,
        .is_signed = true
    };
    audio_mixer_process((uint8_t*)ctrl->dma_buffer_virt, ctrl->dma_buffer_size, &g_hda_format);
    ctrl->write_pos = 0;

    /* Run Stream DMA with Interrupts */
    uint32_t ctl = stream_read32(ctrl, HDA_SD_CTL);
    ctl |= HDA_SD_CTL_RUN | HDA_SD_CTL_IOCE;
    stream_write32(ctrl, HDA_SD_CTL, ctl);

    ctrl->stream_active = true;
    ctrl->stream_paused = false;
    return true;
}

/*
 * Stop Audio DMA Streaming
 */
void bos_hda_stream_stop(void) {
    bos_hda_controller_t* ctrl = bos_hda_get_controller();
    if (!ctrl || !ctrl->initialized) return;

    uint32_t ctl = stream_read32(ctrl, HDA_SD_CTL);
    ctl &= ~HDA_SD_CTL_RUN;
    stream_write32(ctrl, HDA_SD_CTL, ctl);

    ctrl->stream_active = false;
    ctrl->stream_paused = false;
}

/*
 * Pause Audio DMA Streaming
 */
void bos_hda_stream_pause(void) {
    bos_hda_controller_t* ctrl = bos_hda_get_controller();
    if (!ctrl || !ctrl->initialized) return;

    uint32_t ctl = stream_read32(ctrl, HDA_SD_CTL);
    ctl &= ~HDA_SD_CTL_RUN;
    stream_write32(ctrl, HDA_SD_CTL, ctl);

    ctrl->stream_paused = true;
}

/*
 * Resume Audio DMA Streaming
 */
void bos_hda_stream_resume(void) {
    bos_hda_controller_t* ctrl = bos_hda_get_controller();
    if (!ctrl || !ctrl->initialized) return;

    uint32_t ctl = stream_read32(ctrl, HDA_SD_CTL);
    ctl |= HDA_SD_CTL_RUN;
    stream_write32(ctrl, HDA_SD_CTL, ctl);

    ctrl->stream_paused = false;
}

/*
 * Expose DMA Buffer for BOS Audio Mixer Engine
 */
bool bos_hda_stream_allocate_dma(size_t requested_size, void** virt_addr, uint32_t* phys_addr) {
    bos_hda_controller_t* ctrl = bos_hda_get_controller();
    if (!ctrl || !ctrl->dma_buffer_virt) return false;

    (void)requested_size;
    if (virt_addr) *virt_addr = ctrl->dma_buffer_virt;
    if (phys_addr) *phys_addr = (uint32_t)(ctrl->dma_buffer_phys & 0xFFFFFFFFU);
    return true;
}

/*
 * Update Write Position and Refill DMA Buffer
 */
void bos_hda_stream_update_pointers(uint32_t frames_written) {
    bos_hda_controller_t* ctrl = bos_hda_get_controller();
    if (!ctrl || !ctrl->stream_active || ctrl->stream_paused || !ctrl->dma_buffer_virt) return;

    (void)frames_written;

    uint32_t lpib = stream_read32(ctrl, HDA_SD_LPIB);
    uint32_t chunk_size = ctrl->dma_buffer_size / ctrl->bdl_entries; /* 16384 bytes */
    if (chunk_size == 0) return;

    uint32_t hw_chunk = (lpib / chunk_size) % ctrl->bdl_entries;
    uint32_t sw_chunk = (ctrl->write_pos / chunk_size) % ctrl->bdl_entries;

    static const AudioPcmFormat g_hda_format = {
        .format = PCM_FORMAT_S16_LE,
        .sample_rate = 48000,
        .channels = 2,
        .bit_depth = 16,
        .is_signed = true
    };

    /* Refill any chunk that hardware has finished playing */
    while (sw_chunk != hw_chunk) {
        uint8_t* target = (uint8_t*)ctrl->dma_buffer_virt + (sw_chunk * chunk_size);
        audio_mixer_process(target, chunk_size, &g_hda_format);
        sw_chunk = (sw_chunk + 1) % ctrl->bdl_entries;
        ctrl->write_pos = sw_chunk * chunk_size;
    }
}

/*
 * Read Hardware DMA Position (LPIB)
 */
uint32_t bos_hda_stream_get_position(void) {
    bos_hda_controller_t* ctrl = bos_hda_get_controller();
    if (!ctrl || !ctrl->initialized) return 0;

    return stream_read32(ctrl, HDA_SD_LPIB);
}

/*
 * Hardware Volume Setting
 */
void bos_hda_stream_set_volume(uint8_t left, uint8_t right) {
    bos_hda_controller_t* ctrl = bos_hda_get_controller();
    if (!ctrl || !ctrl->initialized) return;

    bos_hda_codec_set_volume(ctrl, left, right);
}

/*
 * HAL Interface Binding
 */
static bool hda_hal_init(void* device_info) {
    if (!device_info) {
        /* Auto-discover HDA via PCI subsystem */
        PCIDevice dev;
        if (pci_find_by_class(0x04, 0x03, &dev)) {
            return bos_hda_controller_init(&dev);
        }
        return false;
    }

    uint32_t* pci_info = (uint32_t*)device_info;
    uint8_t bus  = (uint8_t)pci_info[0];
    uint8_t slot = (uint8_t)pci_info[1];

    /* Retrieve device from PCI registry */
    uint32_t dev_count = pci_get_device_count();
    for (uint32_t i = 0; i < dev_count; i++) {
        PCIDevice* d = pci_get_device(i);
        if (d && d->bus == bus && d->slot == slot && d->base_class == 0x04 && d->sub_class == 0x03) {
            return bos_hda_controller_init(d);
        }
    }

    /* Direct discovery fallback */
    PCIDevice dev;
    if (pci_find_by_class(0x04, 0x03, &dev)) {
        return bos_hda_controller_init(&dev);
    }

    return false;
}

static void hda_hal_shutdown(void) {
    bos_hda_controller_shutdown();
}

static audio_hal_driver_t hda_driver = {
    .name = "Intel High Definition Audio (HDA)",
    .capabilities = 0,
    .init = hda_hal_init,
    .shutdown = hda_hal_shutdown,
    .start_stream = bos_hda_stream_start,
    .stop_stream = bos_hda_stream_stop,
    .pause_stream = bos_hda_stream_pause,
    .resume_stream = bos_hda_stream_resume,
    .allocate_dma_buffer = bos_hda_stream_allocate_dma,
    .update_pointers = bos_hda_stream_update_pointers,
    .get_hardware_position = bos_hda_stream_get_position,
    .set_hardware_volume = bos_hda_stream_set_volume,
    .register_irq_callback = NULL
};

void hda_driver_register(void) {
    audio_hal_register_driver(&hda_driver);
}
