# ROOTFS PLAN: Boundary B14 — Root Filesystem Provisioning & Mountroot Architecture

**Document Version**: 1.0  
**Phase**: Pre-Patch Architecture Plan (Rule 0 / Phase Isolation)  
**Target Milestone**: Boundary B14 (FreeBSD Kernel Mounts Root & Reaches `/sbin/init` Login Prompt)  
**Target Hardware**: Physical ASUS PRIME B760M-K / Intel Core i3-14100F (8GB RAM)  
**Precondition**: Boundary B13 Certified (`OBSERVED-PASS` — VirtIO-BLK Interrupt Delivery, IOAPIC RTE 11 Decoding, and Request Queue verified).  

---

## 1. Executive Summary

With Boundary B13 certified on real physical hardware:
1. VirtIO-BLK device `vtbd0` is recognized by FreeBSD GEOM.
2. The guest issues read requests via VirtIO queue 0 (`LBA 0` and `LBA 0x7FFFFF`), which the hypervisor completes with `VIRTIO_BLK_S_OK`.
3. The guest unblocks its random device (`random: unblocking device.`).
4. The guest reaches `Trying to mount root from ufs:/dev/vtbd0 []...` and waits.

The current fallback disk backing in `kernel/core/hypervisor/src/virtio_blk.c:64` returns all zeros for unallocated sectors. It contains no partition tables, no cylinder groups, no inode tables, and no `/sbin/init` binary. Consequently, FreeBSD's `vfs_mountroot` cannot mount root from `/dev/vtbd0`.

This document details the architectural options for provisioning an authentic FreeBSD root filesystem to successfully cross Boundary B14.

---

## 2. Analysis of Available Rootfs Assets in Repository

The repository already contains authentic FreeBSD 14.1 rootfs assets located in `tools/freebsd_payload/`:

| Asset Name | Size | Type | Content / Details |
| :--- | :--- | :--- | :--- |
| `tools/freebsd_payload/minimal_rootfs` | 4.16 MB (directory) | Raw FreeBSD Directory Tree | Contains `/sbin/init` (1.2MB), `/bin/sh` (168KB), `/bin/ls`, `/bin/cat`, `/bin/echo`, `/bin/hostname`, plus dynamic linkers (`/libexec/ld-elf.so.1`) and shared libraries (`/lib/libc.so.7`, `/lib/libedit.so.8`, etc.). |
| `tools/freebsd_payload/freebsd_rootfs.ufs2` | 502.1 MB (`526,516,224` bytes) | Raw UFS2 Filesystem Image | Extracted raw UFS2 partition 'a' with valid superblock magic `0x19540119` at offset 64KB. Contains full FreeBSD 14.1 userland. |
| `tools/freebsd_payload/freebsd_rootfs.img` | 534.6 MB (`560,615,936` bytes) | Full MBR Disk Image | Complete FreeBSD 14.1 mini-memstick image with MBR partition table, BSD disklabel, and UFS2 slice 2 at LBA 66601. |

---

## 3. Architecture Options for Boundary B14

### Option A: Authentic UFS2 Chunked RAM Disk Backing for VirtIO-BLK (Recommended)
- **Concept**:
  VirtIO-BLK in ATOMS OS already possesses a chunk-based sparse backing table:
  ```c
  #define VIRTIO_BLK_CHUNK_SIZE_BYTES  (2 * 1024 * 1024) // 2MB per chunk
  #define VIRTIO_BLK_SECTORS_PER_CHUNK 4096
  ```
  Instead of embedding a massive 500MB monolithic image into `BOOTX64.EFI`, we create a compact 16MB or 32MB UFS2 image containing the 4.16 MB `minimal_rootfs` tree (or stage the authentic `freebsd_rootfs.ufs2` metadata chunks: Superblock at 64KB, Cylinder Groups, Inode Table, and Root Directory).
- **How It Works**:
  1. A host build script (`tools/make_minimal_ufs2.py`) formats a 16MB UFS2 filesystem image containing the files in `minimal_rootfs`.
  2. The 16MB image is embedded as a payload asset or loaded via PXE TFTP alongside the kernel.
  3. When VirtIO-BLK receives read requests for LBA 0 through LBA 32768, it satisfies them from the memory-backed UFS2 image.
  4. FreeBSD's `vfs_mountroot` identifies the UFS2 superblock, mounts `/dev/vtbd0`, and executes `/sbin/init`.
- **Pros**:
  - Pure VirtIO-BLK compliance (exercises the certified B13 interrupt and request queue paths).
  - Fast TFTP transfer (<10 seconds over GbE).
  - Keeps `BOOTX64.EFI` within reasonable memory limits (<64MB).
- **Cons**:
  - Requires generating or extracting a valid UFS2 inode structure for the 4.16 MB binaries.

---

### Option B: FreeBSD Preloaded Memory Disk (`mfsroot` / `md0`)
- **Concept**:
  FreeBSD kernel natively supports booting from a memory disk (`md0`) loaded into memory by the bootloader before kernel execution:
  ```text
  vfs.root.mountfrom="ufs:/dev/md0"
  ```
  The bootloader passes an `mfsroot` module in the loader metadata (`MODTYPE_MD_IMAGE`).
- **How It Works**:
  1. `freebsd_loader.c` passes an ACPI/metadata module descriptor pointing to the RAM address of the rootfs image.
  2. The FreeBSD kernel creates `/dev/md0` during driver configuration and mounts root from `ufs:/dev/md0`.
- **Pros**:
  - Bypasses any disk geometry or partitioning issues.
  - Standard FreeBSD installer boot mechanism.
- **Cons**:
  - Does not exercise VirtIO-BLK during runtime (bypasses the virtual storage controller).

---

### Option C: Network NFS Root via VirtIO-NET (`vtnet0`)
- **Concept**:
  FreeBSD boots root from NFS over the already-negotiated `vtnet0` interface:
  ```text
  vfs.root.mountfrom="nfs:192.168.2.1:/freebsd_root"
  ```
- **How It Works**:
  1. Host laptop runs an NFS server exporting `tools/freebsd_payload/minimal_rootfs`.
  2. FreeBSD kernel uses BOOTP/DHCP over `vtnet0` and mounts the host directory over NFS.
- **Pros**:
  - Zero disk image size restrictions on physical target; host filesystem is live.
- **Cons**:
  - Requires VirtIO-NET packet transmit and receive ring emulation to be certified first (new boundary).

---

## 4. Recommended Strategy: Option A (16MB Compact UFS2 on VirtIO-BLK)

Option A is strongly recommended because:
1. It validates the complete end-to-end storage virtualization stack (VirtIO-BLK + GEOM + UFS2).
2. It reuses the 100% certified Boundary B13 interrupt delivery engine without architectural divergence.
3. The 4.16 MB `minimal_rootfs` contains everything necessary for FreeBSD to mount `/`, start `/sbin/init`, and launch a functional `/bin/sh` interactive shell or single-user milestone banner.

---

## 5. Technical Implementation Steps for Boundary B14 (When Approved)

1. **Step 1: Minimal UFS2 Image Generator (`tools/make_ufs2_rootfs.py`)**:
   - Construct a valid UFS2 filesystem image (16MB or 32MB) with:
     - Sector 0: Protective MBR / Partition table (or raw partition).
     - Offset 64KB: Standard UFS2 Superblock (`fs_magic = 0x19540119`, `fs_bsize = 32768`, `fs_fsize = 4096`).
     - Cylinder Group 0 descriptor, Block Bitmap, and Inode Allocation Bitmap.
     - Root Inode (Inode 2) pointing to directory containing entries: `.`, `..`, `bin`, `etc`, `lib`, `libexec`, `sbin`.
     - Inodes and data blocks for `/sbin/init`, `/bin/sh`, and supporting dynamic libraries from `tools/freebsd_payload/minimal_rootfs`.
2. **Step 2: VirtIO-BLK Memory Backing Integration**:
   - Link or load the generated UFS2 image as the backing storage for `VirtIOBlock` (`blk->storage_backing` in `kernel/core/hypervisor/src/virtio_blk.c`).
3. **Step 3: Loader Arguments Alignment (`kernel/core/hypervisor/src/freebsd_loader.c`)**:
   - Ensure `vfs.root.mountfrom` is configured to `ufs:/dev/vtbd0`.
   - Pass single-user mode flag (`-s` or `boot_single=1`) so FreeBSD launches `/bin/sh` directly upon mounting root.
4. **Step 4: QEMU Pre-Flight & Physical Target PXE Certification**:
   - Verify mountroot succeeds cleanly in QEMU.
   - Deploy to physical target via PXE and verify FreeBSD prints `/sbin/init` banner and reaches shell prompt.

---

## 6. Stop Condition & Next Action

Per Phase Isolation (Rule 0):
- **NO PRODUCTION FILES HAVE BEEN EDITED FOR BOUNDARY B14**.
- Boundary B13 is certified.
- Awaiting user review and formal approval of `ROOTFS_PLAN.md` before proceeding to PATCH phase for Boundary B14.
