# AUDIT PART 7: GUEST KERNEL & USERLAND COMPATIBILITY
**Document**: `docs/audit/07_guest.md`  
**Classification**: READ-ONLY AUDIT - EVIDENCE-BASED  
**Date**: 2026-11-01  
**Scope**: FreeBSD kernel version, userland ABI parity, `tmpfs` binary audit, `RLIMIT_PIPEBUF`, and module compatibility.

---

## 1. Concrete Binary Audit: Kernel Version & `tmpfs`

### 1.1 Embedded Kernel Version: FreeBSD 14.1-RELEASE
- [VERIFIED-BY-CODE: Binary string inspection of `tools/freebsd_payload/freebsd_stripped_kernel.elf`]
- Version string: `@(#)FreeBSD 14.1-RELEASE releng/14.1-n267679-10e31f0946d8 GENERIC`
- Kernel OS Version integer: `__FreeBSD_version = 1401000`.

### 1.2 Userland Version on USB Image: FreeBSD 14.5
- [VERIFIED-BY-CODE: Inspection of `vtbd0p2` userland binaries]
- Userland OS Version integer: `__FreeBSD_version = 1405000`.

### 1.3 Definitive `tmpfs`, `nullfs`, and `unionfs` Binary Audit
A Python binary inspection of `tools/freebsd_payload/freebsd_stripped_kernel.elf` was executed to resolve the contradiction between earlier reports:

| Filesystem | Static Kernel Symbols Found | Key Symbols Present | Included in Embedded Kernel? |
|---|---|---|---|
| **`tmpfs`** | **215 matches** | `vfs_tmpfs`, `tmpfs_alloc_file`, `tmpfs_dir_lookup`, `tmpfs_stat`, `tmpfs_getattr`, `vfs_vector_tmpfs_vnodeop_entries_f_sys_init` | **YES** |
| **`nullfs`** | **0 matches** | None | **NO** |
| **`unionfs`** | **0 matches** | None | **NO** |

#### Why did `/sbin/mount -t tmpfs tmpfs /var` fail in the minimal rootfs?
- In FreeBSD, the `/sbin/mount` command delegates filesystem-specific mounting to an external helper binary named `/sbin/mount_<fstype>`.
- Inspection of `tools/freebsd_payload/minimal_rootfs/sbin/` reveals:
  - Files present: `dhclient`, `dhclient-script`, `dmesg`, `ifconfig`, `init`, `mount`, `ping`, `ping6`, `route`, `sysctl`.
  - **`/sbin/mount_tmpfs` was NOT included in the directory!**
- When `/etc/rc` ran `/sbin/mount -t tmpfs tmpfs /var`, `/sbin/mount` attempted `execv("/sbin/mount_tmpfs", ...)` and failed with `No such file or directory`.
- **Earlier AI Error**: An earlier report mistakenly claimed "TMPFS is NOT compiled into the static GENERIC kernel". That claim was false: `tmpfs` is compiled into the kernel, but the userland helper `/sbin/mount_tmpfs` was missing from the 16MB ramdisk.
- On the full USB image (`vtbd0p2`), `/sbin/mount_tmpfs` exists in the standard FreeBSD installation, allowing `tmpfs` mounts to succeed.

---

## 2. Root Cause of `getting pipebuf resource limit: invalid argument`

- [VERIFIED-ON-HARDWARE: User photograph of physical console on ASUS B760M-K]
- Message: `2026-10-31T21:33:01.980904+00:00 - INIT 17 - - GETTING PIPEBUF RESOURCE LIMIT : INVALID ARGUMENT`
- **Root Cause**:
  - In FreeBSD 14.5, a new resource limit `RLIMIT_PIPEBUF` (`15`) was introduced in `/usr/include/sys/resource.h` (`RLIM_NLIMITS = 16`).
  - When FreeBSD 14.5 `/sbin/init` starts, it issues `getrlimit(RLIMIT_PIPEBUF, &rl)`.
  - The running kernel is FreeBSD 14.1-RELEASE, where `RLIM_NLIMITS = 15` (valid limits are 0 through 14).
  - In FreeBSD 14.1 `sys/kern/kern_resource.c`:
    ```c
    if (which >= RLIM_NLIMITS) return (EINVAL);
    ```
  - Kernel returns `EINVAL` (`22`, Invalid argument).
  - While `init` treats this as a non-fatal warning, it is concrete forensic proof that the userland is running on an older kernel.

---

## 3. Downstream Risks of 14.1 Kernel with 14.5 Userland

1. **Kernel Module Loading Failure**:
   Any 14.5 loadable kernel modules (`.ko` files) located under `/boot/kernel/` will fail to load with version mismatch:
   `module_register: module ... version 1405000 != kernel version 1401000`.
2. **Xorg & DRI/DRM Panics**:
   Display servers and graphic drivers query kernel-specific DRM ioctls; running mismatched userland drivers against an older kernel risks undefined ioctl errors.
3. **Firefox & Web Engine Dependencies**:
   Modern Chromium and Firefox binaries rely on specific FreeBSD 14.5 syscall semantics (e.g. `shm_open2`, updated `cap_enter` capabilities).

---

## 4. Top 10 Claims Least Sure About (Part 7)

1. Whether FreeBSD 14.5 userland has fallback paths when `RLIMIT_PIPEBUF` returns `EINVAL`.
2. If Firefox 159 requires shared memory (`tmpfs` mounted on `/tmp`) to launch without crashing.
3. Whether the embedded kernel ELF includes `device vt` (Newcons) or `device sc` (Syscons) as the active video driver.
4. If extracting `/boot/kernel/kernel` from the 14.5 USB image increases the compressed EFI payload size beyond PXE TFTP limits.
5. Whether the 14.5 kernel requires additional loader metadata entries (`MODINFO_NAME`, `MODINFO_TYPE`) not emitted by `freebsd_loader.c`.
6. Exact memory consumption of FreeBSD 14.5 GENERIC kernel with all default drivers loaded.
7. Whether FreeBSD 14.5 `syslogd` requires `/var/run/log` socket to exist before daemonizing.
8. If userland `dhclient` binary on 14.5 functions reliably when running on a 14.1 kernel.
9. Whether `shm_open2` syscall exists in FreeBSD 14.1-RELEASE.
10. Latency of parsing UFS2 superblock during direct kernel bootstrap compared to EFI loader.
