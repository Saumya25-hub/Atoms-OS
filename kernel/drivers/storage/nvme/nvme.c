#include "nvme.h"
#include "kernel/core/pci/pci.h"
#include "kernel/core/memory/pmm/include/pmm.h"
#include "kernel/core/memory/vmm/include/vmm.h"
#include "kernel/core/memory/vmm/include/paging.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/drivers/display/display.h"

extern void com1_puts(const char* s);

#define NVME_ADMIN_Q_SIZE   64
#define NVME_IO_Q_SIZE      64

/* Controller State */
static uint64_t s_bar0_phys = 0;
static uint64_t s_bar0_virt = 0;
static uint32_t s_dstrd = 4; /* Default 4 bytes (stride 0) */
static uint16_t s_cmd_id = 0;

/* Admin Queue Structures */
static nvme_sqe_t* s_admin_sq = NULL;
static nvme_cqe_t* s_admin_cq = NULL;
static uint64_t    s_admin_sq_phys = 0;
static uint64_t    s_admin_cq_phys = 0;
static uint16_t    s_admin_sq_tail = 0;
static uint16_t    s_admin_cq_head = 0;
static uint8_t     s_admin_cq_phase = 1;

/* I/O Queue 1 Structures */
static nvme_sqe_t* s_io_sq = NULL;
static nvme_cqe_t* s_io_cq = NULL;
static uint64_t    s_io_sq_phys = 0;
static uint64_t    s_io_cq_phys = 0;
static uint16_t    s_io_sq_tail = 0;
static uint16_t    s_io_cq_head = 0;
static uint8_t     s_io_cq_phase = 1;

/* Bounce buffer for aligned DMA */
static void*       s_bounce_buf = NULL;
static uint64_t    s_bounce_phys = 0;

/* Global Telemetry & BlockDevice */
static NVMeControllerTelemetry s_telemetry;
static BlockDevice             s_nvme_bdev;

/* MMIO Helpers */
static inline uint32_t nvme_read32(uint64_t offset) {
    return *(volatile uint32_t*)(s_bar0_virt + offset);
}

static inline void nvme_write32(uint64_t offset, uint32_t val) {
    *(volatile uint32_t*)(s_bar0_virt + offset) = val;
}

static inline uint64_t nvme_read64(uint64_t offset) {
    uint32_t low = *(volatile uint32_t*)(s_bar0_virt + offset);
    uint32_t high = *(volatile uint32_t*)(s_bar0_virt + offset + 4);
    return ((uint64_t)high << 32) | low;
}

static inline void nvme_write64(uint64_t offset, uint64_t val) {
    *(volatile uint32_t*)(s_bar0_virt + offset) = (uint32_t)(val & 0xFFFFFFFF);
    *(volatile uint32_t*)(s_bar0_virt + offset + 4) = (uint32_t)(val >> 32);
}

/* Doorbell Helpers */
static inline void nvme_ring_admin_sq(uint16_t tail) {
    nvme_write32(0x1000 + (0 * s_dstrd), (uint32_t)tail);
}

static inline void nvme_ring_admin_cq(uint16_t head) {
    nvme_write32(0x1000 + (1 * s_dstrd), (uint32_t)head);
}

static inline void nvme_ring_io_sq(uint16_t tail) {
    nvme_write32(0x1000 + (2 * s_dstrd), (uint32_t)tail);
}

static inline void nvme_ring_io_cq(uint16_t head) {
    nvme_write32(0x1000 + (3 * s_dstrd), (uint32_t)head);
}

static void nvme_dbg_hex(uint64_t val) {
    com1_puts("0x");
    if (val == 0) { com1_puts("0"); return; }
    char buf[20]; int pos = 18; buf[19] = '\0';
    const char hex[] = "0123456789ABCDEF";
    while (val > 0) { buf[pos--] = hex[val & 0xF]; val >>= 4; }
    com1_puts(&buf[pos + 1]);
}

static void nvme_dbg_dec(uint64_t val) {
    if (val == 0) { com1_puts("0"); return; }
    char buf[24]; int pos = 22; buf[23] = '\0';
    while (val > 0) { buf[pos--] = '0' + (val % 10); val /= 10; }
    com1_puts(&buf[pos + 1]);
}

/* Ensure physical frame is mapped in active and kernel page tables */
static void nvme_map_physical_frame(uint64_t phys_addr) {
    void* pml4 = vmm_get_active_pml4();
    void* k_pml4 = vmm_get_kernel_pml4();
    if (phys_addr >= 0x40000000ULL) {
        vmm_map_page(pml4, phys_addr, phys_addr, PAGE_PRESENT | PAGE_WRITABLE | PAGE_CACHE_DISABLE);
        if (k_pml4 && k_pml4 != pml4) {
            vmm_map_page(k_pml4, phys_addr, phys_addr, PAGE_PRESENT | PAGE_WRITABLE | PAGE_CACHE_DISABLE);
        }
    }
}

/* Submit command to Admin Queue and poll for completion */
static bool nvme_submit_admin_cmd(nvme_sqe_t* cmd, nvme_cqe_t* out_cqe) {
    uint16_t cid = ++s_cmd_id;
    cmd->command_id = cid;

    /* Copy into submission queue entry */
    memcpy(&s_admin_sq[s_admin_sq_tail], cmd, sizeof(nvme_sqe_t));
    __asm__ volatile("" ::: "memory");

    /* Advance SQ tail and ring doorbell */
    s_admin_sq_tail = (s_admin_sq_tail + 1) % NVME_ADMIN_Q_SIZE;
    nvme_ring_admin_sq(s_admin_sq_tail);

    /* Poll Admin Completion Queue */
    int timeout = 3000000;
    while (timeout-- > 0) {
        nvme_cqe_t* cqe = &s_admin_cq[s_admin_cq_head];
        uint8_t phase = (uint8_t)(cqe->status & 0x01);
        if (phase == s_admin_cq_phase) {
            /* Entry posted by controller */
            if (out_cqe) memcpy(out_cqe, cqe, sizeof(nvme_cqe_t));

            /* Check status */
            uint16_t status_code = (cqe->status >> 1) & 0x7FFF;

            /* Advance CQ head */
            s_admin_cq_head = (s_admin_cq_head + 1) % NVME_ADMIN_Q_SIZE;
            if (s_admin_cq_head == 0) {
                s_admin_cq_phase ^= 1; /* Invert expected phase on ring wrap */
            }
            nvme_ring_admin_cq(s_admin_cq_head);

            if (status_code != 0) {
                com1_puts("[NVMe] Admin command failed! Status=0x");
                nvme_dbg_hex(status_code); com1_puts("\r\n");
                return false;
            }
            return true;
        }
        __asm__ volatile("pause");
    }

    com1_puts("[NVMe] Admin command timed out!\r\n");
    return false;
}

/* Submit command to I/O Queue 1 and poll for completion */
static bool nvme_submit_io_cmd(nvme_sqe_t* cmd, nvme_cqe_t* out_cqe) {
    uint16_t cid = ++s_cmd_id;
    cmd->command_id = cid;

    memcpy(&s_io_sq[s_io_sq_tail], cmd, sizeof(nvme_sqe_t));
    __asm__ volatile("" ::: "memory");

    s_io_sq_tail = (s_io_sq_tail + 1) % NVME_IO_Q_SIZE;
    nvme_ring_io_sq(s_io_sq_tail);

    int timeout = 5000000;
    while (timeout-- > 0) {
        nvme_cqe_t* cqe = &s_io_cq[s_io_cq_head];
        uint8_t phase = (uint8_t)(cqe->status & 0x01);
        if (phase == s_io_cq_phase) {
            if (out_cqe) memcpy(out_cqe, cqe, sizeof(nvme_cqe_t));

            uint16_t status_code = (cqe->status >> 1) & 0x7FFF;

            s_io_cq_head = (s_io_cq_head + 1) % NVME_IO_Q_SIZE;
            if (s_io_cq_head == 0) {
                s_io_cq_phase ^= 1;
            }
            nvme_ring_io_cq(s_io_cq_head);

            if (status_code != 0) {
                com1_puts("[NVMe] I/O command failed! Status=0x");
                nvme_dbg_hex(status_code); com1_puts("\r\n");
                return false;
            }
            return true;
        }
        __asm__ volatile("pause");
    }

    com1_puts("[NVMe] I/O command timed out!\r\n");
    return false;
}

/* BlockDevice Read Callback */
static bool nvme_bdev_read_cb(struct BlockDevice* dev, uint64_t lba, uint32_t count, void* buffer) {
    (void)dev;
    return nvme_read_sectors(s_telemetry.active_nsid, lba, count, buffer);
}

/* BlockDevice Write Callback */
static bool nvme_bdev_write_cb(struct BlockDevice* dev, uint64_t lba, uint32_t count, void* buffer) {
    (void)dev;
    return nvme_write_sectors(s_telemetry.active_nsid, lba, count, (const void*)buffer);
}

/* BlockDevice Flush Callback */
static bool nvme_bdev_flush_cb(struct BlockDevice* dev) {
    (void)dev;
    return nvme_flush(s_telemetry.active_nsid);
}

/* Public I/O: Read Sectors */
bool nvme_read_sectors(uint32_t nsid, uint64_t lba, uint32_t count, void* buffer) {
    if (!s_telemetry.io_queues_created || !buffer || count == 0) return false;

    uint32_t sec_size = s_telemetry.sector_size ? s_telemetry.sector_size : 512;
    uint32_t max_sectors_per_page = 4096 / sec_size;
    uint8_t* dst_ptr = (uint8_t*)buffer;
    uint64_t cur_lba = lba;
    uint32_t remaining = count;

    while (remaining > 0) {
        uint32_t chunk = (remaining > max_sectors_per_page) ? max_sectors_per_page : remaining;

        nvme_sqe_t cmd;
        memset(&cmd, 0, sizeof(cmd));
        cmd.opcode = NVME_IO_OP_READ;
        cmd.nsid   = nsid;
        cmd.prp1   = s_bounce_phys;
        cmd.cdw10  = (uint32_t)(cur_lba & 0xFFFFFFFF);
        cmd.cdw11  = (uint32_t)(cur_lba >> 32);
        cmd.cdw12  = (chunk - 1) & 0xFFFF;

        if (!nvme_submit_io_cmd(&cmd, NULL)) {
            com1_puts("[NVMe] Read sectors failed at LBA ");
            nvme_dbg_dec(cur_lba); com1_puts("\r\n");
            return false;
        }

        memcpy(dst_ptr, s_bounce_buf, chunk * sec_size);
        dst_ptr += (chunk * sec_size);
        cur_lba += chunk;
        remaining -= chunk;
    }

    return true;
}

/* Public I/O: Write Sectors */
bool nvme_write_sectors(uint32_t nsid, uint64_t lba, uint32_t count, const void* buffer) {
    if (!s_telemetry.io_queues_created || !buffer || count == 0) return false;

    uint32_t sec_size = s_telemetry.sector_size ? s_telemetry.sector_size : 512;
    uint32_t max_sectors_per_page = 4096 / sec_size;
    const uint8_t* src_ptr = (const uint8_t*)buffer;
    uint64_t cur_lba = lba;
    uint32_t remaining = count;

    while (remaining > 0) {
        uint32_t chunk = (remaining > max_sectors_per_page) ? max_sectors_per_page : remaining;

        memcpy(s_bounce_buf, src_ptr, chunk * sec_size);

        nvme_sqe_t cmd;
        memset(&cmd, 0, sizeof(cmd));
        cmd.opcode = NVME_IO_OP_WRITE;
        cmd.nsid   = nsid;
        cmd.prp1   = s_bounce_phys;
        cmd.cdw10  = (uint32_t)(cur_lba & 0xFFFFFFFF);
        cmd.cdw11  = (uint32_t)(cur_lba >> 32);
        cmd.cdw12  = (chunk - 1) & 0xFFFF;

        if (!nvme_submit_io_cmd(&cmd, NULL)) {
            com1_puts("[NVMe] Write sectors failed at LBA ");
            nvme_dbg_dec(cur_lba); com1_puts("\r\n");
            return false;
        }

        src_ptr += (chunk * sec_size);
        cur_lba += chunk;
        remaining -= chunk;
    }

    return true;
}

/* Public I/O: Flush */
bool nvme_flush(uint32_t nsid) {
    if (!s_telemetry.io_queues_created) return false;

    nvme_sqe_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.opcode = NVME_IO_OP_FLUSH;
    cmd.nsid   = nsid;

    return nvme_submit_io_cmd(&cmd, NULL);
}

/* Trim trailing ASCII whitespace */
static void nvme_clean_ascii(char* dst, const char* src, size_t max_len) {
    size_t i = 0;
    while (i < max_len && src[i] != '\0') {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
    /* Trim trailing spaces */
    while (i > 0 && (dst[i - 1] == ' ' || dst[i - 1] == '\r' || dst[i - 1] == '\n')) {
        dst[--i] = '\0';
    }
}

/* Driver Initialization */
bool nvme_init(void) {
    com1_puts("[NVMe] Scanning PCI bus for NVMe Storage Controllers...\r\n");
    memset(&s_telemetry, 0, sizeof(s_telemetry));
    s_telemetry.bdev_id = -1;

    PCIDevice* nvme_dev = NULL;
    uint32_t dev_count = pci_get_device_count();

    for (uint32_t i = 0; i < dev_count; i++) {
        PCIDevice* dev = pci_get_device(i);
        if (!dev) continue;

        /* Class 0x01 (Mass Storage), SubClass 0x08 (NVM), ProgIF 0x02 (NVMe) */
        if (dev->base_class == 0x01 && dev->sub_class == 0x08) {
            nvme_dev = dev;
            break;
        }
    }

    if (!nvme_dev) {
        com1_puts("[NVMe] No PCI NVMe Controller found in system.\r\n");
        return false;
    }

    s_telemetry.controller_detected = true;
    s_telemetry.pci_bus = nvme_dev->bus;
    s_telemetry.pci_slot = nvme_dev->slot;
    s_telemetry.pci_func = nvme_dev->func;
    s_telemetry.vendor_id = nvme_dev->vendor_id;
    s_telemetry.device_id = nvme_dev->device_id;

    com1_puts("[NVMe] Discovered Controller at PCI ");
    nvme_dbg_dec(nvme_dev->bus); com1_puts(":");
    nvme_dbg_dec(nvme_dev->slot); com1_puts(".");
    nvme_dbg_dec(nvme_dev->func); com1_puts(" (VID=");
    nvme_dbg_hex(nvme_dev->vendor_id); com1_puts(", DID=");
    nvme_dbg_hex(nvme_dev->device_id); com1_puts(")\r\n");

    /* Enable Bus Mastering and Memory Space */
    pci_enable_memory_space(nvme_dev);
    pci_enable_bus_mastering(nvme_dev);

    /* Extract BAR0 (64-bit Non-Prefetchable MMIO) */
    uint64_t bar0 = nvme_dev->bars[0].base_address;
    if (bar0 == 0 || bar0 == 0xFFFFFFFFFFFFFFFFULL) {
        com1_puts("[NVMe] Error: Invalid BAR0 address!\r\n");
        return false;
    }

    s_bar0_phys = bar0;
    s_bar0_virt = bar0; /* Identity mapped */
    s_telemetry.bar0_phys = bar0;

    com1_puts("[NVMe] BAR0 MMIO Physical Address: ");
    nvme_dbg_hex(bar0); com1_puts("\r\n");

    /* Map 8 pages (32KB) for Controller Registers and Doorbells */
    for (uint64_t p = 0; p < 8; p++) {
        uint64_t page_addr = (bar0 + p * 4096) & PAGE_PHYS_ADDRESS_MASK;
        nvme_map_physical_frame(page_addr);
    }

    /* Read Controller Capabilities & Version */
    uint64_t cap = nvme_read64(NVME_REG_CAP);
    uint32_t vs  = nvme_read32(NVME_REG_VS);
    s_telemetry.cap = cap;
    s_telemetry.version = vs;

    uint32_t dstrd_shift = (uint32_t)((cap >> 32) & 0x0F);
    s_dstrd = (4 << dstrd_shift); /* Doorbell spacing in bytes */

    com1_puts("[NVMe] Version: ");
    nvme_dbg_dec((vs >> 16) & 0xFFFF); com1_puts(".");
    nvme_dbg_dec((vs >> 8) & 0xFF); com1_puts(".");
    nvme_dbg_dec(vs & 0xFF); com1_puts(" | CAP: ");
    nvme_dbg_hex(cap); com1_puts(" | Doorbell Stride: ");
    nvme_dbg_dec(s_dstrd); com1_puts(" bytes\r\n");

    /* Check CSTS.RDY - If already enabled, disable first to reset */
    uint32_t csts = nvme_read32(NVME_REG_CSTS);
    if (csts & NVME_CSTS_RDY) {
        com1_puts("[NVMe] Controller active, resetting...\r\n");
        nvme_write32(NVME_REG_CC, 0);
        int reset_timeout = 2000000;
        while (reset_timeout-- > 0) {
            csts = nvme_read32(NVME_REG_CSTS);
            if (!(csts & NVME_CSTS_RDY)) break;
            __asm__ volatile("pause");
        }
    }

    /* Allocate physical pages for Admin SQ, Admin CQ, and Bounce Buffer */
    void* asq_frame = pmm_alloc_page();
    void* acq_frame = pmm_alloc_page();
    void* bounce_frame = pmm_alloc_page();

    if (!asq_frame || !acq_frame || !bounce_frame) {
        com1_puts("[NVMe] Error: Failed to allocate Admin DMA frames!\r\n");
        return false;
    }

    s_admin_sq = (nvme_sqe_t*)asq_frame;
    s_admin_cq = (nvme_cqe_t*)acq_frame;
    s_bounce_buf = bounce_frame;

    s_admin_sq_phys = (uint64_t)asq_frame;
    s_admin_cq_phys = (uint64_t)acq_frame;
    s_bounce_phys = (uint64_t)bounce_frame;

    memset(s_admin_sq, 0, 4096);
    memset(s_admin_cq, 0, 4096);
    memset(s_bounce_buf, 0, 4096);

    nvme_map_physical_frame(s_admin_sq_phys);
    nvme_map_physical_frame(s_admin_cq_phys);
    nvme_map_physical_frame(s_bounce_phys);

    s_admin_sq_tail = 0;
    s_admin_cq_head = 0;
    s_admin_cq_phase = 1;

    /* Set Admin Queue Attributes: ASQS (11:0) = 63, ACQS (27:16) = 63 */
    uint32_t aqa = ((NVME_ADMIN_Q_SIZE - 1) << 16) | (NVME_ADMIN_Q_SIZE - 1);
    nvme_write32(NVME_REG_AQA, aqa);

    /* Write ASQ and ACQ Base Addresses */
    nvme_write64(NVME_REG_ASQ, s_admin_sq_phys);
    nvme_write64(NVME_REG_ACQ, s_admin_cq_phys);

    /* Configure and Enable Controller */
    uint32_t cc = NVME_CC_IOCQES_16B | NVME_CC_IOSQES_64B | NVME_CC_MPS_4K | NVME_CC_CSS_NVM | NVME_CC_EN;
    nvme_write32(NVME_REG_CC, cc);
    s_telemetry.cc = cc;

    /* Wait for CSTS.RDY == 1 */
    int enable_timeout = 3000000;
    while (enable_timeout-- > 0) {
        csts = nvme_read32(NVME_REG_CSTS);
        if (csts & NVME_CSTS_RDY) break;
        __asm__ volatile("pause");
    }

    s_telemetry.csts = csts;
    if (!(csts & NVME_CSTS_RDY)) {
        com1_puts("[NVMe] Error: Controller failed to enter RDY state! CSTS=");
        nvme_dbg_hex(csts); com1_puts("\r\n");
        return false;
    }
    com1_puts("[NVMe] Controller successfully ENABLED and READY (CSTS=");
    nvme_dbg_hex(csts); com1_puts(")\r\n");

    /* STEP 1: Identify Controller */
    nvme_sqe_t id_cmd;
    memset(&id_cmd, 0, sizeof(id_cmd));
    id_cmd.opcode = NVME_ADMIN_OP_IDENTIFY;
    id_cmd.nsid   = 0;
    id_cmd.prp1   = s_bounce_phys;
    id_cmd.cdw10  = NVME_IDENTIFY_CNS_CTRL;

    if (!nvme_submit_admin_cmd(&id_cmd, NULL)) {
        com1_puts("[NVMe] Error: Identify Controller failed!\r\n");
        return false;
    }

    nvme_id_ctrl_t* id_ctrl = (nvme_id_ctrl_t*)s_bounce_buf;
    nvme_clean_ascii(s_telemetry.model, id_ctrl->mn, sizeof(id_ctrl->mn));
    nvme_clean_ascii(s_telemetry.serial, id_ctrl->sn, sizeof(id_ctrl->sn));
    nvme_clean_ascii(s_telemetry.firmware, id_ctrl->fr, sizeof(id_ctrl->fr));
    s_telemetry.namespace_count = id_ctrl->nn;

    com1_puts("[NVMe] Identified Controller:\r\n");
    com1_puts("       Model Number:      "); com1_puts(s_telemetry.model); com1_puts("\r\n");
    com1_puts("       Serial Number:     "); com1_puts(s_telemetry.serial); com1_puts("\r\n");
    com1_puts("       Firmware Revision: "); com1_puts(s_telemetry.firmware); com1_puts("\r\n");
    com1_puts("       Active Namespaces: "); nvme_dbg_dec(s_telemetry.namespace_count); com1_puts("\r\n");

    if (s_telemetry.namespace_count == 0) {
        com1_puts("[NVMe] Error: No active namespaces reported by controller!\r\n");
        return false;
    }

    /* STEP 2: Identify Namespace 1 */
    s_telemetry.active_nsid = 1;
    memset(&id_cmd, 0, sizeof(id_cmd));
    id_cmd.opcode = NVME_ADMIN_OP_IDENTIFY;
    id_cmd.nsid   = s_telemetry.active_nsid;
    id_cmd.prp1   = s_bounce_phys;
    id_cmd.cdw10  = NVME_IDENTIFY_CNS_NAMESPACE;

    if (!nvme_submit_admin_cmd(&id_cmd, NULL)) {
        com1_puts("[NVMe] Error: Identify Namespace 1 failed!\r\n");
        return false;
    }

    nvme_id_ns_t* id_ns = (nvme_id_ns_t*)s_bounce_buf;
    uint8_t flba_idx = id_ns->flbas & 0x0F;
    uint8_t lbads = id_ns->lbaf[flba_idx].lbads;
    uint32_t sec_size = (lbads >= 9 && lbads <= 16) ? (1U << lbads) : 512;

    s_telemetry.sector_count = id_ns->nsze;
    s_telemetry.sector_size  = sec_size;
    s_telemetry.capacity_mb  = (s_telemetry.sector_count * (uint64_t)sec_size) / (1024 * 1024);

    com1_puts("[NVMe] Identified Namespace 1:\r\n");
    com1_puts("       Sector Count:      "); nvme_dbg_dec(s_telemetry.sector_count); com1_puts("\r\n");
    com1_puts("       Sector Size:       "); nvme_dbg_dec(s_telemetry.sector_size); com1_puts(" bytes\r\n");
    com1_puts("       Capacity:          "); nvme_dbg_dec(s_telemetry.capacity_mb / 1024); com1_puts(" GB (");
    nvme_dbg_dec(s_telemetry.capacity_mb); com1_puts(" MB)\r\n");

    /* STEP 3: Create I/O Completion Queue (QID = 1) */
    void* io_cq_frame = pmm_alloc_page();
    void* io_sq_frame = pmm_alloc_page();
    if (!io_cq_frame || !io_sq_frame) {
        com1_puts("[NVMe] Error: Failed to allocate I/O queue DMA frames!\r\n");
        return false;
    }

    s_io_cq = (nvme_cqe_t*)io_cq_frame;
    s_io_sq = (nvme_sqe_t*)io_sq_frame;
    s_io_cq_phys = (uint64_t)io_cq_frame;
    s_io_sq_phys = (uint64_t)io_sq_frame;

    memset(s_io_cq, 0, 4096);
    memset(s_io_sq, 0, 4096);
    nvme_map_physical_frame(s_io_cq_phys);
    nvme_map_physical_frame(s_io_sq_phys);

    s_io_sq_tail = 0;
    s_io_cq_head = 0;
    s_io_cq_phase = 1;

    nvme_sqe_t create_cq_cmd;
    memset(&create_cq_cmd, 0, sizeof(create_cq_cmd));
    create_cq_cmd.opcode = NVME_ADMIN_OP_CREATE_IO_CQ;
    create_cq_cmd.prp1   = s_io_cq_phys;
    create_cq_cmd.cdw10  = ((NVME_IO_Q_SIZE - 1) << 16) | 1; /* Queue Size, QID=1 */
    create_cq_cmd.cdw11  = (1 << 0); /* Physically Contiguous, Interrupts Disabled */

    if (!nvme_submit_admin_cmd(&create_cq_cmd, NULL)) {
        com1_puts("[NVMe] Error: Create I/O Completion Queue failed!\r\n");
        return false;
    }

    /* STEP 4: Create I/O Submission Queue (QID = 1) */
    nvme_sqe_t create_sq_cmd;
    memset(&create_sq_cmd, 0, sizeof(create_sq_cmd));
    create_sq_cmd.opcode = NVME_ADMIN_OP_CREATE_IO_SQ;
    create_sq_cmd.prp1   = s_io_sq_phys;
    create_sq_cmd.cdw10  = ((NVME_IO_Q_SIZE - 1) << 16) | 1; /* Queue Size, QID=1 */
    create_sq_cmd.cdw11  = (1 << 16) | (1 << 0); /* CQID=1, Physically Contiguous */

    if (!nvme_submit_admin_cmd(&create_sq_cmd, NULL)) {
        com1_puts("[NVMe] Error: Create I/O Submission Queue failed!\r\n");
        return false;
    }

    s_telemetry.io_queues_created = true;
    com1_puts("[NVMe] I/O Queue Pair 1 Created Successfully (SQ=64, CQ=64)\r\n");

    /* STEP 5: Register BlockDevice */
    memset(&s_nvme_bdev, 0, sizeof(s_nvme_bdev));
    s_nvme_bdev.name = "nvme0n1";
    s_nvme_bdev.sector_size = s_telemetry.sector_size;
    s_nvme_bdev.sector_count = s_telemetry.sector_count;
    s_nvme_bdev.read_only = false;
    s_nvme_bdev.driver_data = NULL;
    s_nvme_bdev.read  = nvme_bdev_read_cb;
    s_nvme_bdev.write = nvme_bdev_write_cb;
    s_nvme_bdev.flush = nvme_bdev_flush_cb;

    int bd_id = block_device_register(&s_nvme_bdev);
    s_telemetry.bdev_id = bd_id;

    com1_puts("[NVMe] Registered BlockDevice nvme0n1 (Global ID: ");
    nvme_dbg_dec(bd_id); com1_puts(")\r\n\r\n");

    return true;
}

const NVMeControllerTelemetry* nvme_get_telemetry(void) {
    return &s_telemetry;
}
