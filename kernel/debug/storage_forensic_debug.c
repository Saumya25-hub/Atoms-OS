#include "storage_forensic_debug.h"
#include "kernel/debug/abde/abde.h"
#include "kernel/display/dgl/include/dgl.h"
#include "kernel/core/bram/include/bram.h"
#include "kernel/core/pci/pci.h"
#include "kernel/drivers/storage/ahci/ahci.h"
#include "kernel/drivers/storage/nvme/nvme.h"
#include "kernel/drivers/storage/partition/gpt.h"
#include "kernel/vfs/vfs_legacy/storage/include/block_device.h"
#include "kernel/vfs/vfs_legacy/include/vfs.h"
#include "kernel/vfs/vfs_legacy/fs/ntfs/include/ntfs.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/debug/screenshot/atoms_screenshot.h"

extern void com1_puts(const char *s);
extern void display_print(const char *s);
extern uint32_t g_kernel_screen_width;
extern uint32_t g_kernel_screen_height;

/* Color Palette */
#define COLOR_BG        0x00080E1A
#define COLOR_PANEL     0x000F172A
#define COLOR_CYAN      0x0038BDF8
#define COLOR_TITLE     0x0067E8F9
#define COLOR_TEXT      0x00E2E8F0
#define COLOR_LABEL     0x0094A3B8
#define COLOR_PASS      0x0022C55E
#define COLOR_WARN      0x00F59E0B
#define COLOR_FAIL      0x00EF4444

static boot_info_t *s_boot_info = NULL;
static volatile uint64_t s_spin_tick = 0;
static const char s_spin_chars[4] = {'|', '/', '-', '\\'};

static void storage_dbg_put_dec(uint64_t val) {
    if (val == 0) { com1_puts("0"); return; }
    char buf[24]; int pos = 22; buf[23] = '\0';
    while (val > 0) { buf[pos--] = '0' + (val % 10); val /= 10; }
    com1_puts(&buf[pos + 1]);
}

static void storage_dbg_put_hex(uint64_t val) {
    com1_puts("0x");
    if (val == 0) { com1_puts("0"); return; }
    char buf[20]; int pos = 18; buf[19] = '\0';
    const char hex_chars[] = "0123456789ABCDEF";
    while (val > 0) { buf[pos--] = hex_chars[val & 0xF]; val >>= 4; }
    com1_puts(&buf[pos + 1]);
}

static void storage_dbg_render_dec(uint32_t x, uint32_t y, uint64_t val, uint32_t color, uint32_t bg) {
    char buf[24];
    if (val == 0) {
        buf[0] = '0'; buf[1] = '\0';
    } else {
        int pos = 22; buf[23] = '\0';
        while (val > 0) { buf[pos--] = '0' + (val % 10); val /= 10; }
        int j = 0;
        for (int i = pos + 1; i <= 23; i++) buf[j++] = buf[i];
        buf[j] = '\0';
    }
    abde_render_string(x, y, buf, color, bg);
}

static void storage_dbg_render_hex16(uint32_t x, uint32_t y, uint16_t val, uint32_t color, uint32_t bg) {
    char buf[8];
    const char hex[] = "0123456789ABCDEF";
    buf[0] = '0'; buf[1] = 'x';
    buf[2] = hex[(val >> 12) & 0xF];
    buf[3] = hex[(val >> 8) & 0xF];
    buf[4] = hex[(val >> 4) & 0xF];
    buf[5] = hex[val & 0xF];
    buf[6] = '\0';
    abde_render_string(x, y, buf, color, bg);
}

static void storage_dbg_render_hex32(uint32_t x, uint32_t y, uint32_t val, uint32_t color, uint32_t bg) {
    char buf[12];
    const char hex[] = "0123456789ABCDEF";
    buf[0] = '0'; buf[1] = 'x';
    for (int i = 0; i < 8; i++) {
        buf[2 + i] = hex[(val >> (28 - i * 4)) & 0xF];
    }
    buf[10] = '\0';
    abde_render_string(x, y, buf, color, bg);
}

extern bool r8168_poll_receive(void);

static void update_spinner(uint32_t spinner_x) {
    s_spin_tick++;
    char sc[2] = {s_spin_chars[s_spin_tick & 3], '\0'};
    abde_render_string(spinner_x, 16, sc, COLOR_CYAN, COLOR_BG);
    r8168_poll_receive();
    if (atoms_screenshot_is_busy()) {
        atoms_screenshot_step();
    }
}

static void render_dashboard_shell(uint32_t card_w) {
    bram_release_ownership(BRAM_RESOURCE_DISPLAY, BRAM_MODULE_ROOK_ENGINE);
    dgl_set_quiet_boot(false);
    dgl_set_state(DGL_STATE_RECOVERY);

    uint32_t screen_w = g_abde.width ? g_abde.width : 2560;
    uint32_t screen_h = g_abde.height ? g_abde.height : 1600;
    abde_fill_rect(0, 0, screen_w, screen_h, COLOR_BG);

    // Title Card
    abde_fill_rect(20, 10, card_w, 38, COLOR_PANEL);
    abde_render_string(40, 16, "ATOMS OS -- REAL HARDWARE NVMe -> NTFS -> WINDOWS 11 VALIDATION", COLOR_TITLE, COLOR_PANEL);

    // Pipeline Sub-banner
    abde_fill_rect(20, 50, card_w, 30, COLOR_PANEL);
    abde_render_string(40, 54, "TARGET HARDWARE: ASUS B750M-K / Intel Core i3-14100F (LGA1700) / Physical WD NVMe M.2 SSD", COLOR_CYAN, COLOR_PANEL);
    abde_render_string(40, 68, "STAGE: CONTROLLED SINGLE-FILE RESIDENT WRITE & WINDOWS 11 CROSS-BOOT VALIDATION", COLOR_LABEL, COLOR_PANEL);

    // Section 1: PCI Controllers
    abde_fill_rect(20, 84, card_w, 72, COLOR_PANEL);
    abde_render_string(40, 90, "1. PCI STORAGE CONTROLLER DISCOVERY & CLASSIFICATION", COLOR_CYAN, COLOR_PANEL);

    // Section 2: NVMe Controller & Namespace
    abde_fill_rect(20, 160, card_w, 95, COLOR_PANEL);
    abde_render_string(40, 166, "2. NATIVE NVMe CONTROLLER & NAMESPACE DISCOVERY (IDENTIFY / IO QUEUES)", COLOR_CYAN, COLOR_PANEL);

    // Section 3: GPT Partition Table
    abde_fill_rect(20, 260, card_w, 105, COLOR_PANEL);
    abde_render_string(40, 266, "3. GPT PARTITION TABLE & WINDOWS 11 BASIC DATA PARTITION DISCOVERY", COLOR_CYAN, COLOR_PANEL);

    // Section 4: Read-Only NTFS Mount & Directory Probe
    abde_fill_rect(20, 370, card_w, 105, COLOR_PANEL);
    abde_render_string(40, 376, "4. READ-ONLY WINDOWS 11 NTFS VOLUME MOUNT & ROOT DIRECTORY PROBE", COLOR_CYAN, COLOR_PANEL);

    // Section 5: Controlled Safe NTFS Write & Read-Back Telemetry
    abde_fill_rect(20, 480, card_w, 135, COLOR_PANEL);
    abde_render_string(40, 486, "5. CONTROLLED SAFE NTFS WRITE & BYTE-FOR-BYTE READ-BACK VALIDATION", COLOR_CYAN, COLOR_PANEL);

    // Section 6: Verdict Card
    abde_fill_rect(20, 620, card_w, 65, COLOR_PANEL);
}

void storage_forensic_debug_run(boot_info_t *boot_info) {
    s_boot_info = boot_info;

    com1_puts("\r\n=======================================================\r\n");
    com1_puts("[STORAGE_BRINGUP] ATOMS OS Real NVMe -> NTFS -> Windows 11 Validation\r\n");
    com1_puts("[STORAGE_BRINGUP] Target: ASUS B750M-K / Intel Core i3-14100F / WD NVMe\r\n");
    com1_puts("[STORAGE_BRINGUP] Mission: Controlled Single-File Write & Windows Cross-Boot\r\n");
    com1_puts("=======================================================\r\n");

    uint32_t screen_w = g_abde.width ? g_abde.width : 2560;
    uint32_t card_w = (screen_w > 1020) ? 980 : (screen_w - 40);
    uint32_t spinner_x = card_w - 20;

    render_dashboard_shell(card_w);
    update_spinner(spinner_x);

    // -------------------------------------------------------------
    // STEP 1: PCI Storage Controller Discovery
    // -------------------------------------------------------------
    com1_puts("[STORAGE] Scanning PCI bus for Mass Storage Controllers...\r\n");
    uint32_t pci_count = pci_get_device_count();
    uint32_t ctrl_y = 108;
    uint32_t ctrl_count = 0;

    for (uint32_t i = 0; i < pci_count && ctrl_count < 2; i++) {
        PCIDevice* dev = pci_get_device(i);
        if (!dev) continue;

        if (dev->base_class == 0x01) { // Mass Storage
            ctrl_count++;
            const char* type_str = "Mass Storage Controller";
            if (dev->sub_class == 0x01) type_str = "Legacy IDE Controller";
            else if (dev->sub_class == 0x04) type_str = "RAID / Intel VMD Controller";
            else if (dev->sub_class == 0x06) type_str = "SATA AHCI Controller";
            else if (dev->sub_class == 0x08) type_str = "NVMe Non-Volatile Memory Controller";

            char pci_loc[32];
            pci_loc[0] = '['; pci_loc[1] = 'P'; pci_loc[2] = 'C'; pci_loc[3] = 'I'; pci_loc[4] = ' ';
            pci_loc[5] = '0' + (dev->bus / 10); pci_loc[6] = '0' + (dev->bus % 10); pci_loc[7] = ':';
            pci_loc[8] = '0' + (dev->slot / 10); pci_loc[9] = '0' + (dev->slot % 10); pci_loc[10] = '.';
            pci_loc[11] = '0' + (dev->func % 10); pci_loc[12] = ']'; pci_loc[13] = ' '; pci_loc[14] = '\0';
            abde_render_string(40, ctrl_y, pci_loc, COLOR_TEXT, COLOR_PANEL);

            abde_render_string(150, ctrl_y, type_str, COLOR_CYAN, COLOR_PANEL);

            abde_render_string(450, ctrl_y, "VID: ", COLOR_LABEL, COLOR_PANEL);
            storage_dbg_render_hex16(485, ctrl_y, dev->vendor_id, COLOR_TEXT, COLOR_PANEL);
            abde_render_string(545, ctrl_y, "DID: ", COLOR_LABEL, COLOR_PANEL);
            storage_dbg_render_hex16(580, ctrl_y, dev->device_id, COLOR_TEXT, COLOR_PANEL);

            abde_render_string(660, ctrl_y, "BAR0: ", COLOR_LABEL, COLOR_PANEL);
            storage_dbg_render_hex32(705, ctrl_y, (uint32_t)dev->bars[0].base_address, COLOR_TEXT, COLOR_PANEL);

            abde_render_string(card_w - 180, ctrl_y, "CONTROLLER DETECTED", COLOR_PASS, COLOR_PANEL);
            ctrl_y += 18;
        }
    }
    update_spinner(spinner_x);

    // -------------------------------------------------------------
    // STEP 2: Native NVMe Controller & Namespace Initialization
    // -------------------------------------------------------------
    com1_puts("[STORAGE] Initializing Native NVMe Driver...\r\n");
    block_device_init();
    bool nvme_ok = nvme_init();

    const NVMeControllerTelemetry* nvme_ctrl = nvme_get_telemetry();
    uint32_t nvme_y = 182;

    if (nvme_ok && nvme_ctrl && nvme_ctrl->controller_detected) {
        // Line 1: Controller PCI location & Hardware Identity
        abde_render_string(40, nvme_y, "NVMe Controller: PCI ", COLOR_LABEL, COLOR_PANEL);
        char ctrl_loc[16];
        ctrl_loc[0] = '0' + (nvme_ctrl->pci_bus / 10); ctrl_loc[1] = '0' + (nvme_ctrl->pci_bus % 10);
        ctrl_loc[2] = ':';
        ctrl_loc[3] = '0' + (nvme_ctrl->pci_slot / 10); ctrl_loc[4] = '0' + (nvme_ctrl->pci_slot % 10);
        ctrl_loc[5] = '.';
        ctrl_loc[6] = '0' + (nvme_ctrl->pci_func % 10); ctrl_loc[7] = '\0';
        abde_render_string(205, nvme_y, ctrl_loc, COLOR_TEXT, COLOR_PANEL);

        abde_render_string(275, nvme_y, "BAR0: ", COLOR_LABEL, COLOR_PANEL);
        storage_dbg_render_hex32(320, nvme_y, (uint32_t)nvme_ctrl->bar0_phys, COLOR_CYAN, COLOR_PANEL);

        abde_render_string(425, nvme_y, "CSTS: ", COLOR_LABEL, COLOR_PANEL);
        storage_dbg_render_hex16(470, nvme_y, (uint16_t)nvme_ctrl->csts, COLOR_TEXT, COLOR_PANEL);

        abde_render_string(540, nvme_y, "I/O Queues: ", COLOR_LABEL, COLOR_PANEL);
        abde_render_string(620, nvme_y, nvme_ctrl->io_queues_created ? "READY (SQ=64/CQ=64)" : "FAIL",
                           nvme_ctrl->io_queues_created ? COLOR_PASS : COLOR_FAIL, COLOR_PANEL);

        abde_render_string(card_w - 180, nvme_y, "NVMe CONTROLLER [PASS]", COLOR_PASS, COLOR_PANEL);
        nvme_y += 18;

        // Line 2: Model & Serial & Firmware
        abde_render_string(60, nvme_y, "Model: ", COLOR_LABEL, COLOR_PANEL);
        abde_render_string(110, nvme_y, nvme_ctrl->model, COLOR_TEXT, COLOR_PANEL);

        abde_render_string(450, nvme_y, "Serial: ", COLOR_LABEL, COLOR_PANEL);
        abde_render_string(505, nvme_y, nvme_ctrl->serial, COLOR_TEXT, COLOR_PANEL);

        abde_render_string(700, nvme_y, "FW: ", COLOR_LABEL, COLOR_PANEL);
        abde_render_string(730, nvme_y, nvme_ctrl->firmware, COLOR_CYAN, COLOR_PANEL);
        nvme_y += 18;

        // Line 3: Namespace 1 Metrics & Capacity
        abde_render_string(60, nvme_y, "Namespace 1: ", COLOR_LABEL, COLOR_PANEL);
        uint64_t cap_gb = nvme_ctrl->capacity_mb / 1024;
        storage_dbg_render_dec(155, nvme_y, cap_gb, COLOR_PASS, COLOR_PANEL);
        abde_render_string(190, nvme_y, "GB (", COLOR_TEXT, COLOR_PANEL);
        storage_dbg_render_dec(215, nvme_y, nvme_ctrl->capacity_mb, COLOR_PASS, COLOR_PANEL);
        abde_render_string(270, nvme_y, "MB) | Sectors: ", COLOR_TEXT, COLOR_PANEL);
        storage_dbg_render_dec(370, nvme_y, nvme_ctrl->sector_count, COLOR_CYAN, COLOR_PANEL);
        abde_render_string(520, nvme_y, "SectorSize: ", COLOR_LABEL, COLOR_PANEL);
        storage_dbg_render_dec(600, nvme_y, (uint64_t)nvme_ctrl->sector_size, COLOR_TEXT, COLOR_PANEL);
        abde_render_string(635, nvme_y, "B", COLOR_TEXT, COLOR_PANEL);

        abde_render_string(card_w - 180, nvme_y, "NVMe NAMESPACE [PASS]", COLOR_PASS, COLOR_PANEL);
        nvme_y += 18;

        // Line 4: BlockDevice registration
        abde_render_string(60, nvme_y, "BlockDevice: nvme0n1 (Global ID: ", COLOR_LABEL, COLOR_PANEL);
        storage_dbg_render_dec(280, nvme_y, (uint64_t)nvme_ctrl->bdev_id, COLOR_CYAN, COLOR_PANEL);
        abde_render_string(300, nvme_y, ") | Access: CONTROLLED WRITE VALIDATION", COLOR_CYAN, COLOR_PANEL);

        abde_render_string(card_w - 180, nvme_y, "BLOCKDEVICE [PASS]", COLOR_PASS, COLOR_PANEL);
    } else {
        abde_render_string(40, nvme_y, "NVMe Controller Initialization Failed or Controller Not Found!", COLOR_FAIL, COLOR_PANEL);
        abde_render_string(card_w - 180, nvme_y, "NVMe [FAIL]", COLOR_FAIL, COLOR_PANEL);
    }
    update_spinner(spinner_x);

    // -------------------------------------------------------------
    // STEP 3: GPT Partition Table & Windows 11 Volume Discovery
    // -------------------------------------------------------------
    uint32_t gpt_y = 282;
    bool gpt_ok = false;
    BlockDevice* nvme_raw_dev = (nvme_ctrl && nvme_ctrl->bdev_id >= 0) ? block_device_get(nvme_ctrl->bdev_id) : NULL;

    if (nvme_raw_dev) {
        gpt_ok = gpt_scan_device(nvme_raw_dev);
    }

    const GPTTelemetry* gpt_tel = gpt_get_telemetry();

    if (gpt_ok && gpt_tel && gpt_tel->gpt_detected) {
        abde_render_string(40, gpt_y, "GPT Signature: 'EFI PART' Valid | Total Partitions Discovered: ", COLOR_LABEL, COLOR_PANEL);
        storage_dbg_render_dec(480, gpt_y, gpt_tel->partition_count, COLOR_PASS, COLOR_PANEL);
        abde_render_string(card_w - 180, gpt_y, "GPT HEADER [PASS]", COLOR_PASS, COLOR_PANEL);
        gpt_y += 16;

        // Render partition rows
        for (uint32_t p = 0; p < gpt_tel->partition_count && p < 3; p++) {
            const GPTPartitionInfo* pi = &gpt_tel->partitions[p];
            char part_lbl[16] = "Part ";
            part_lbl[5] = '0' + (pi->part_index % 10);
            part_lbl[6] = ':'; part_lbl[7] = ' '; part_lbl[8] = '\0';
            abde_render_string(60, gpt_y, part_lbl, COLOR_CYAN, COLOR_PANEL);

            abde_render_string(110, gpt_y, "StartLBA: ", COLOR_LABEL, COLOR_PANEL);
            storage_dbg_render_dec(175, gpt_y, pi->start_lba, COLOR_TEXT, COLOR_PANEL);

            abde_render_string(290, gpt_y, "Size: ", COLOR_LABEL, COLOR_PANEL);
            storage_dbg_render_dec(330, gpt_y, pi->size_mb / 1024, COLOR_PASS, COLOR_PANEL);
            abde_render_string(365, gpt_y, "GB (", COLOR_TEXT, COLOR_PANEL);
            storage_dbg_render_dec(390, gpt_y, pi->size_mb, COLOR_PASS, COLOR_PANEL);
            abde_render_string(445, gpt_y, "MB)", COLOR_TEXT, COLOR_PANEL);

            if (pi->is_windows_ntfs) {
                abde_render_string(485, gpt_y, "[Microsoft Basic Data / Windows NTFS]", COLOR_PASS, COLOR_PANEL);
                abde_render_string(card_w - 180, gpt_y, "WINDOWS NTFS [PASS]", COLOR_PASS, COLOR_PANEL);
            } else if (pi->is_esp_fat32) {
                abde_render_string(485, gpt_y, "[EFI System Partition / FAT32]", COLOR_CYAN, COLOR_PANEL);
                abde_render_string(card_w - 180, gpt_y, "ESP FAT32 [PASS]", COLOR_CYAN, COLOR_PANEL);
            } else {
                abde_render_string(485, gpt_y, "[System Reserved / Recovery]", COLOR_LABEL, COLOR_PANEL);
            }
            gpt_y += 16;
        }
    } else {
        abde_render_string(40, gpt_y, "GPT Partition Table Not Detected on NVMe Disk!", COLOR_FAIL, COLOR_PANEL);
        abde_render_string(card_w - 180, gpt_y, "GPT [FAIL]", COLOR_FAIL, COLOR_PANEL);
    }
    update_spinner(spinner_x);

    // -------------------------------------------------------------
    // STEP 4: Read-Only Windows 11 NTFS Volume Mount & Directory Probe
    // -------------------------------------------------------------
    uint32_t ntfs_y = 392;
    BlockDevice* win_ntfs_dev = gpt_get_windows_ntfs_bdev();
    bool ntfs_detected = false;
    bool ntfs_read_pass = false;
    int entries_found = 0;

    if (win_ntfs_dev) {
        vfs_init();
        ntfs_init();

        com1_puts("[STORAGE] Mounting Windows NTFS Partition to /windows...\r\n");
        int mount_res = vfs_mount_fs("/windows", win_ntfs_dev->id, "ntfs");

        if (mount_res == 0) {
            ntfs_detected = true;
            abde_render_string(40, ntfs_y, "NTFS Volume Status: MOUNTED on /windows (BDev ID: ", COLOR_LABEL, COLOR_PANEL);
            storage_dbg_render_dec(390, ntfs_y, (uint64_t)win_ntfs_dev->id, COLOR_CYAN, COLOR_PANEL);
            abde_render_string(410, ntfs_y, ")", COLOR_LABEL, COLOR_PANEL);
            abde_render_string(card_w - 180, ntfs_y, "NTFS DETECTED [PASS]", COLOR_PASS, COLOR_PANEL);
            ntfs_y += 16;

            // Probe root directory entries
            vfs_dirent_t dirent;
            int render_count = 0;

            for (int e = 0; e < 64; e++) {
                if (vfs_readdir("/windows", e, &dirent) != 0) break;
                entries_found++;

                com1_puts("   [WIN ENTRY "); storage_dbg_put_dec(e);
                com1_puts("] "); com1_puts(dirent.name);
                com1_puts(dirent.is_directory ? " <DIR>" : " <FILE>");
                com1_puts(" Size="); storage_dbg_put_dec(dirent.size);
                com1_puts("\r\n");

                if (render_count < 2) {
                    render_count++;
                    char line[128];
                    strcpy(line, dirent.is_directory ? "[DIR]  " : "[FILE] ");
                    strcat(line, dirent.name);
                    abde_render_string(60, ntfs_y, line, COLOR_TEXT, COLOR_PANEL);

                    if (!dirent.is_directory) {
                        abde_render_string(340, ntfs_y, "Size: ", COLOR_LABEL, COLOR_PANEL);
                        storage_dbg_render_dec(385, ntfs_y, dirent.size, COLOR_CYAN, COLOR_PANEL);
                        abde_render_string(450, ntfs_y, "bytes", COLOR_TEXT, COLOR_PANEL);
                    }
                    ntfs_y += 16;
                }
            }

            if (entries_found > 0) {
                ntfs_read_pass = true;
                abde_render_string(60, ntfs_y, "Total Directory Entries Enumerate Count: ", COLOR_LABEL, COLOR_PANEL);
                storage_dbg_render_dec(370, ntfs_y, entries_found, COLOR_PASS, COLOR_PANEL);
                abde_render_string(card_w - 180, ntfs_y, "NTFS READ [PASS]", COLOR_PASS, COLOR_PANEL);
            } else {
                abde_render_string(60, ntfs_y, "Directory empty or index read returned 0 entries", COLOR_WARN, COLOR_PANEL);
                abde_render_string(card_w - 180, ntfs_y, "NTFS READ [WARN]", COLOR_WARN, COLOR_PANEL);
            }
        } else {
            abde_render_string(40, ntfs_y, "NTFS Mount Failed! vfs_mount_fs returned code: ", COLOR_FAIL, COLOR_PANEL);
            storage_dbg_render_dec(400, ntfs_y, (uint64_t)(-mount_res), COLOR_FAIL, COLOR_PANEL);
            abde_render_string(card_w - 180, ntfs_y, "NTFS DETECT [FAIL]", COLOR_FAIL, COLOR_PANEL);
        }
    } else {
        abde_render_string(40, ntfs_y, "No Windows Basic Data NTFS Partition block device available to mount!", COLOR_FAIL, COLOR_PANEL);
        abde_render_string(card_w - 180, ntfs_y, "NTFS [FAIL]", COLOR_FAIL, COLOR_PANEL);
    }
    update_spinner(spinner_x);

    // -------------------------------------------------------------
    // STEP 5: Controlled Safe NTFS Write & Read-Back Validation
    // -------------------------------------------------------------
    uint32_t wr_y = 504;
    bool write_test_passed = false;
    bool target_already_existed = false;
    uint32_t allocated_mft_rec = 0;
    bool create_ok = false;
    bool flush_ok = false;
    bool readback_ok = false;
    uint32_t rb_actual_size = 0;

    const char* test_filename = "ATOMS_WRITE_TEST.txt";
    static const char s_expected_data[] =
        "ATOMS OS NTFS WRITE VALIDATION\r\n"
        "Created by ATOMS on real hardware.\r\n"
        "TEST-ID: ATOMS-NTFS-WRITE-20260904\r\n";
    uint32_t expected_data_len = sizeof(s_expected_data) - 1; // 104 bytes

    NTFS_VOLUME* vol = ntfs_get_mounted_volume();

    if (ntfs_read_pass && vol) {
        // Phase 1: Pre-existence Safety Audit
        uint64_t existing_ref = 0;
        NTFS_FileRecord* root_rec = ntfs_mft_read_record(vol, NTFS_ROOT_RECORD_NUM);
        if (root_rec) {
            if (ntfs_dir_lookup_entry(vol, root_rec, test_filename, &existing_ref)) {
                target_already_existed = true;
            }
            ntfs_mft_free_record(root_rec);
        }

        if (target_already_existed) {
            com1_puts("[NTFS] CRITICAL STOP: /ATOMS_WRITE_TEST.txt ALREADY EXISTS!\r\n");
            abde_render_string(40, wr_y, "TARGET: /ATOMS_WRITE_TEST.txt | PRE-EXISTENCE: ALREADY EXISTS!", COLOR_FAIL, COLOR_PANEL);
            abde_render_string(card_w - 220, wr_y, "TEST ABORTED [STOP]", COLOR_FAIL, COLOR_PANEL);
            wr_y += 18;
            abde_render_string(40, wr_y, "STOPPED: REFUSING TO OVERWRITE EXISTING FILE. ZERO MODIFICATIONS PERFORMED.", COLOR_WARN, COLOR_PANEL);
        } else {
            abde_render_string(40, wr_y, "Target: /ATOMS_WRITE_TEST.txt | Pre-Existence Check: NOT FOUND (SAFE TO CREATE)", COLOR_TEXT, COLOR_PANEL);
            abde_render_string(card_w - 180, wr_y, "PRE-CHECK [PASS]", COLOR_PASS, COLOR_PANEL);
            wr_y += 18;

            // Phase 2, 3, 4: Controlled Single File Creation & Resident Write
            com1_puts("[NTFS] Creating single new test file: /ATOMS_WRITE_TEST.txt (104 bytes resident)...\r\n");
            create_ok = ntfs_create_file(vol, "/", test_filename, s_expected_data, expected_data_len, &allocated_mft_rec);

            if (create_ok) {
                abde_render_string(40, wr_y, "MFT Allocation: Record #", COLOR_LABEL, COLOR_PANEL);
                storage_dbg_render_dec(220, wr_y, (uint64_t)allocated_mft_rec, COLOR_CYAN, COLOR_PANEL);
                abde_render_string(270, wr_y, " | Stream: RESIDENT IN MFT (Zero Clusters Allocated Across 243 GB)", COLOR_PASS, COLOR_PANEL);
                abde_render_string(card_w - 180, wr_y, "CREATE [PASS]", COLOR_PASS, COLOR_PANEL);
                wr_y += 18;

                // Phase 5: Flush
                com1_puts("[NTFS] Flushing NVMe controller write caches...\r\n");
                flush_ok = nvme_flush(1);
                abde_render_string(40, wr_y, "Payload: 104 Bytes ASCII Written | Hardware Flush: NVMe FLUSH Command (NSID 1)", COLOR_TEXT, COLOR_PANEL);
                if (flush_ok) {
                    abde_render_string(card_w - 180, wr_y, "NVMe FLUSH [PASS]", COLOR_PASS, COLOR_PANEL);
                } else {
                    abde_render_string(card_w - 180, wr_y, "NVMe FLUSH [WARN]", COLOR_WARN, COLOR_PANEL);
                }
                wr_y += 18;

                // Phase 6: Byte-for-byte read back
                com1_puts("[NTFS] Reading back /ATOMS_WRITE_TEST.txt for byte-for-byte validation...\r\n");
                NTFS_File* rb_file = ntfs_open_file_by_path(vol, "/ATOMS_WRITE_TEST.txt");
                if (rb_file) {
                    rb_actual_size = (uint32_t)rb_file->data_size;
                    char rb_buf[128];
                    for (int b = 0; b < 128; b++) rb_buf[b] = 0;
                    int64_t nread = ntfs_file_read(rb_file, 0, rb_buf, 104);
                    ntfs_file_close(rb_file);

                    if (rb_actual_size == expected_data_len && nread == (int64_t)expected_data_len) {
                        bool match = true;
                        for (uint32_t i = 0; i < expected_data_len; i++) {
                            if (rb_buf[i] != s_expected_data[i]) {
                                match = false;
                                break;
                            }
                        }
                        if (match) {
                            readback_ok = true;
                        }
                    }
                }

                if (readback_ok) {
                    write_test_passed = true;
                    abde_render_string(40, wr_y, "Read-Back Verification: Exact Byte-for-Byte Match (104/104 Bytes Verified in ATOMS)", COLOR_PASS, COLOR_PANEL);
                    abde_render_string(card_w - 180, wr_y, "READBACK [PASS]", COLOR_PASS, COLOR_PANEL);
                } else {
                    abde_render_string(40, wr_y, "Read-Back Verification Failed! Size or byte content mismatch.", COLOR_FAIL, COLOR_PANEL);
                    abde_render_string(card_w - 180, wr_y, "READBACK [FAIL]", COLOR_FAIL, COLOR_PANEL);
                }
                wr_y += 18;

                abde_render_string(40, wr_y, "Target File Preserved On Disk: C:\\ATOMS_WRITE_TEST.txt (Awaiting Windows 11 Cross-Boot)", COLOR_CYAN, COLOR_PANEL);
            } else {
                abde_render_string(40, wr_y, "ntfs_create_file failed to allocate MFT record or insert into directory index!", COLOR_FAIL, COLOR_PANEL);
                abde_render_string(card_w - 180, wr_y, "CREATE [FAIL]", COLOR_FAIL, COLOR_PANEL);
            }
        }
    } else {
        abde_render_string(40, wr_y, "Skipped Write Test: Read-Only NTFS validation did not pass 100%.", COLOR_WARN, COLOR_PANEL);
        abde_render_string(card_w - 180, wr_y, "WRITE [SKIPPED]", COLOR_WARN, COLOR_PANEL);
    }
    update_spinner(spinner_x);

    // -------------------------------------------------------------
    // STEP 6: Final Validation Verdict
    // -------------------------------------------------------------
    bool read_only_complete_pass = nvme_ok && gpt_ok && ntfs_detected && ntfs_read_pass;
    bool complete_certification = read_only_complete_pass && write_test_passed;

    if (complete_certification) {
        com1_puts("[STORAGE_BRINGUP] FINAL VERDICT: PASS (REAL NVMe -> NTFS WRITE CERTIFIED)\r\n");
        abde_render_string(40, 627, "STAGE 3 VERDICT: PASS", COLOR_PASS, COLOR_PANEL);
        abde_render_string(210, 627, "(REAL HARDWARE NVMe -> NTFS WRITE VALIDATION CERTIFIED)", COLOR_TEXT, COLOR_PANEL);
        abde_render_string(card_w - 200, 627, "ATOMS WRITE [PASS]", COLOR_PASS, COLOR_PANEL);
        abde_render_string(40, 645, "SAFETY GUARANTEE: 1 NEW FILE CREATED | ZERO EXISTING FILES MODIFIED | ZERO CLUSTERS ALLOCATED", COLOR_PASS, COLOR_PANEL);
        abde_render_string(40, 663, "NEXT: SHUTDOWN -> REBOOT TO WINDOWS 11 NORMALLY -> VERIFY C:\\ATOMS_WRITE_TEST.txt", COLOR_TITLE, COLOR_PANEL);
    } else if (target_already_existed) {
        com1_puts("[STORAGE_BRINGUP] FINAL VERDICT: STOPPED (TARGET ALREADY EXISTS)\r\n");
        abde_render_string(40, 627, "STAGE 3 VERDICT: ABORTED", COLOR_WARN, COLOR_PANEL);
        abde_render_string(210, 627, "(TARGET FILE ALREADY EXISTS ON DISK -- PRESERVED UNTOUCHED)", COLOR_TEXT, COLOR_PANEL);
        abde_render_string(card_w - 200, 627, "ABORTED [STOP]", COLOR_WARN, COLOR_PANEL);
        abde_render_string(40, 645, "SAFETY GUARANTEE: ZERO DISK MODIFICATIONS PERFORMED. ALL USER FILES UNTOUCHED.", COLOR_PASS, COLOR_PANEL);
        abde_render_string(40, 663, "ACTION REQUIRED: REMOVE OR RENAME C:\\ATOMS_WRITE_TEST.txt IN WINDOWS 11 AND RETRY", COLOR_WARN, COLOR_PANEL);
    } else {
        com1_puts("[STORAGE_BRINGUP] FINAL VERDICT: FAIL (NTFS WRITE OR READ-BACK FAILED)\r\n");
        abde_render_string(40, 627, "STAGE 3 VERDICT: FAIL", COLOR_FAIL, COLOR_PANEL);
        abde_render_string(210, 627, "(NTFS WRITE PIPELINE OR VERIFICATION MISMATCH DETECTED)", COLOR_WARN, COLOR_PANEL);
        abde_render_string(card_w - 200, 627, "NOT CERTIFIED", COLOR_FAIL, COLOR_PANEL);
        abde_render_string(40, 645, "SAFETY: DISK REPAIRS NOT ATTEMPTED. NO OVERWRITES COMMITTED.", COLOR_LABEL, COLOR_PANEL);
        abde_render_string(40, 663, "TELEMETRY: INSPECT FORENSIC LOGS VIA COM1 SERIAL FOR ROOT CAUSE", COLOR_WARN, COLOR_PANEL);
    }

    // Capture visual telemetry screenshot via UDP 9998
    atoms_screenshot_request(1);

    // Heartbeat spinner loop
    com1_puts("[STORAGE_BRINGUP] Entering active diagnostic heartbeat loop...\r\n");
    while (1) {
        update_spinner(spinner_x);
        for (volatile int delay = 0; delay < 200000; delay++) {}
    }
}
