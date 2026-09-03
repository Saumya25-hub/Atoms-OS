#include "kernel/vfs/vfs_legacy/storage/include/mbr.h"
#include "kernel/drivers/display/display.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/vfs/vfs_legacy/storage/include/disk_manager.h"

// Note: In Sprint 4 (Disk Manager) we will create a global partition registry.
// For Sprint 3, we just parse and print/validate.

void mbr_parse(int block_device_id) {
    display_print("\n[MBR] Reading Sector 0...\n");

    MBR_Sector* sector = (MBR_Sector*)kmalloc(512);
    if (!sector) {
        display_print("[MBR] Error: kmalloc failed\n");
        return;
    }

    if (!block_device_read(block_device_id, 0, 1, sector)) {
        display_print("[MBR] Error: Failed to read from Block Device Layer\n");
        kfree(sector);
        return;
    }

    if (sector->signature == 0xAA55) {
        display_print("[MBR] Signature 55AA PASS\n");
    } else {
        display_print("[MBR] Signature 55AA FAILED (Found 0x");
        display_print_hex(sector->signature);
        display_print(")\n");
        kfree(sector);
        return;
    }

    int active_partitions = 0;
    for (int i = 0; i < 4; i++) {
        if (sector->partitions[i].type != 0x00) { // Type 0x00 is empty
            active_partitions++;
        }
    }

    display_print("[MBR] Partition Count : ");
    display_print_dec(active_partitions);
    display_print("\n");

    for (int i = 0; i < 4; i++) {
        if (sector->partitions[i].type != 0x00) {
            display_print("\n[MBR] Partition ");
            display_print_dec(i + 1);
            display_print("\n");
            
            display_print("Bootable : ");
            if (sector->partitions[i].status == 0x80) {
                display_print("Yes\n");
            } else {
                display_print("No\n");
            }
            
            display_print("Type : 0x");
            display_print_hex(sector->partitions[i].type);
            display_print("\n");
            
            display_print("Start LBA : ");
            display_print_dec(sector->partitions[i].start_lba);
            display_print("\n");
            
            display_print("Sector Count : ");
            display_print_dec(sector->partitions[i].sector_count);
            display_print("\n");
            
            // Check for GPT Protective MBR
            if (sector->partitions[i].type == 0xEE) {
                display_print("[MBR] GPT Protective MBR detected. Parsing GPT tables at LBA 1...\n");
                uint8_t* gpt_hdr_buf = (uint8_t*)kmalloc(512);
                if (gpt_hdr_buf) {
                    if (block_device_read(block_device_id, 1, 1, gpt_hdr_buf)) {
                        uint64_t gpt_sig = *(uint64_t*)gpt_hdr_buf;
                        if (gpt_sig == 0x5452415020494645ULL) { // "EFI PART"
                            display_print("[GPT] Valid GPT Header verified on physical disk!\n");
                            uint64_t part_array_lba = *(uint64_t*)(gpt_hdr_buf + 72);
                            uint32_t num_parts = *(uint32_t*)(gpt_hdr_buf + 80);
                            uint32_t part_size = *(uint32_t*)(gpt_hdr_buf + 84);
                            if (part_size == 0) part_size = 128;

                            uint8_t* gpt_part_buf = (uint8_t*)kmalloc(512);
                            if (gpt_part_buf) {
                                if (block_device_read(block_device_id, part_array_lba, 1, gpt_part_buf)) {
                                    for (uint32_t p = 0; p < 4 && p < num_parts; p++) {
                                        uint8_t* p_entry = gpt_part_buf + (p * part_size);
                                        bool is_empty = true;
                                        for (int g = 0; g < 16; g++) {
                                            if (p_entry[g] != 0) { is_empty = false; break; }
                                        }
                                        if (!is_empty) {
                                            uint64_t p_start = *(uint64_t*)(p_entry + 32);
                                            uint64_t p_end = *(uint64_t*)(p_entry + 40);
                                            uint64_t p_count = (p_end >= p_start) ? (p_end - p_start + 1) : 0;
                                            if (p_count > 0) {
                                                display_print("[GPT] Partition Registered! Start LBA: ");
                                                display_print_dec(p_start);
                                                display_print(" Sectors: ");
                                                display_print_dec(p_count);
                                                display_print("\n");
                                                disk_manager_register_partition(block_device_id, p_start, p_count, 0xEE);
                                            }
                                        }
                                    }
                                }
                                kfree(gpt_part_buf);
                            }
                        }
                    }
                    kfree(gpt_hdr_buf);
                }
                continue;
            }

            // Register standard MBR partition
            disk_manager_register_partition(
                block_device_id,
                sector->partitions[i].start_lba,
                sector->partitions[i].sector_count,
                sector->partitions[i].type
            );
        }
    }
    
    display_print("\n[MBR] PASS\n");
    kfree(sector);
}
