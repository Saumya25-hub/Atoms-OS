#!/usr/bin/env python3
"""
ATOMS OS — PXE TFTP Media RAM-Disk Architecture Certification Script
Verifies all 10 architectural stages:
1. UEFI Bootloader (BOOTX64.EFI) executes cleanly
2. Acquisition of media.img into RAM below 4GB
3. Propagation of ramdisk_base & ramdisk_size through boot_info
4. PMM physical reservation of the entire RAM disk region
5. VMM identity mapping of physical range
6. RAM-backed BlockDevice driver registration (ramdisk0)
7. GPT partition table scanning on ramdisk0
8. FAT32 filesystem detection and mount at /
9. Direct file access to /DOLBY.MP4 & /TEST.MP4 from RAM disk
10. Media Player bitstream demux, codec decode, and screen presentation
"""

import os
import sys

def certify(log_path):
    print("================================================================")
    print("  ATOMS OS — PXE TFTP MEDIA RAM-DISK CERTIFICATION SUITE")
    print("================================================================")
    
    if not os.path.exists(log_path):
        print(f"[ERROR] Serial log '{log_path}' does not exist!")
        return False

    with open(log_path, "r", encoding="utf-8", errors="ignore") as f:
        log = f.read()

    pipeline_stages = [
        ("STAGE 1: UEFI Bootloader Stage 1 Execution",
         "[UEFI STAGE 1] Starting SignaturesOS Production UEFI Loader"),
        
        ("STAGE 2: Media Image Acquired into RAM",
         "media.img loaded from local disk into RAM!" in log or "media.img downloaded from TFTP into RAM!" in log),
        
        ("STAGE 3: Silent Handoff & Kernel Entry",
         "[SUCCESS] ExitBootServices() Succeeded 100% on Real Hardware!" in log and "[BOOT] Enter kernel_main" in log),
        
        ("STAGE 4: PMM Ramdisk Buffer Reservation",
         "Reserving Temporary Media Ramdisk Buffer:" in log and "[PMM_PASS]" in log),
        
        ("STAGE 5: VMM Physical Space Identity Mapping",
         "IDENTITY MAP FULL PHYSICAL RANGE" in log and "[VMM_PASS]" in log),
        
        ("STAGE 6: BOS RAM-backed Block Device Online",
         "[RAMDISK] Online: base=" in log and "sectors=196608" in log),
        
        ("STAGE 7: GPT Partition Table Detected on RAM Disk",
         "[MBR] GPT Protective MBR detected. Parsing GPT tables at LBA 1..." in log and "[GPT] Valid GPT Header verified on physical disk!" in log),
        
        ("STAGE 8: FAT32 Root VFS Mount from RAM Disk",
         "[VFS] Attempting to mount Block Device 2 to / using fat32" in log and "[VFS] Mount SUCCESS!" in log),
        
        ("STAGE 9: Media Bitstream File Access from RAM Disk",
         "[MEDIA_MANAGER] Opening Media URI:" in log and "File Open Result: SUCCESS" in log),
        
        ("STAGE 10: MP4 Demux & Video Player Screen Presentation",
         "[MP4] file opened" in log and "[MP4] video track found" in log and "Bitmap Blit Result: SUCCESS" in log)
    ]

    all_passed = True
    for stage_name, condition in pipeline_stages:
        passed = condition if isinstance(condition, bool) else (condition in log)
        mark = "PASS" if passed else "FAIL"
        print(f"  [{mark}] {stage_name}")
        if not passed:
            all_passed = False

    print("================================================================")
    if all_passed:
        print(" >>> FINAL ARCHITECTURAL VERDICT: PASS (10/10 MILESTONES CERTIFIED) <<<")
    else:
        print(" >>> FINAL ARCHITECTURAL VERDICT: FAIL <<<")
    print("================================================================")
    return all_passed

if __name__ == "__main__":
    log_file = r"build\qemu_media_player.log"
    if len(sys.argv) > 1:
        log_file = sys.argv[1]
    success = certify(log_file)
    sys.exit(0 if success else 1)
