#include "ahci.h"
#include "kernel/core/pci/pci.h"
#include "kernel/core/memory/pmm/include/pmm.h"
#include "kernel/core/memory/vmm/include/vmm.h"
#include "kernel/core/memory/vmm/include/paging.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/drivers/display/display.h"
#include "kernel/vfs/vfs_legacy/storage/include/block_device.h"

extern void com1_puts(const char *s);

/* Diagnostic string and number print helpers */
static void ahci_dbg_hex(uint64_t val) {
    display_print_hex(val);
}

static void ahci_dbg_dec(uint64_t val) {
    display_print_dec(val);
}

/* Port Context structure */
typedef struct {
    bool                active;
    uint8_t             port_num;
    uint64_t            port_base;
    uint64_t            frame_phys;     /* Physical address of 4KB control structure frame */
    AHCICommandHeader*  cmd_header;     /* Offset 0x000 (1024 bytes, 1KB aligned) */
    AHCIFISReceived*    rx_fis;         /* Offset 0x400 (256 bytes, 256B aligned) */
    AHCICommandTable*   cmd_table;      /* Offset 0x500 (512 bytes, 128B aligned) */
    uint64_t            bounce_phys;    /* Physical address of 4KB DMA bounce frame */
    uint8_t*            bounce_virt;    /* Virtual pointer to DMA bounce frame */
} AHCIPortContext;

static AHCIPortContext s_ports[MAX_AHCI_PORTS];
static AHCIDriveData   s_ahci_drives[MAX_AHCI_DRIVES];
static BlockDevice     s_ahci_bdevs[MAX_AHCI_DRIVES];
static char            s_ahci_names[MAX_AHCI_DRIVES][16];
static int             s_ahci_drive_count = 0;
static uint64_t        s_abar_virt = 0;
static uint64_t        s_abar_phys = 0;
static AHCIControllerTelemetry s_telemetry;

static inline uint32_t ahci_read32(uint64_t reg_addr) {
    return *(volatile uint32_t*)reg_addr;
}

static inline void ahci_write32(uint64_t reg_addr, uint32_t val) {
    *(volatile uint32_t*)reg_addr = val;
}

/* Stops port DMA engines in accordance with AHCI 1.3 spec section 10.3.1 */
static bool ahci_stop_port(uint64_t port_base) {
    uint32_t cmd = ahci_read32(port_base + AHCI_PORT_CMD);
    if (cmd & AHCI_PxCMD_ST) {
        cmd &= ~AHCI_PxCMD_ST;
        ahci_write32(port_base + AHCI_PORT_CMD, cmd);
        int timeout = 500000;
        while (timeout-- > 0) {
            if (!(ahci_read32(port_base + AHCI_PORT_CMD) & AHCI_PxCMD_CR)) break;
            __asm__ volatile("pause");
        }
    }

    cmd = ahci_read32(port_base + AHCI_PORT_CMD);
    if (cmd & AHCI_PxCMD_FRE) {
        cmd &= ~AHCI_PxCMD_FRE;
        ahci_write32(port_base + AHCI_PORT_CMD, cmd);
        int timeout = 500000;
        while (timeout-- > 0) {
            if (!(ahci_read32(port_base + AHCI_PORT_CMD) & AHCI_PxCMD_FR)) break;
            __asm__ volatile("pause");
        }
    }
    return true;
}

/* Starts port DMA engines */
static void ahci_start_port(uint64_t port_base) {
    int timeout = 500000;
    while (timeout-- > 0) {
        if (!(ahci_read32(port_base + AHCI_PORT_CMD) & AHCI_PxCMD_CR)) break;
        __asm__ volatile("pause");
    }

    uint32_t cmd = ahci_read32(port_base + AHCI_PORT_CMD);
    cmd |= AHCI_PxCMD_FRE;
    ahci_write32(port_base + AHCI_PORT_CMD, cmd);
    cmd |= AHCI_PxCMD_ST;
    ahci_write32(port_base + AHCI_PORT_CMD, cmd);
}

/* Executes an ATA command on Slot 0 with bounded timeout */
static bool ahci_exec_cmd_slot0(AHCIPortContext* port_ctx, bool is_write, uint32_t byte_count) {
    uint64_t port_base = port_ctx->port_base;

    /* Wait for port to clear BSY and DRQ */
    int timeout = 200000;
    while (timeout-- > 0) {
        uint32_t tfd = ahci_read32(port_base + AHCI_PORT_TFD);
        if (!(tfd & (AHCI_PxTFD_BSY | AHCI_PxTFD_DRQ))) break;
        __asm__ volatile("pause");
    }
    if (timeout <= 0) {
        display_print("[AHCI] Port busy timeout before command!\n");
        return false;
    }

    /* Clear diagnostic status and interrupts */
    ahci_write32(port_base + AHCI_PORT_SERR, 0xFFFFFFFF);
    ahci_write32(port_base + AHCI_PORT_IS, 0xFFFFFFFF);

    /* Setup Command Header 0 */
    AHCICommandHeader* hdr = &port_ctx->cmd_header[0];
    hdr->cfl = sizeof(AHCIFIS_RegH2D) / 4; /* 5 DWORDS */
    hdr->a = 0;
    hdr->w = is_write ? 1 : 0;
    hdr->p = 0;
    hdr->r = 0;
    hdr->b = 0;
    hdr->c = 0;
    hdr->pmp = 0;
    hdr->prdtl = 1;
    hdr->prdbc = 0;

    /* Setup PRD Table Entry 0 pointing to bounce buffer */
    AHCIPRDTEntry* prdt = &port_ctx->cmd_table->prdt_entries[0];
    prdt->dba = (uint32_t)(port_ctx->bounce_phys & 0xFFFFFFFF);
    prdt->dbau = (uint32_t)(port_ctx->bounce_phys >> 32);
    prdt->rsvd0 = 0;
    prdt->dbc = ((byte_count - 1) & 0x3FFFFF) | (1U << 31); /* Interrupt on completion */
    prdt->rsvd1 = 0;
    prdt->i = 1;

    /* Memory barrier before issue */
    __asm__ volatile("" ::: "memory");

    /* Issue command on slot 0 */
    ahci_write32(port_base + AHCI_PORT_CI, (1U << 0));

    /* Bounded completion wait */
    timeout = 3000000;
    while (timeout-- > 0) {
        uint32_t ci = ahci_read32(port_base + AHCI_PORT_CI);
        if (!(ci & (1U << 0))) {
            break; /* Command completed by hardware */
        }
        uint32_t tfd = ahci_read32(port_base + AHCI_PORT_TFD);
        if (tfd & AHCI_PxTFD_ERR) {
            display_print("[AHCI] Command error! TFD=0x");
            ahci_dbg_hex(tfd);
            display_print("\n");
            return false;
        }
        __asm__ volatile("pause");
    }

    if (timeout <= 0) {
        display_print("[AHCI] Command timed out! CI=0x");
        ahci_dbg_hex(ahci_read32(port_base + AHCI_PORT_CI));
        display_print("\n");
        return false;
    }

    return true;
}

/* Issues ATA IDENTIFY DEVICE (0xEC) to retrieve drive parameters */
static bool ahci_identify_device(AHCIPortContext* port_ctx, uint16_t* out_ident_data) {
    memset(port_ctx->bounce_virt, 0, 512);
    memset(port_ctx->cmd_table->cfis, 0, 64);

    AHCIFIS_RegH2D* fis = (AHCIFIS_RegH2D*)port_ctx->cmd_table->cfis;
    fis->fis_type = FIS_TYPE_REG_H2D;
    fis->pm = 0;
    fis->c = 1; /* Command Register */
    fis->command = ATA_CMD_IDENTIFY;
    fis->device = 0;

    if (!ahci_exec_cmd_slot0(port_ctx, false, 512)) {
        return false;
    }

    memcpy(out_ident_data, port_ctx->bounce_virt, 512);
    return true;
}

/* Reads a chunk of sectors (up to 8 sectors / 4096 bytes) using READ DMA EXT */
static bool ahci_read_dma_chunk(AHCIPortContext* port_ctx, uint64_t lba, uint16_t count, void* dest) {
    if (count == 0 || count > 8) return false;

    memset(port_ctx->cmd_table->cfis, 0, 64);
    AHCIFIS_RegH2D* fis = (AHCIFIS_RegH2D*)port_ctx->cmd_table->cfis;
    fis->fis_type = FIS_TYPE_REG_H2D;
    fis->pm = 0;
    fis->c = 1;
    fis->command = ATA_CMD_READ_DMA_EXT;
    fis->device = (1U << 6); /* LBA mode */

    fis->lba0 = (uint8_t)(lba & 0xFF);
    fis->lba1 = (uint8_t)((lba >> 8) & 0xFF);
    fis->lba2 = (uint8_t)((lba >> 16) & 0xFF);
    fis->lba3 = (uint8_t)((lba >> 24) & 0xFF);
    fis->lba4 = (uint8_t)((lba >> 32) & 0xFF);
    fis->lba5 = (uint8_t)((lba >> 40) & 0xFF);

    fis->count_low = (uint8_t)(count & 0xFF);
    fis->count_high = (uint8_t)((count >> 8) & 0xFF);

    if (!ahci_exec_cmd_slot0(port_ctx, false, count * 512)) {
        return false;
    }

    memcpy(dest, port_ctx->bounce_virt, count * 512);
    return true;
}

/* Public bounded sector read API */
bool ahci_read_sectors(uint8_t port, uint64_t lba, uint32_t count, void* buffer) {
    if (port >= MAX_AHCI_PORTS || !s_ports[port].active || !buffer || count == 0) return false;
    AHCIPortContext* pctx = &s_ports[port];

    uint8_t* dest = (uint8_t*)buffer;
    uint64_t cur_lba = lba;
    uint32_t remaining = count;

    while (remaining > 0) {
        uint16_t chunk = (remaining > 8) ? 8 : (uint16_t)remaining;
        if (!ahci_read_dma_chunk(pctx, cur_lba, chunk, dest)) {
            return false;
        }
        dest += (chunk * 512);
        cur_lba += chunk;
        remaining -= chunk;
    }

    return true;
}

/* BlockDevice read wrapper */
static bool ahci_block_device_read(BlockDevice* dev, uint64_t lba, uint32_t count, void* buffer) {
    if (!dev || !dev->driver_data || !buffer) return false;
    AHCIDriveData* ddata = (AHCIDriveData*)dev->driver_data;
    if (lba + count > ddata->sector_count) {
        display_print("[AHCI] Out of bounds read requested on ");
        display_print(dev->name ? dev->name : "disk");
        display_print("\n");
        return false;
    }
    return ahci_read_sectors(ddata->port_num, lba, count, buffer);
}

/* Configures and initializes an individual AHCI port */
static bool ahci_init_port(uint8_t port_num, uint64_t port_base) {
    AHCIPortContext* pctx = &s_ports[port_num];
    pctx->port_num = port_num;
    pctx->port_base = port_base;
    pctx->active = false;

    AHCIPortTelemetry* pt = &s_telemetry.ports[port_num];
    pt->port_num = port_num;
    pt->implemented = true;
    pt->bdev_id = -1;

    /* Check device presence and link status */
    uint32_t ssts = ahci_read32(port_base + AHCI_PORT_SSTS);
    pt->ssts = ssts;
    pt->det = (uint8_t)(ssts & AHCI_PxSSTS_DET_MASK);
    pt->spd = (uint8_t)((ssts >> 4) & 0x0F);
    pt->ipm = (uint8_t)((ssts >> 8) & 0x0F);
    pt->sig = ahci_read32(port_base + AHCI_PORT_SIG);

    if (pt->det != AHCI_PxSSTS_DET_PRESENT) {
        pt->state = AHCI_PORT_STATE_NO_DEVICE;
        return false; /* No drive connected */
    }

    pt->state = AHCI_PORT_STATE_PHY_ONLINE;

    display_print("[AHCI] Port "); ahci_dbg_dec(port_num);
    display_print(": SATA Device Link UP (SSTS=0x");
    ahci_dbg_hex(ssts); display_print(")\n");

    /* Stop port engines before configuring memory pointers */
    ahci_stop_port(port_base);

    /* Allocate physical DMA frame for Command List, RX FIS, and Command Table */
    void* ctrl_frame = pmm_alloc_page();
    if (!ctrl_frame) {
        display_print("[AHCI] Failed to allocate control DMA frame for port!\n");
        return false;
    }
    uint64_t ctrl_phys = (uint64_t)ctrl_frame;
    memset(ctrl_frame, 0, 4096);

    /* Allocate physical DMA bounce buffer frame (4096 bytes) */
    void* bounce_frame = pmm_alloc_page();
    if (!bounce_frame) {
        display_print("[AHCI] Failed to allocate bounce DMA frame for port!\n");
        return false;
    }
    uint64_t bounce_phys = (uint64_t)bounce_frame;
    memset(bounce_frame, 0, 4096);

    /* Ensure physical frames above 1GB are mapped */
    void* pml4 = vmm_get_active_pml4();
    void* k_pml4 = vmm_get_kernel_pml4();
    if (ctrl_phys >= 0x40000000ULL) {
        vmm_map_page(pml4, ctrl_phys, ctrl_phys, PAGE_PRESENT | PAGE_WRITABLE | PAGE_CACHE_DISABLE);
        if (k_pml4 && k_pml4 != pml4) vmm_map_page(k_pml4, ctrl_phys, ctrl_phys, PAGE_PRESENT | PAGE_WRITABLE | PAGE_CACHE_DISABLE);
    }
    if (bounce_phys >= 0x40000000ULL) {
        vmm_map_page(pml4, bounce_phys, bounce_phys, PAGE_PRESENT | PAGE_WRITABLE | PAGE_CACHE_DISABLE);
        if (k_pml4 && k_pml4 != pml4) vmm_map_page(k_pml4, bounce_phys, bounce_phys, PAGE_PRESENT | PAGE_WRITABLE | PAGE_CACHE_DISABLE);
    }

    /* Assign pointers within partitioned control frame */
    pctx->frame_phys = ctrl_phys;
    pctx->cmd_header = (AHCICommandHeader*)((uint8_t*)ctrl_frame + 0x000);  /* 1024 bytes */
    pctx->rx_fis     = (AHCIFISReceived*)((uint8_t*)ctrl_frame + 0x400);    /* 256 bytes */
    pctx->cmd_table  = (AHCICommandTable*)((uint8_t*)ctrl_frame + 0x500);   /* 512 bytes */

    pctx->bounce_phys = bounce_phys;
    pctx->bounce_virt = (uint8_t*)bounce_frame;

    /* Program hardware base registers */
    ahci_write32(port_base + AHCI_PORT_CLB, (uint32_t)(ctrl_phys & 0xFFFFFFFF));
    ahci_write32(port_base + AHCI_PORT_CLBU, (uint32_t)(ctrl_phys >> 32));

    uint64_t fb_phys = ctrl_phys + 0x400;
    ahci_write32(port_base + AHCI_PORT_FB, (uint32_t)(fb_phys & 0xFFFFFFFF));
    ahci_write32(port_base + AHCI_PORT_FBU, (uint32_t)(fb_phys >> 32));

    /* Link Command Header 0 to Command Table */
    uint64_t ctba_phys = ctrl_phys + 0x500;
    pctx->cmd_header[0].ctba = (uint32_t)(ctba_phys & 0xFFFFFFFF);
    pctx->cmd_header[0].ctbau = (uint32_t)(ctba_phys >> 32);

    /* Clear errors and interrupts */
    ahci_write32(port_base + AHCI_PORT_SERR, 0xFFFFFFFF);
    ahci_write32(port_base + AHCI_PORT_IS, 0xFFFFFFFF);

    /* Start port engine */
    ahci_start_port(port_base);
    pctx->active = true;

    /* Wait for device ready (BSY to clear) */
    int wait_bsy = 500000;
    while (wait_bsy-- > 0) {
        uint32_t tfd = ahci_read32(port_base + AHCI_PORT_TFD);
        if (!(tfd & AHCI_PxTFD_BSY)) break;
        __asm__ volatile("pause");
    }

    /* Check device signature */
    uint32_t sig = ahci_read32(port_base + AHCI_PORT_SIG);
    pt->sig = sig;
    display_print("[AHCI] Port "); ahci_dbg_dec(port_num);
    display_print(": Device Signature = 0x");
    ahci_dbg_hex(sig); display_print("\n");

    if (sig == SATA_SIG_ATAPI) {
        display_print("[AHCI] Port is ATAPI optical drive, skipping\n");
        pctx->active = false;
        pt->state = AHCI_PORT_STATE_DEVICE_INITIALIZED;
        ahci_stop_port(port_base);
        return false;
    }

    /* Issue IDENTIFY DEVICE */
    uint16_t ident_buf[256];
    if (!ahci_identify_device(pctx, ident_buf)) {
        display_print("[AHCI] Port "); ahci_dbg_dec(port_num);
        display_print(": IDENTIFY failed!\n");
        pt->identify_pass = false;
        pctx->active = false;
        ahci_stop_port(port_base);
        return false;
    }

    pt->identify_pass = true;
    pt->state = AHCI_PORT_STATE_DEVICE_INITIALIZED;

    /* Extract drive metrics dynamically */
    int d_idx = s_ahci_drive_count;
    if (d_idx >= MAX_AHCI_DRIVES) {
        display_print("[AHCI] Maximum drive limit reached!\n");
        return true;
    }

    AHCIDriveData* ddata = &s_ahci_drives[d_idx];
    ddata->port_num = port_num;

    /* Extract Model string (words 27..46, byte-swapped) */
    for (int i = 0; i < 40; i += 2) {
        ddata->model[i] = (char)(ident_buf[27 + i/2] >> 8);
        ddata->model[i+1] = (char)(ident_buf[27 + i/2] & 0xFF);
    }
    ddata->model[40] = '\0';
    int end = 39;
    while (end >= 0 && (ddata->model[end] == ' ' || ddata->model[end] == '\0')) {
        ddata->model[end--] = '\0';
    }

    /* Extract Serial number (words 10..19, byte-swapped) */
    for (int i = 0; i < 20; i += 2) {
        ddata->serial[i] = (char)(ident_buf[10 + i/2] >> 8);
        ddata->serial[i+1] = (char)(ident_buf[10 + i/2] & 0xFF);
    }
    ddata->serial[20] = '\0';
    end = 19;
    while (end >= 0 && (ddata->serial[end] == ' ' || ddata->serial[end] == '\0')) {
        ddata->serial[end--] = '\0';
    }

    /* Extract Capacity */
    bool supports_lba48 = (ident_buf[83] & (1 << 10)) != 0;
    if (supports_lba48) {
        ddata->sector_count = (uint64_t)ident_buf[100] |
                              ((uint64_t)ident_buf[101] << 16) |
                              ((uint64_t)ident_buf[102] << 32) |
                              ((uint64_t)ident_buf[103] << 48);
    } else {
        ddata->sector_count = (uint32_t)ident_buf[60] | ((uint32_t)ident_buf[61] << 16);
    }

    /* Extract Sector Size */
    ddata->sector_size = 512;
    if ((ident_buf[106] & (1 << 14)) && !(ident_buf[106] & (1 << 15))) {
        if (ident_buf[106] & (1 << 12)) {
            ddata->sector_size = (uint32_t)ident_buf[117] | ((uint32_t)ident_buf[118] << 16);
            if (ddata->sector_size == 0) ddata->sector_size = 512;
        }
    }

    uint64_t cap_mb = (ddata->sector_count * ddata->sector_size) / (1024 * 1024);

    /* Record in telemetry structure */
    memcpy(pt->model, ddata->model, sizeof(pt->model));
    memcpy(pt->serial, ddata->serial, sizeof(pt->serial));
    pt->sector_count = ddata->sector_count;
    pt->sector_size = ddata->sector_size;
    pt->capacity_mb = cap_mb;

    display_print("[AHCI] Identified SATA Drive on Port "); ahci_dbg_dec(port_num); display_print(":\n");
    display_print("       Model:    "); display_print(ddata->model); display_print("\n");
    display_print("       Serial:   "); display_print(ddata->serial); display_print("\n");
    display_print("       Capacity: "); ahci_dbg_dec(cap_mb); display_print(" MB (");
    ahci_dbg_dec(ddata->sector_count); display_print(" sectors)\n");

    /* Register BlockDevice */
    char* name_buf = s_ahci_names[d_idx];
    name_buf[0] = 's'; name_buf[1] = 'a'; name_buf[2] = 't'; name_buf[3] = 'a';
    name_buf[4] = '_'; name_buf[5] = 'd'; name_buf[6] = 'i'; name_buf[7] = 's';
    name_buf[8] = 'k'; name_buf[9] = '0' + (d_idx % 10); name_buf[10] = '\0';

    BlockDevice* bdev = &s_ahci_bdevs[d_idx];
    bdev->name = name_buf;
    bdev->sector_size = ddata->sector_size;
    bdev->sector_count = ddata->sector_count;
    bdev->read_only = true;
    bdev->driver_data = ddata;
    bdev->read = ahci_block_device_read;
    bdev->write = NULL;
    bdev->flush = NULL;

    int bd_id = block_device_register(bdev);
    pt->bdev_id = bd_id;
    pt->state = AHCI_PORT_STATE_BDEV_REGISTERED;

    display_print("[AHCI] Registered BlockDevice "); display_print(name_buf);
    display_print(" (Global Block ID: "); ahci_dbg_dec(bd_id); display_print(")\n");

    s_ahci_drive_count++;
    return true;
}

/* Master AHCI Controller Probe & Initialization */
bool ahci_init(void) {
    display_print("\n[AHCI] Scanning PCI bus for SATA AHCI Controllers...\n");
    s_ahci_drive_count = 0;

    memset(&s_telemetry, 0, sizeof(s_telemetry));
    for (int i = 0; i < MAX_AHCI_PORTS; i++) {
        s_telemetry.ports[i].port_num = (uint8_t)i;
        s_telemetry.ports[i].bdev_id = -1;
        s_telemetry.ports[i].state = AHCI_PORT_STATE_NOT_IMPLEMENTED;
    }

    PCIDevice* ahci_dev = NULL;
    uint32_t dev_count = pci_get_device_count();

    for (uint32_t i = 0; i < dev_count; i++) {
        PCIDevice* dev = pci_get_device(i);
        if (!dev) continue;

        if (dev->base_class == 0x01 && dev->sub_class == 0x06) {
            ahci_dev = dev;
            break;
        }
    }

    if (!ahci_dev) {
        display_print("[AHCI] No PCI AHCI Controller found in system.\n");
        return false;
    }

    s_telemetry.controller_detected = true;
    s_telemetry.pci_bus = ahci_dev->bus;
    s_telemetry.pci_slot = ahci_dev->slot;
    s_telemetry.pci_func = ahci_dev->func;
    s_telemetry.vendor_id = ahci_dev->vendor_id;
    s_telemetry.device_id = ahci_dev->device_id;

    display_print("[AHCI] Found Controller at PCI ");
    ahci_dbg_dec(ahci_dev->bus); display_print(":");
    ahci_dbg_dec(ahci_dev->slot); display_print(".");
    ahci_dbg_dec(ahci_dev->func); display_print(" (Vendor: 0x");
    ahci_dbg_hex(ahci_dev->vendor_id); display_print(", Device: 0x");
    ahci_dbg_hex(ahci_dev->device_id); display_print(")\n");

    /* Enable Bus Mastering and Memory Space */
    pci_enable_memory_space(ahci_dev);
    pci_enable_bus_mastering(ahci_dev);
    display_print("[AHCI] Enabled PCI Memory Space & Bus Mastering\n");

    /* Validate and Map ABAR (BAR5) */
    uint64_t abar_phys = ahci_dev->bars[5].base_address;
    if (abar_phys == 0 || abar_phys == 0xFFFFFFFFFFFFFFFFULL) {
        display_print("[AHCI] Error: Invalid ABAR address in BAR5!\n");
        return false;
    }

    s_abar_phys = abar_phys;
    s_abar_virt = abar_phys; /* Identity mapped in ATOMS */
    s_telemetry.abar_phys = abar_phys;

    display_print("[AHCI] ABAR Physical Address: 0x");
    ahci_dbg_hex(abar_phys); display_print("\n");

    void* pml4 = vmm_get_active_pml4();
    void* k_pml4 = vmm_get_kernel_pml4();

    /* Map 4 pages (16KB) for ABAR MMIO */
    for (uint64_t p = 0; p < 4; p++) {
        uint64_t page_addr = (abar_phys + p * 4096) & PAGE_PHYS_ADDRESS_MASK;
        if (page_addr >= 0x40000000ULL) {
            vmm_map_page(pml4, page_addr, page_addr, PAGE_PRESENT | PAGE_WRITABLE | PAGE_CACHE_DISABLE);
            if (k_pml4 && k_pml4 != pml4) {
                vmm_map_page(k_pml4, page_addr, page_addr, PAGE_PRESENT | PAGE_WRITABLE | PAGE_CACHE_DISABLE);
            }
        }
    }

    /* BIOS/OS Handoff if supported (AHCI spec section 10.6.3) */
    uint32_t cap2 = ahci_read32(s_abar_virt + AHCI_REG_CAP2);
    if (cap2 & AHCI_CAP2_BOH) {
        display_print("[AHCI] BIOS/OS Handoff supported. Acquiring ownership...\n");
        uint32_t bohc = ahci_read32(s_abar_virt + AHCI_REG_BOHC);
        if (bohc & AHCI_BOHC_BOS) {
            ahci_write32(s_abar_virt + AHCI_REG_BOHC, bohc | AHCI_BOHC_OOS);
            int timeout = 100000;
            while (timeout-- > 0) {
                if (!(ahci_read32(s_abar_virt + AHCI_REG_BOHC) & AHCI_BOHC_BOS)) break;
                __asm__ volatile("pause");
            }
            if (timeout <= 0) {
                display_print("[AHCI] Warning: BIOS did not release ownership within timeout, proceeding...\n");
            } else {
                display_print("[AHCI] OS ownership acquired successfully\n");
            }
        }
    }

    /* Global HBA Reset */
    display_print("[AHCI] Issuing Global HBA Reset...\n");
    ahci_write32(s_abar_virt + AHCI_REG_GHC, AHCI_GHC_HR);
    int rst_timeout = 1000000;
    while (rst_timeout-- > 0) {
        if (!(ahci_read32(s_abar_virt + AHCI_REG_GHC) & AHCI_GHC_HR)) break;
        __asm__ volatile("pause");
    }
    if (rst_timeout <= 0) {
        display_print("[AHCI] Error: HBA Reset timed out!\n");
        return false;
    }

    /* Lock into native AHCI mode */
    ahci_write32(s_abar_virt + AHCI_REG_GHC, AHCI_GHC_AE);
    display_print("[AHCI] HBA Reset PASS. AHCI Mode Enabled (GHC=0x");
    ahci_dbg_hex(ahci_read32(s_abar_virt + AHCI_REG_GHC)); display_print(")\n");

    /* Read Host Capabilities & Version */
    uint32_t cap = ahci_read32(s_abar_virt + AHCI_REG_CAP);
    uint32_t vs  = ahci_read32(s_abar_virt + AHCI_REG_VS);
    s_telemetry.cap = cap;
    s_telemetry.version = vs;

    display_print("[AHCI] Version: ");
    ahci_dbg_dec((vs >> 16) & 0xFFFF); display_print(".");
    ahci_dbg_dec(vs & 0xFFFF); display_print(" | Capabilities: 0x");
    ahci_dbg_hex(cap); display_print("\n");

    /* Scan Ports Implemented */
    uint32_t pi = ahci_read32(s_abar_virt + AHCI_REG_PI);
    s_telemetry.ports_impl_mask = pi;

    display_print("[AHCI] Ports Implemented Bitmask: 0x");
    ahci_dbg_hex(pi); display_print("\n");

    for (uint8_t p = 0; p < MAX_AHCI_PORTS; p++) {
        if (pi & (1U << p)) {
            uint64_t port_base = s_abar_virt + 0x100 + (p * 0x80);
            ahci_init_port(p, port_base);
        }
    }

    s_telemetry.drive_count = (uint8_t)s_ahci_drive_count;

    display_print("[AHCI] Initialization complete. Total SATA Drives Discovered: ");
    ahci_dbg_dec(s_ahci_drive_count); display_print("\n\n");
    return (s_ahci_drive_count > 0);
}

int ahci_get_drive_count(void) {
    return s_ahci_drive_count;
}

AHCIDriveData* ahci_get_drive_data(int index) {
    if (index < 0 || index >= s_ahci_drive_count) return NULL;
    return &s_ahci_drives[index];
}

const AHCIControllerTelemetry* ahci_get_controller_telemetry(void) {
    return &s_telemetry;
}

const AHCIPortTelemetry* ahci_get_port_telemetry(uint8_t port_num) {
    if (port_num >= MAX_AHCI_PORTS) return NULL;
    return &s_telemetry.ports[port_num];
}
