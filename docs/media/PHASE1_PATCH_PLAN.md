# ATOMS OS — Phase 1 Architectural Patch Plan
**Subsystem:** Kernel Media Execution ➔ Ring-3 Userspace Migration  
**Milestone:** Phase 1 (Architecture & Decoupling Plan)  
**Date:** September 12, 2026  
**Status:** **APPROVED FOR IMPLEMENTATION (RULE 0 ENFORCED)**  

---

## 1. Objective

To migrate video and audio decoding completely out of the ATOMS OS kernel into an isolated Ring-3 userspace media engine process (`media_player.elf`), ensuring that any decoder stalls or crashes do not compromise the operating system, window compositor, or kernel stability.

---

## 2. Modified Files & Rationale

| File Path | Function / Scope | Change Description | Non-Obvious Rationale |
|:---|:---|:---|:---|
| `kernel/wm/bcm/src/bcm_task.c` | `bcm_compositor_thread()` | Removed synchronous `bos_media_player_tick()` | Decouples 60 FPS compositor presentation from media decode operations. |
| `kernel/wm/bwe/src/bwe_core.c` | `BOHeart_Pulse()` | Removed `bos_media_player_tick()` | Removes legacy window heartbeat media ticking. |
| `kernel/shell/apps/explorer.c` | `explorer_open_item()` | Switched from `bos_media_player_launch_file` to `sys_service_exec("/media_player.elf", media_argv, NULL)` | Uses standard userspace process spawn pipeline for all media extensions. |
| `kernel/shell/apps/bos_media_player/bos_media_player.c` | Subsystem Core | Guarded with `#ifdef ATOMS_LEGACY_KERNEL_MEDIA_ACTIVE` | Prevents duplicate in-kernel playback sessions. |
| `kernel/core/syscall/src/services.c` | `sys_service_exec()` | Added VFS ELF loading via `elf_load_image(new_pml4, path)` | Allows launching external ELFs (`/media_player.elf`, `/MEDIA.ELF`) from disk. |
| `kernel/core/memory/vmm/src/vmm.c` | `vmm_create_address_space()` & `vmm_destroy_address_space()` | Set `user_pd1[0] = 0;` and guarded copied kernel huge pages against deallocation | Retains physical RAM identity map (`user_pd1[1..511]`) for syscall VFS buffers while reserving `user_pd1[0]` (`0x40000000`..`0x40200000`) for user stack and code. |
| `tools/gpt_image_builder.c` | FAT32 ESP Partition Builder | Embedded `/MEDIA.ELF`, `/MEDIA_PLELF`, `/TEST.MP4`, and `/HEROES.MP3` | Populates the bootable test media directly into the FAT32 volume. |
| `userspace/apps/media_player/main.cpp` | `main()` & `MediaCenterWidget` | Implemented Ring-3 media player with BOSurface v2.5 and Audio HAL | Genuine userspace process operating strictly in Ring-3 (`CS & 3 == 3`). |

---

## 3. Rollback Plan

If any regression occurs:
1. Re-enable in-kernel playback by setting `#define ATOMS_LEGACY_KERNEL_MEDIA_ACTIVE 1` in `bos_media_player.c`.
2. Restore `explorer.c` media launch pointer to `bos_media_player_launch_file()`.
3. Revert `vmm.c` by removing `user_pd1[0] = 0;`.
