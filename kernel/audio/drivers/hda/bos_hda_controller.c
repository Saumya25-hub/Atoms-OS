/*
 * ATOMS OS — Intel High Definition Audio (HDA) Driver
 * Controller Hardware Initialization, MMIO & CORB/RIRB Engine
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

static bos_hda_controller_t g_hda_controller;

bos_hda_controller_t* bos_hda_get_controller(void) {
    return &g_hda_controller;
}

static inline uint8_t hda_read8(bos_hda_controller_t* ctrl, uint32_t reg) {
    return *(volatile uint8_t*)(ctrl->mmio_virt + reg);
}

static inline uint16_t hda_read16(bos_hda_controller_t* ctrl, uint32_t reg) {
    return *(volatile uint16_t*)(ctrl->mmio_virt + reg);
}

static inline uint32_t hda_read32(bos_hda_controller_t* ctrl, uint32_t reg) {
    return *(volatile uint32_t*)(ctrl->mmio_virt + reg);
}

static inline void hda_write8(bos_hda_controller_t* ctrl, uint32_t reg, uint8_t val) {
    *(volatile uint8_t*)(ctrl->mmio_virt + reg) = val;
}

static inline void hda_write16(bos_hda_controller_t* ctrl, uint32_t reg, uint16_t val) {
    *(volatile uint16_t*)(ctrl->mmio_virt + reg) = val;
}

static inline void hda_write32(bos_hda_controller_t* ctrl, uint32_t reg, uint32_t val) {
    *(volatile uint32_t*)(ctrl->mmio_virt + reg) = val;
}

static void hda_delay_us(uint32_t count) {
    for (volatile uint32_t i = 0; i < count * 25; i++) {
        __asm__ __volatile__("pause");
    }
}

/*
 * Immediate Command Interface
 * Primary fast command submission protocol with CORB/RIRB fallback.
 */
static bool hda_send_verb_immediate(bos_hda_controller_t* ctrl, uint32_t verb, uint32_t* response) {
    /* Check if Immediate Command Engine is busy */
    uint32_t timeout = 50000;
    while ((hda_read16(ctrl, HDA_ICS) & HDA_ICS_BUSY) && --timeout) {
        hda_delay_us(2);
    }
    if (timeout == 0) {
        return false;
    }

    /* Write verb to Immediate Command Register */
    hda_write32(ctrl, HDA_IC, verb);

    /* Set Busy and Clear Result Valid */
    hda_write16(ctrl, HDA_ICS, HDA_ICS_BUSY | HDA_ICS_IRV);

    /* Wait for response */
    timeout = 50000;
    while (((hda_read16(ctrl, HDA_ICS) & HDA_ICS_BUSY) || !(hda_read16(ctrl, HDA_ICS) & HDA_ICS_IRV)) && --timeout) {
        hda_delay_us(2);
    }
    if (timeout == 0) {
        return false;
    }

    if (response) {
        *response = hda_read32(ctrl, HDA_IR);
    }
    return true;
}

/*
 * CORB / RIRB Command Ring Submission
 */
static bool hda_send_verb_corb_rirb(bos_hda_controller_t* ctrl, uint32_t verb, uint32_t* response) {
    if (!ctrl->corb_virt || !ctrl->rirb_virt) {
        return false;
    }

    uint16_t wp = (ctrl->corb_wp + 1) % ctrl->corb_entries;
    ctrl->corb_virt[wp] = verb;
    ctrl->corb_wp = wp;
    hda_write16(ctrl, HDA_CORBWP, wp);

    /* Poll RIRB for completion */
    uint32_t timeout = 50000;
    while (--timeout) {
        uint16_t rirb_wp = hda_read16(ctrl, HDA_RIRBWP) & 0xFF;
        if (rirb_wp != ctrl->rirb_rp) {
            ctrl->rirb_rp = (ctrl->rirb_rp + 1) % ctrl->rirb_entries;
            uint64_t resp_entry = ctrl->rirb_virt[ctrl->rirb_rp];
            if (response) {
                *response = (uint32_t)(resp_entry & 0xFFFFFFFFULL);
            }
            return true;
        }
        hda_delay_us(2);
    }

    return false;
}

/*
 * Unified Verb Submission Function
 */
uint32_t bos_hda_send_verb(uint8_t cad, uint8_t node, uint32_t verb, uint32_t param) {
    bos_hda_controller_t* ctrl = &g_hda_controller;
    if (!ctrl->initialized) {
        return 0;
    }

    uint32_t full_verb = 0;
    if (verb <= 0x0F) {
        /* 4-bit verb with 8-bit parameter */
        full_verb = HDA_VERB_4BIT(cad, node, verb, param);
    } else {
        /* 12-bit verb with 8-bit parameter */
        full_verb = HDA_VERB_12BIT(cad, node, verb, param);
    }

    uint32_t resp = 0;
    if (hda_send_verb_immediate(ctrl, full_verb, &resp)) {
        return resp;
    }

    if (hda_send_verb_corb_rirb(ctrl, full_verb, &resp)) {
        return resp;
    }

    return 0;
}

/*
 * Controller Hardware Initialization
 */
bool bos_hda_controller_init(PCIDevice* dev) {
    if (!dev) return false;

    /* Validate Class 0x04 (Multimedia), Subclass 0x03 (High Definition Audio) */
    if (dev->base_class != 0x04 || dev->sub_class != 0x03) {
        return false;
    }

    display_print("[BOS-AUDIO] Intel HDA controller detected\n");
    display_print("[BOS-AUDIO] Vendor: ");
    display_print_hex(dev->vendor_id);
    display_print(" Device: ");
    display_print_hex(dev->device_id);
    display_print("\n");

    /* Verify BAR0 is 64-bit or 32-bit MMIO */
    if (dev->bars[0].type == PCI_BAR_TYPE_IO || dev->bars[0].base_address == 0) {
        display_print("[BOS-AUDIO] [ERROR] BAR0 is not MMIO mapped!\n");
        return false;
    }

    bos_hda_controller_t* ctrl = &g_hda_controller;
    memset(ctrl, 0, sizeof(bos_hda_controller_t));
    ctrl->pci_dev = *dev;
    ctrl->mmio_phys = dev->bars[0].base_address;
    ctrl->mmio_size = (dev->bars[0].size > 0 && dev->bars[0].size <= 0x10000) ? (uint32_t)dev->bars[0].size : 0x4000;

    display_print("[BOS-AUDIO] BAR0: ");
    display_print_hex(ctrl->mmio_phys);
    display_print(" BAR type: MMIO\n");

    /* Enable PCI Bus Mastering & Memory Space */
    pci_enable_bus_mastering(dev);
    pci_enable_memory_space(dev);

    /* Map MMIO pages into kernel address space (Uncached) */
    void* pml4 = vmm_get_active_pml4();
    void* k_pml4 = vmm_get_kernel_pml4();
    for (uint64_t off = 0; off < ctrl->mmio_size; off += 4096) {
        uint64_t phys_page = (ctrl->mmio_phys + off) & ~0xFFFULL;
        vmm_map_page(pml4, phys_page, phys_page, PAGE_PRESENT | PAGE_WRITABLE | PAGE_CACHE_DISABLE);
        if (k_pml4 && k_pml4 != pml4) {
            vmm_map_page(k_pml4, phys_page, phys_page, PAGE_PRESENT | PAGE_WRITABLE | PAGE_CACHE_DISABLE);
        }
    }
    ctrl->mmio_virt = ctrl->mmio_phys;

    /* Step 1: Controller Reset Sequence (CRST) */
    uint32_t gctl = hda_read32(ctrl, HDA_GCTL);
    if (gctl & HDA_GCTL_CRST) {
        hda_write32(ctrl, HDA_GCTL, 0);
        uint32_t timeout = 50000;
        while ((hda_read32(ctrl, HDA_GCTL) & HDA_GCTL_CRST) && --timeout) {
            hda_delay_us(2);
        }
    }
    hda_delay_us(100);

    /* Assert CRST = 1 to enter operating state */
    hda_write32(ctrl, HDA_GCTL, HDA_GCTL_CRST);
    uint32_t reset_timeout = 50000;
    while (!(hda_read32(ctrl, HDA_GCTL) & HDA_GCTL_CRST) && --reset_timeout) {
        hda_delay_us(2);
    }
    if (reset_timeout == 0) {
        display_print("[BOS-AUDIO] [ERROR] Controller CRST reset timeout!\n");
        return false;
    }

    /* Wait 1 ms for codecs to initialize analog circuitry and power up */
    hda_delay_us(1500);

    display_print("[BOS-AUDIO] Controller reset: OK\n");

    /* Read Capabilities (GCAP) */
    uint16_t gcap = hda_read16(ctrl, HDA_GCAP);
    ctrl->num_iss = (gcap >> 8) & 0x0F;
    ctrl->num_oss = (gcap >> 12) & 0x0F;
    ctrl->num_bss = (gcap >> 3) & 0x1F;

    /* Setup CORB and RIRB Command Ring Buffers */
    void* ring_page = pmm_alloc_pages(1);
    if (!ring_page) {
        display_print("[BOS-AUDIO] [ERROR] Failed to allocate CORB/RIRB page\n");
        return false;
    }
    uint64_t ring_phys = (uint64_t)ring_page;
    if (ring_phys >= 0x40000000ULL) {
        vmm_map_page(pml4, ring_phys, ring_phys, PAGE_PRESENT | PAGE_WRITABLE | PAGE_CACHE_DISABLE);
        if (k_pml4 && k_pml4 != pml4) {
            vmm_map_page(k_pml4, ring_phys, ring_phys, PAGE_PRESENT | PAGE_WRITABLE | PAGE_CACHE_DISABLE);
        }
    }
    memset(ring_page, 0, 4096);

    /* CORB: 256 entries * 4 bytes = 1024 bytes at offset 0 */
    ctrl->corb_virt = (uint32_t*)ring_page;
    ctrl->corb_phys = ring_phys;
    ctrl->corb_entries = 256;
    ctrl->corb_wp = 0;

    /* Stop CORB */
    hda_write8(ctrl, HDA_CORBCTL, 0);
    hda_delay_us(10);
    /* Program CORB Base Address */
    hda_write32(ctrl, HDA_CORBLBASE, (uint32_t)ctrl->corb_phys);
    hda_write32(ctrl, HDA_CORBUBASE, (uint32_t)(ctrl->corb_phys >> 32));
    /* Reset CORB Read Pointer */
    hda_write16(ctrl, HDA_CORBRP, HDA_CORBRP_RST);
    hda_delay_us(10);
    hda_write16(ctrl, HDA_CORBRP, 0);
    /* Set CORB Write Pointer to 0 */
    hda_write16(ctrl, HDA_CORBWP, 0);
    /* Set CORB size to 256 entries (0x02) */
    hda_write8(ctrl, HDA_CORBSIZE, 0x02);
    /* Start CORB DMA Engine */
    hda_write8(ctrl, HDA_CORBCTL, HDA_CORBCTL_RUN);

    /* RIRB: 256 entries * 8 bytes = 2048 bytes at offset 1024 */
    ctrl->rirb_virt = (uint64_t*)((uint8_t*)ring_page + 1024);
    ctrl->rirb_phys = ring_phys + 1024;
    ctrl->rirb_entries = 256;
    ctrl->rirb_rp = 0;

    /* Stop RIRB */
    hda_write8(ctrl, HDA_RIRBCTL, 0);
    hda_delay_us(10);
    /* Program RIRB Base Address */
    hda_write32(ctrl, HDA_RIRBLBASE, (uint32_t)ctrl->rirb_phys);
    hda_write32(ctrl, HDA_RIRBUBASE, (uint32_t)(ctrl->rirb_phys >> 32));
    /* Reset RIRB Write Pointer */
    hda_write16(ctrl, HDA_RIRBWP, 0x8000);
    /* Set Response Interrupt Count to 1 */
    hda_write16(ctrl, HDA_RINTCNT, 1);
    /* Set RIRB size to 256 entries (0x02) */
    hda_write8(ctrl, HDA_RIRBSIZE, 0x02);
    /* Start RIRB DMA Engine */
    hda_write8(ctrl, HDA_RIRBCTL, HDA_RIRBCTL_RUN);

    ctrl->initialized = true;

    /* Step 2: Codec Scan & Configuration */
    uint16_t statests = hda_read16(ctrl, HDA_STATESTS);
    display_print("[BOS-AUDIO] Codec scan\n");

    if (statests == 0) {
        /* Retry codec discovery wait */
        hda_delay_us(5000);
        statests = hda_read16(ctrl, HDA_STATESTS);
    }

    if (statests == 0) {
        display_print("[BOS-AUDIO] [WARN] No codecs reported in STATESTS, probing Codec 0...\n");
    }

    if (!bos_hda_codec_discover_and_configure(ctrl, &ctrl->active_codec)) {
        display_print("[BOS-AUDIO] [ERROR] Codec discovery failed!\n");
        return false;
    }
    ctrl->codec_ready = true;

    /* Step 3: Stream Engine Initialization */
    if (!bos_hda_stream_init(ctrl)) {
        display_print("[BOS-AUDIO] [ERROR] DMA Stream engine init failed!\n");
        return false;
    }

    display_print("[BOS-AUDIO] Audio output READY\n");
    return true;
}

void bos_hda_controller_shutdown(void) {
    bos_hda_controller_t* ctrl = &g_hda_controller;
    if (!ctrl->initialized) return;

    bos_hda_stream_stop();

    /* Disable CORB and RIRB */
    hda_write8(ctrl, HDA_CORBCTL, 0);
    hda_write8(ctrl, HDA_RIRBCTL, 0);

    /* Reset Controller */
    hda_write32(ctrl, HDA_GCTL, 0);

    ctrl->initialized = false;
    display_print("[BOS-AUDIO] Subsystem shutdown completed\n");
}
