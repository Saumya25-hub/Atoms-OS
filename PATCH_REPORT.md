# PATCH_REPORT.md — Unblocking FreeBSD Live Shell Initialization
## Date: 2026-10-21
## Author: Atoms OS Patch Team

---

## 1. Files Changed

### File 1: `tools/freebsd_payload/minimal_rootfs/etc/ttys`
- **Lines Changed**:
  - Line 29: Changed `console "/bin/sh" vt100 on secure` -> `console none unknown off secure`
  - Line 43: Changed `ttyu0 "/bin/sh" vt100 on secure` -> `ttyu0 none unknown off secure`
  - Line 31 (`ttyv0`): Kept active as `ttyv0 "/bin/sh" xterm on secure`
- **Rationale**:
  Eliminates the indefinite blocking loop inside `start_getty()` caused by `ttyu0` waiting for the physical DCD (Data Carrier Detect) hardware line on bare-metal COM1 UART. Disables redundant `console` alias to avoid duplicate session conflicts on the primary display.

### File 2: `tools/freebsd_payload/minimal_rootfs/etc/rc`
- **Lines Changed**:
  - Line 19: Appended `exec /bin/sh`
- **Rationale**:
  Prevents `/etc/rc` from exiting at EOF, keeping the spawned root process alive and active as an interactive live shell directly attached to the system console.

### File 3: `tools/make_minimal_ufs2.py`
- **Lines Changed**:
  - Line 170: Changed `is_exec` flag from `False` to `True` for `/etc/rc`
- **Rationale**:
  Sets `/etc/rc` file mode to `0755` (`rwxr-xr-x`) in the UFS2 filesystem inode table, ensuring `access("/etc/rc", X_OK)` succeeds.

### File 4: `kernel/core/hypervisor/src/freebsd_loader.c`
- **Lines Changed**:
  - Lines 168–169: Added `"init_script=/etc/rc"` and `"init_shell=/bin/sh"` to `env_vars[]`
- **Rationale**:
  Instructs FreeBSD 14 `/sbin/init` via `kenv(KENV_GET, "init_script")` to directly invoke `/etc/rc` with `/bin/sh`.

---

## 2. Build Verification
- Image Builder: `python tools/make_minimal_ufs2.py` -> Successfully generated 16MB authentic UFS2 `build/media.img`.
- GPT Image Builder: `build/gpt_image_builder.exe` -> Successfully generated `build/atoms_uefi_test.img`.
- Kernel & Bootloader: `build.ps1` -> Clean compilation with 0 errors. Updated `build/BOOTX64.EFI` (65,983,488 bytes) generated.
