# PATCH_PLAN.md — Unblocking FreeBSD Userland Live Shell
## Date: 2026-10-21
## Author: Atoms OS Architecture Team

---

## 1. Input Reference
- Forensic Report: `d:\Signatures_OS\FORENSIC_REPORT.md`

---

## 2. Scope of Modifications (Strict File Whitelist)
Only the following files are permitted to be modified:
1. `tools/freebsd_payload/minimal_rootfs/etc/ttys`
2. `tools/freebsd_payload/minimal_rootfs/etc/rc`
3. `tools/make_minimal_ufs2.py`
4. `kernel/core/hypervisor/src/freebsd_loader.c`

No other files (especially `kernel.c`, `hypervisor.c`, `virtual_platform.c`) may be modified.

---

## 3. Modification Specifications

### File 1: `tools/freebsd_payload/minimal_rootfs/etc/ttys`
- **What**: 
  - Change `console` status from `on` to `off`.
  - Ensure `ttyv0` remains `on` with command `/bin/sh` and type `xterm`.
  - Change `ttyu0` status from `on` to `off`.
- **Why**: 
  - Prevents `init` from attempting to open serial port `ttyu0`, which blocks indefinitely waiting for DCD modem carrier detect.
  - Eliminates duplicate terminal conflict between `console` and `ttyv0`.

### File 2: `tools/freebsd_payload/minimal_rootfs/etc/rc`
- **What**: 
  - Append `exec /bin/sh` to the end of the script.
- **Why**: 
  - Prevents `/etc/rc` from exiting at EOF, which would cause `init` to proceed to `/etc/ttys` clean state.
  - Directly launches an interactive root shell `# ` that stays permanently active on the primary console.

### File 3: `tools/make_minimal_ufs2.py`
- **What**: 
  - Change the `is_exec` flag for `rc` from `False` to `True`.
- **Why**: 
  - Gives `/etc/rc` executable mode `0755` (`rwxr-xr-x`) in the UFS2 filesystem inode table, allowing `access("/etc/rc", X_OK)` to succeed without returning `EACCES`.

### File 4: `kernel/core/hypervisor/src/freebsd_loader.c`
- **What**: 
  - Add `"init_script=/etc/rc"` to the `env_vars` array passed to FreeBSD kernel BootInfo environment.
- **Why**: 
  - Explicitly instructs FreeBSD 14.1 `/sbin/init` to execute `/etc/rc` as the initial userland script via `kenv(KENV_GET, "init_script")`.

---

## 4. Expected Results
1. FreeBSD kernel mounts `/dev/vtbd0`.
2. Kernel executes `/sbin/init`.
3. `/sbin/init` reads `init_script=/etc/rc` from `kenv` and spawns `/bin/sh /etc/rc autoboot`.
4. The physical screen and serial log render the Atoms OS Hypervisor Live Shell banner:
   ```text
   ==================================================================
       ATOMS TYPE-1 HYPERVISOR -- FREEBSD 14.1-RELEASE LIVE SHELL   
   ==================================================================
     Host: Atoms OS Bare-Metal Micro-Hypervisor
     Guest: FreeBSD 14.1-RELEASE amd64 (GENERIC)
     Root Disk: /dev/vtbd0 (UFS2 Minimal Fast File System)
   ==================================================================
   ```
5. `/etc/rc` executes `exec /bin/sh`, presenting an interactive root shell `# ` prompt.

---

## 5. Risk & Rollback Plan
- **Risk**: Very low. All changes conform to standard FreeBSD system administration practices and UFS2 image structure.
- **Rollback**:
  - Revert `tools/freebsd_payload/minimal_rootfs/etc/ttys` via git checkout.
  - Revert `tools/freebsd_payload/minimal_rootfs/etc/rc` via git checkout.
  - Revert `tools/make_minimal_ufs2.py` via git checkout.
  - Revert `kernel/core/hypervisor/src/freebsd_loader.c` via git checkout.
