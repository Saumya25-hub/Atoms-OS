# ATOMS OS — Phase 1 Forensic Investigation Report
**Subsystem:** Kernel Media Execution ➔ Ring-3 Userspace Migration  
**Milestone:** Phase 1 (Forensic Baseline & Decoupling Audit)  
**Date:** September 12, 2026  
**Status:** **INVESTIGATION COMPLETE — FORENSIC VERIFIED (RULE 0 ENFORCED)**  

---

## 1. Executive Summary

Prior to Phase 1, media playback in ATOMS OS suffered from severe architectural flaws:
1. **Ring-0 Vulnerability**: Video decoding (`h264bsd`, `BOSpectra`) executed directly in kernel space (Ring 0). Malformed video packets could corrupt kernel page tables or crash the entire operating system.
2. **Compositor Hijacking**: The authoritative 60 FPS BCM window compositor (`kernel/wm/bcm/src/bcm_task.c`) and window heartbeat (`kernel/wm/bwe/src/bwe_core.c`) directly ticked the video decoder, causing severe frame stuttering and blocking GUI refreshes during demux/decode stalls.
3. **Hardcoded Explorer Couplings**: Double-clicking media files in File Explorer directly invoked `bos_media_player_launch_file()`, launching an in-kernel window.

---

## 2. Root Cause Analysis & Evidence

### 2.1 Compositor Loop Degradation
- **Location**: `kernel/wm/bcm/src/bcm_task.c:62-63` and `kernel/wm/bwe/src/bwe_core.c:928-929`
- **Evidence**:
  ```c
  // bcm_task.c
  bos_media_player_tick(); // Runs video decoding inside compositor thread!
  ```
- **Impact**: Every video packet decode occurred synchronously inside the desktop compositor's presentation loop, violating the single-responsibility principle and introducing latency jitter to window management.

### 2.2 Explorer Launch Path Coupling
- **Location**: `kernel/shell/apps/explorer.c:256`
- **Evidence**:
  ```c
  bos_media_player_launch_file(target_path, 0);
  ```
- **Impact**: The desktop shell bypassed the standard process lifecycle (`sys_service_exec`), executing arbitrary untrusted media files in kernel mode.

### 2.3 Address Space Memory Discrepancy under Userspace CR3
- **Location**: `kernel/core/memory/vmm/src/vmm.c:360-380`
- **Evidence**:
  When userspace processes issued syscalls (e.g. `SYS_OPEN` for `/TEST.MP4` or `/volumes/usb0/TEST.MP4`), the VFS accessed ramdisk/FAT32 buffers mapped above 1GB (e.g. `0x717C3000`).
  In `vmm_create_address_space()`, if `user_pd1` was entirely unmapped, kernel syscall routines running under the active process CR3 faulted on `0x717C3000` (#PF).
  Conversely, copying all of `k_pd1` into `user_pd1` brought along `k_pd1[0]` (a 2MB huge page covering `0x40000000`..`0x40200000`), causing `vmm_map_user_page()` to fail stack allocation at `0x400FE000` because the region was already occupied.
- **Root Cause**: Virtual memory creation must retain identity-mapped physical RAM (`user_pd1[1..511]`) while strictly clearing `user_pd1[0] = 0` to preserve the first 2MB range exclusively for userspace stack, code, and data.

---

## 3. Decoupling & Isolation Strategy

1. **BCM & BWE Cleanup**: Completely excise `bos_media_player_tick()` from `bcm_task.c` and `bwe_core.c`.
2. **Legacy Kernel Player Gating**: Guard in-kernel player with `#ifdef ATOMS_LEGACY_KERNEL_MEDIA_ACTIVE` to prevent accidental execution.
3. **Explorer Redirection**: Update `explorer.c` to launch the isolated Ring-3 media player via `sys_service_exec("/media_player.elf", media_argv, NULL)`.
4. **VMM User Isolation**: Configure `user_pd1[0] = 0` in `vmm_create_address_space()` and protect copied huge pages in `vmm_destroy_address_space()`.
5. **Userspace Media Engine**: Deploy native userspace `media_player.elf` utilizing BOSurface v2.5 shared memory blits, Audio HAL streams, and genuine libmpv event abstractions.
