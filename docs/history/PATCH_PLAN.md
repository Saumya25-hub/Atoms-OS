# PATCH PLAN: Media Telemetry Presentation, Profile Compatibility & Clean Window Lifecycle

**Date:** 2026-09-13  
**Author:** System Architecture Team  
**Input:** `FORENSIC_REPORT.md`  
**Status:** PROPOSED FOR USER APPROVAL (ARCHITECTURE PLAN — NO CODE COMMITTED)  

---

## 1. Architectural Objectives

1. **Active Frame Presentation & Telemetry Counter Update**:
   - Ensure `bos_media_pipeline.cpp` increments `p->telemetry.presented_frames++` on every presented frame and updates `p->telemetry.video_pts_ms` and `p->telemetry.drift_ms`.
   - Resolve the issue where `[MEDIA_SYNC]` telemetry continuously printed zeroes (`PRESENTED=00`).
2. **H.264 Header & Slice Consumption Guard**:
   - In `bos_media_pipeline.cpp`, handle `h264bsdDecode` returning `2` (`H264BSD_HDRS_RDY`): when `read_bytes > 0`, advance the buffer pointer; when `read_bytes == 0`, permit immediate single-pass slice data decode without running into an infinite loop or terminating at 100 iterations.
3. **High Profile CABAC Detection & Overlay**:
   - When an H.264 stream is High Profile with CABAC entropy encoding (`profile == 100`), display an explicit on-screen diagnostic banner:
     `[H.264 High Profile (CABAC) Detected — Transcode to Baseline (CAVLC) for full hardware rendering]`
     instead of a silent dark/gray viewport.
4. **Certified Baseline Test Media Configuration**:
   - In `run_uefi_forensic_test.ps1` and `tools/gpt_image_builder.c`, burn the verified Baseline CAVLC video (`TEST-VIDEO/test.mp4`, 20/20 flawless frames, `num_err = 0`) as `/TEST.MP4` on `/volumes/usb0/`.
   - Keep `/volumes/usb0/Dolby_Vision_AtmosHDR.mp4` accessible for High Profile testing.
5. **Clean Desktop Window Lifecycle & Debouncing**:
   - In `userspace/apps/desktop_shell/main.c`, add a debounce check in `exec_media_player()` (cooldown 1500 ms).
   - Dismiss File Explorer (`g_app_win.active = false`) upon launching Media Player to avoid multi-window collisions and physical page reuse corruption.
   - In `userspace/apps/media_player/main.cpp`, eliminate `media_widget.clear_parent()`, sizing the widget to `window.client_size()`.

---

## 2. File-by-File Modification Specifications

### File 1: `userspace/libbos_media/src/bos_media_pipeline.cpp`
- **Target Section 1 (Frame Presentation & Telemetry Updates)**:
  - **Location**: Lines ~653–715 inside `bos_media_pipeline_render_frame()`
  - **Modification**:
    ```c
    // Present Frame
    p->telemetry.presented_frames++;
    p->telemetry.video_pts_ms = (int64_t)(pframe.pts_us / 1000);
    p->telemetry.drift_ms = (int64_t)(p->scheduler.last_drift_ms);
    ```
  - **Rationale**: Connects the frame presentation engine directly to the telemetry pipeline so `bos_media_dump_sync_telemetry()` reflects real, advancing playback frames.
- **Target Section 2 (H264 NAL Header & Slice Consumption Loop)**:
  - **Location**: Lines ~536–549 inside `bos_media_pipeline_render_frame()`
  - **Modification**: When `dec_ret == 2 /* H264BSD_HDRS_RDY */`, advance `cur_ptr` and decrement `rem_bytes` if `read_bytes > 0`. If `read_bytes == 0`, set a one-shot refeed flag and re-invoke `h264bsdDecode` once for the slice payload.
  - **Rationale**: Prevents infinite `continue` loops and avoids terminating the sample decode with unconverted slice bytes.
- **Target Section 3 (High Profile CABAC Visual Notice)**:
  - **Location**: Lines ~716–748 inside fallback rendering
  - **Modification**: If `p->meta.video_codec` indicates High Profile and macroblock errors occur (`num_err > 0`), render an aesthetic on-screen badge in the viewport explaining the profile constraint.

---

### File 2: `userspace/apps/media_player/main.cpp`
- **Target Section 1 (Widget Attachment & Client Sizing)**:
  - **Location**: Lines ~408–412
  - **Modification**: Remove `media_widget.clear_parent();`. Explicitly set `media_widget.set_bounds(Rect(0, 0, window.client_size().width, window.client_size().height));`.
  - **Rationale**: Fixes widget tree detachment and ensures the viewport bounds match the client area precisely.

---

### File 3: `run_uefi_forensic_test.ps1`
- **Target Section 1 (Test Media Asset Selection)**:
  - **Location**: Lines ~24–27
  - **Modification**: Copy `TEST-VIDEO\test.mp4` to `build\TEST.MP4` (Baseline Profile CAVLC) instead of overwriting it with the raw 38 MB Dolby Vision High Profile file. Copy `TEST1[TEMP]\Dolby_Vision_AtmosHDR.mp4` strictly to `build\DOLBY.MP4`.
  - **Rationale**: Ensures the default test stream `/volumes/usb0/TEST.MP4` is 100% playable on the Hantro G1 integer decoder.

---

### File 4: `tools/gpt_image_builder.c`
- **Target Section 1 (MP4 Opening Priority)**:
  - **Location**: Lines ~356–361
  - **Modification**: Check `build/TEST.MP4` and `TEST-VIDEO/test.mp4` first for the primary `/TEST.MP4` FAT32 entry. Ensure `/DOLBY.MP4` is linked to `build/DOLBY.MP4`.
  - **Rationale**: Aligns FAT32 disk image layout with the certified decoder profile.

---

### File 5: `userspace/apps/desktop_shell/main.c`
- **Target Section 1 (Media Player Launch Debounce & Window Dismissal)**:
  - **Location**: Lines ~1120–1142 (`exec_media_player`) and lines ~1171–1182 (`handle_window_client_click`)
  - **Modification**: Add a 1500 ms cooldown timer `s_last_media_exec_ms` in `exec_media_player()`. Set `g_app_win.active = false` upon dispatching the media player.
  - **Rationale**: Prevents double/triple process spawns and avoids window overlap crashes.

---

### File 6: `kernel/core/syscall/src/services.c`
- **Target Section (User MMap Bump Pointer & Safe Virtual Zeroing)**:
  - **Location**: Lines ~528–571 (`sys_service_mmap`)
  - **Modification**:
    1. Relocate `s_user_mmap_bump` base from `0x50000000ULL` to `0x60000000ULL` (1.5 GB), strictly separating it from window surfaces (`0x50000000ULL..0x5F000000ULL`).
    2. Derive `target_pml4` from `hw_cr3 & PAGE_PHYS_ADDRESS_MASK` (fallback `current->pml4`).
    3. Perform `vmm_map_page(target_pml4, ...)` and `current->pml4`.
    4. Move `memset` to operate on the mapped virtual address `(void*)(virt_start + off)` instead of the raw physical pointer `phys`.
    5. Run `vmm_walk_and_verify(target_pml4, virt_start)` and log `[SYS_MMAP]` diagnostic.
  - **Rationale**: Eliminates `#PF` caused by virtual address collisions and unsafe physical address accesses under process CR3.

### File 7: `kernel/core/memory/vmm/src/vmm.c`
- **Target Section 1 (`vmm_create_address_space` Zero-Initialization of `user_pd1`)**:
  - **Location**: Lines ~464–482
  - **Modification**:
    Replace copying of `k_pd1` with clean zero initialization:
    ```c
    uint64_t *user_pd1 = (uint64_t *)pmm_alloc_page();
    if (!user_pd1) { pmm_free_page(new_pdp); pmm_free_page(new_pml4); return NULL; }
    for (int i = 0; i < 512; i++) user_pd1[i] = 0;
    ```
  - **Rationale**: Eliminates process address space pollution from kernel identity mappings and splits (e.g. `0x0C72A000` from `vmm_init` stress tests). Guarantees every userspace Page Table is allocated fresh per-process on demand.
- **Target Section 2 (`vmm_destroy_address_space` Shared Kernel PT Guard)**:
  - **Location**: Lines ~548–552
  - **Modification**:
    Add check verifying that `pt_phys` does not match any entry in `k_pd`:
    ```c
    if (k_pdp && (k_pdp[pdp_idx] & PAGE_PRESENT)) {
        uint64_t *k_pd = (uint64_t*)(k_pdp[pdp_idx] & PAGE_PHYS_ADDRESS_MASK);
        if (k_pd && (k_pd[pd_idx] & PAGE_PRESENT) &&
            !(k_pd[pd_idx] & PAGE_HUGE) &&
            (k_pd[pd_idx] & PAGE_PHYS_ADDRESS_MASK) == pt_phys) {
            pd_table[pd_idx] = 0;
            continue;
        }
    }
    ```
  - **Rationale**: Completely prevents process teardown from freeing kernel-owned Page Tables into PMM's free list.

---

## 3. Expected Verification Results

1. **QEMU Pre-Flight**:
   - `build\BOOTX64.EFI` and `build\atoms_uefi_test.img` build cleanly with zero errors.
   - Booting in QEMU UEFI mode displays the desktop shell without crashes.
   - User `mmap` allocations (`0x60000000` and `0x60011000`) succeed with `RESULT=PASS (PRESENT=1 WRITABLE=1 USER=1)`.
   - Clicking `TEST.MP4` in File Explorer launches `media_player.elf` with active video playback (`PRESENTED` counter incrementing from 1 to 20+ frames).
   - Telemetry outputs:
     `[MEDIA_SYNC] AUDIO_PTS=... VIDEO_PTS=... DRIFT_MS=0 DROPPED_FRAMES=0 PRESENTED=...`.
2. **PXE Real Hardware Verification**:
   - Real hardware client (`A0:AD:9F:C5:81:27`) boots the updated `BOOTX64.EFI` via TFTP.
   - ABDE diagnostic table renders cleanly.
   - Heartbeat spinner rotates smoothly (`| / - \`).

---

## 4. Risk Analysis & Containment

- **Subsystem Scope**: Confined strictly to `vmm.c` (isolated address space creation/destruction), `services.c` (`sys_service_mmap`), userspace `libbos_media`, `media_player`, `desktop_shell`, and image builder tools.
- **Kernel Subsystems**: `kernel.c`, scheduler, PMM bitmap logic, and hardware drivers remain completely untouched.
- **Rollback Plan**: All changes are reversible via clean git checkout.

